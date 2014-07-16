#!/usr/bin/python

import collections
import os.path
import sys

# Keep in sync with the enums in x86_base.h
REGS_16 = ["AX", "BX", "CX", "DX", "CS", "DS", "SS", "ES", "BP", "SP", "DI", "SI"]
REGS_8 = ["AL", "AH", "BL", "BH", "CL", "CH", "DL", "DH"]

SEGMENT_OVERRIDE_OPCODES = ["ES:", "CS:", "SS:", "DS:"]
PREFIX_OPCODES = SEGMENT_OVERRIDE_OPCODES
NOP_OPCODES = ["NOP"] 
NON_MANDATORY_OPCODES = PREFIX_OPCODES + NOP_OPCODES


# Input and output filenames.
CPP_OUT = "x86_base.cpp"
H_OUT = "x86_base.h"
GENERATOR_PATH = "generator"

CPP_TEMPLATE = os.path.join(GENERATOR_PATH, CPP_OUT + ".template")
H_TEMPLATE = os.path.join(GENERATOR_PATH, H_OUT + ".template")
OPCODES_TABLE_FILENAME = os.path.join(GENERATOR_PATH, "8086_table.txt")


# Generated code markers.
GENERATED_CODE_PLACEHOLDER = "// GENERATED CODE"
GENERATED_CODE_BEGIN = "// BEGIN GENERATED CODE"
GENERATED_CODE_END = "// END GENERATED CODE"


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
for line in file(OPCODES_TABLE_FILENAME, "rb"):
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
# Build list of method variants (word/byte/immediate/etc).
#
method_variants = collections.defaultdict(set)
for opcode in opcodes:
  method_variants[opcode.name.upper()].add(opcode.getWordSuffix())


#
# Generate dispatcher code and build list of method names.
#
class Method:
  def __init__(self):
    self.cpp_name = None
    self.name = None


DISPATCHER = "if (false) {\n"
methods = {} 


for opcode in opcodes:
  # Compute name of implementation method.
  # If there's only one word/byte variant of the opcode, don't add the suffix
  # to the name.
  name = opcode.name.upper()
  if len(method_variants[name]) > 1:
    name += opcode.getWordSuffix()

  if methods.has_key(name):
    method = methods[name]
  else:
    method = Method()
    method.name = name
    method.cpp_name = name.replace(":", "_")
    methods[name] = method

  
  # Generate switch.
  desc = opcode.name + " " + ", ".join(opcode.args)
  DISPATCHER += "} else if (opcode_ == 0x%02X) {  // %s\n" % (opcode.opcode,
                                                              desc.strip())

  DISPATCHER += "  opcode_desc_ = \"%s\";\n" % opcode.name.upper()

  if method.name in PREFIX_OPCODES:
    DISPATCHER += "  is_prefix = true;\n"

  if method.name in SEGMENT_OVERRIDE_OPCODES:
    # Segment override.
    assert len(method.name) == 3
    assert method.name[2] == ":"
    reg = method.name[:2]
    DISPATCHER += "  segment_ = *getReg16Ptr(R16_%s);\n" % reg
    DISPATCHER += "  segment_desc_ = \"%s\";\n" % method.name

  else:
    # General case opcode. Generate code to prepare the arguments.
    fetched_modrm = False
    suffixes = ["arg1", "arg2"]
    for arg in opcode.args:
      suffix = suffixes.pop(0)
      fetch_arg_code = []

      # eAX => AX, etc.
      if arg[0] == "e" and arg[1:] in REGS_16:
        arg = arg[1:]

      need_modrm = False

      if arg in REGS_16:
        fetch_arg_code.append("w%s = getReg16Ptr(R16_%s);" % (suffix, arg))
        fetch_arg_code.append("addConstArgDesc(\"%s\");" % arg)
      elif arg in REGS_8:
        fetch_arg_code.append("b%s = getReg8Ptr(R8_%s);" % (suffix, arg))
        fetch_arg_code.append("addConstArgDesc(\"%s\");" % arg)
      elif arg in ["Gv"]:
        fetch_arg_code.append("w%s = decodeReg_w();" % (suffix))
        need_modrm = True
      elif arg in ["Ev", "Ew"]:
        fetch_arg_code.append("w%s = decodeRM_w();" % (suffix))
        need_modrm = True
      elif arg in ["Gb"]:
        fetch_arg_code.append("b%s = decodeReg_b();" % (suffix))
        need_modrm = True
      elif arg in ["Eb"]:
        fetch_arg_code.append("b%s = decodeRM_b();" % (suffix))
        need_modrm = True
      elif arg in ["Sw"]:
        fetch_arg_code.append("w%s = decodeS();" % (suffix))
        need_modrm = True
      elif arg in ["Iv", "Iw"]:
        fetch_arg_code.append("w%s = decodeI_w();" % (suffix))

      if need_modrm and not fetched_modrm:
        fetched_modrm = True
        fetch_arg_code.insert(0, "fetchModRM();")

      for line in fetch_arg_code:
        DISPATCHER += "  " + line + "\n"


  # Call the custom implementation.
  DISPATCHER += "  %s();\n" % method.cpp_name
 
DISPATCHER += """} else { 
  invalidOpcode();
}"""


#
# Generate the cpp file.
#
cpp_code = insertCode(file(CPP_TEMPLATE, "rb").read(), DISPATCHER, GENERATED_CODE_PLACEHOLDER)
file(CPP_OUT, "wb").write(cpp_code)


#
# Generate the header file.
#
declarations = []
for mname in sorted(methods.keys()):
  method = methods[mname]

  decl = "virtual void %-6s() {" % method.cpp_name
  if method.name not in NON_MANDATORY_OPCODES:
    decl += " notImplemented(\"%s\"); " % method.name
  decl += "}" 

  declarations.append(decl)

METHODS = "\n".join(declarations)

h_code = insertCode(file(H_TEMPLATE, "rb").read(), METHODS, GENERATED_CODE_PLACEHOLDER)
file(H_OUT, "wb").write(h_code)
