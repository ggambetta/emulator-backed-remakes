#!/usr/bin/python

import collections
import sys

# Keep in sync with enum in x86_base.h
REGS_16 = ["AX", "BX", "CX", "DX", "CS", "DS", "SS", "ES", "BP", "SP", "DI", "SI"]
REGS_8 = ["AL", "AH", "BL", "BH", "CL", "CH", "DL", "DH"]


CPP_OUT = "x86_base.cpp"
H_OUT = "x86_base.h"

GENERATED_CODE_PLACEHOLDER = "// GENERATED CODE"
GENERATED_CODE_BEGIN = "// BEGIN GENERATED CODE"
GENERATED_CODE_END = "// END GENERATED CODE"

CPP_TEMPLATE = CPP_OUT + ".template"
H_TEMPLATE = H_OUT + ".template"

class Opcode:
  def __init__(self):
    self.opcode = None
    self.name = ""
    self.args = []
    self.word_suffix = None


  def getWordSuffix(self):
    if self.word_suffix is not None:
      return self.word_suffix

    self.word_suffix = ""
    for arg in self.args:
      if len(arg) == 2:
        if arg[1] in "0b":
          self.word_suffix = "_b"
        elif arg[1] in "wv":
          self.word_suffix = "_w"
        elif arg[1] == "p":
          self.word_suffix = "_p"
    return self.word_suffix


def insertCode(template, code, marker):
  lines = template.split("\n")
  out = []
  for line in lines:
    idx = line.find(marker)
    if idx == -1:
      out.append(line)
      continue
  
    indent = line[:idx]
    out.append(indent + GENERATED_CODE_BEGIN)
    for code_line in code.split("\n"):
      out.append(indent + code_line)
    out.append(indent + GENERATED_CODE_END)

  return "\n".join(out)



#
# Filter and parse the opcodes table.
#
opcodes = []
for line in file("8086_table.txt", "rb"):
  tokens = line.strip().split()

  if not tokens:
    break

  if len(tokens) < 2:
    continue

  if len(tokens[0]) != 2:
    continue

  if tokens[1] == "--":
    continue

  opcode = Opcode()
  opcode.opcode = int(tokens[0], 16)
  opcode.name = tokens[1].upper()
  opcode.args = tokens[2:]

  opcodes.append(opcode)



#
# Build list of method names.
#
method_variants = collections.defaultdict(set)
for opcode in opcodes:
  method_variants[opcode.name.upper()].add(opcode.getWordSuffix())


#
# Generate dispatcher code and build list of method names.
#
DISPATCHER = "if (false) {\n"
methods = [] 


for opcode in opcodes:
  # Compute name of implementation method.
  # If there's only one word/byte variant of the opcode, don't add the suffix to the name.
  impl_name = opcode.name.upper()

  if len(method_variants[impl_name]) > 1:
    impl_name += opcode.getWordSuffix()

  impl_name = impl_name.replace(":", "_")
  methods.append(impl_name)

  # Generate code to prepare the arguments.
  desc = opcode.name + " " + ", ".join(opcode.args)
  DISPATCHER += "} else if (opcode_ == 0x%02X) {  // %s\n" % (opcode.opcode, desc.strip())

  fetched_modrm = False

  suffixes = ["arg1", "arg2"]
  for arg in opcode.args:
    suffix = suffixes.pop(0)
    fetch_arg_code = None

    #DISPATCHER += "  // %s\n" % arg

    # eAX => AX
    if arg[0] == "e" and arg[1:] in REGS_16:
      arg = arg[1:]

    need_modrm = False

    if arg in REGS_16:
      fetch_arg_code = "w%s = getReg16Ptr(R16_%s);\n" % (suffix, arg)
    elif arg in REGS_8:
      fetch_arg_code = "b%s = getReg8Ptr(R8_%s);\n" % (suffix, arg)
    elif arg in ["Gv"]:
      fetch_arg_code = "w%s = decodeReg_w();\n" % (suffix)
      need_modrm = True
    elif arg in ["Ev"]:
      fetch_arg_code = "w%s = decodeRM_w();\n" % (suffix)
      need_modrm = True
    elif arg in ["Gb"]:
      fetch_arg_code = "b%s = decodeReg_b();\n" % (suffix)
      need_modrm = True
    elif arg in ["Eb"]:
      fetch_arg_code = "b%s = decodeRM_b();\n" % (suffix)
      need_modrm = True

    if need_modrm and not fetched_modrm:
      fetched_modrm = True
      DISPATCHER += "  fetchModRM();\n"

    if fetch_arg_code:
      DISPATCHER += "  " + fetch_arg_code


  # Call the custom implementation.
  DISPATCHER += "  %s();\n" % impl_name
 
DISPATCHER += """} else { 
  invalidOpcode();
}"""

cpp_code = insertCode(file(CPP_TEMPLATE, "rb").read(), DISPATCHER, GENERATED_CODE_PLACEHOLDER)
file(CPP_OUT, "wb").write(cpp_code)

max_name_len = max([len(m) for m in methods])

methods = sorted(list(set(methods)))
METHODS = "\n".join(["virtual void %-6s() { notImplemented(\"%s\"); }" % (method, method) for method in methods])

h_code = insertCode(file(H_TEMPLATE, "rb").read(), METHODS, GENERATED_CODE_PLACEHOLDER)
file(H_OUT, "wb").write(h_code)
