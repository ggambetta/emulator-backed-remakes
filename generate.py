#!/usr/bin/python

import collections
import os.path
import sys

# Keep in sync with the enums in x86_base.h
REGS_16 = ["AX", "BX", "CX", "DX", "CS", "DS", "SS", "ES", "BP", "SP", "DI", "SI"]
REGS_8 = ["AL", "AH", "BL", "BH", "CL", "CH", "DL", "DH"]

SEGMENT_OVERRIDE_OPCODES = ["ES:", "CS:", "SS:", "DS:"]
REP_OPCODES = ["REPZ", "REPNZ"]

PREFIX_OPCODES = SEGMENT_OVERRIDE_OPCODES + REP_OPCODES

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


  def getWordSuffix(self, args = None):
    if self.word_suffix is not None:
      return self.word_suffix

    args = args or self.args

    self.word_suffix = ""
    for arg in args:
      if len(arg) == 2:
        if arg[1] in "0b":
          self.word_suffix = "_b"
        elif arg[1] in "wv":
          self.word_suffix = "_w"
        elif arg[1] == "p":
          self.word_suffix = "_p"
        elif arg in REGS_16:
          self.word_suffix = "_w"
        if arg.upper() in REGS_16:
          self.word_suffix = "_w"

      if len(arg) == 1:
        if arg in ["1", "3"]:
          self.word_suffix = "_b"

    if self.args:
      assert(self.word_suffix != "")

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
# Parse the opcodes table.
#
all_opcodes = []
base_opcodes = []
group_opcodes = collections.defaultdict(list)

for line in file(OPCODES_TABLE_FILENAME, "rb"):
  tokens = line.strip().split()

  if len(tokens) < 2 or tokens[1] == "--":
    continue

  group = None
  opcode = Opcode()
  opcode.name = tokens[1].upper()
  opcode.args = tokens[2:]

  for i, arg in enumerate(opcode.args):
    if arg[0] == "e" and arg[1:] in REGS_16:
      opcode.args[i] = arg[1:]

  if tokens[0].startswith("GRP"):
    # Group opcode, e.g. GRP1/5
    group, op = tokens[0].split("/")
    group = group.upper()
    opcode.opcode = int(op)

  else:
    # Regular opcode.
    opcode.opcode = int(tokens[0], 16)


  all_opcodes.append(opcode)
  if group:
    group_opcodes[group].append(opcode)
  else:
    base_opcodes.append(opcode)


#
# Build list of method variants (word/byte/immediate/etc).
#
method_variants = collections.defaultdict(set)
for opcode in base_opcodes:
  if not opcode.name.startswith("GRP"):
    method_variants[opcode.name.upper()].add(opcode.getWordSuffix())
    continue

  # For group opcodes without arguments, use the group's.
  group = opcode.name 
  group_args = opcode.args
  for subopcode in group_opcodes[group]:
    args = subopcode.args or group_args
    method_variants[subopcode.name].add(opcode.getWordSuffix(args))

#for k in sorted(method_variants.keys()):
#  print k, list(method_variants[k])

#sys.exit(1)


#
# Generate dispatcher code and build list of method names.
#
class Method:
  def __init__(self):
    self.cpp_name = None
    self.name = None


methods = {} 

def addMethod(opcode, args = None):
  global methods

  # Compute name of implementation method.
  # If there's only one word/byte variant of the opcode, don't add the suffix
  # to the name.
  name = opcode.name
  args = args or opcode.args

  if len(method_variants[name]) > 1:
    name += opcode.getWordSuffix(args)

  if methods.has_key(name):
    method = methods[name]
  else:
    method = Method()
    method.name = name
    method.cpp_name = name.replace(":", "_")
    methods[name] = method

  return method


def getFetchArgCode(suffix, arg):
  fetch_arg_code = []
  if arg in REGS_16:
    fetch_arg_code.append("w%s = getReg16Ptr(R16_%s);" % (suffix, arg))
    fetch_arg_code.append("addConstArgDesc(\"%s\");" % arg)
  elif arg in REGS_8:
    fetch_arg_code.append("b%s = getReg8Ptr(R8_%s);" % (suffix, arg))
    fetch_arg_code.append("addConstArgDesc(\"%s\");" % arg)
  elif arg in ["Gv"]:
    fetch_arg_code.append("w%s = decodeReg_w();" % suffix)
  elif arg in ["Ev", "Ew"]:
    fetch_arg_code.append("w%s = decodeRM_w();" % suffix)
  elif arg in ["Gb"]:
    fetch_arg_code.append("b%s = decodeReg_b();" % suffix)
  elif arg in ["Eb"]:
    fetch_arg_code.append("b%s = decodeRM_b();" % suffix)
  elif arg in ["Sw"]:
    fetch_arg_code.append("w%s = decodeS();" % suffix)
  elif arg in ["Iv", "Iw"]:
    fetch_arg_code.append("w%s = decodeI_w();" % suffix)
  elif arg in ["Ib"]:
    fetch_arg_code.append("b%s = decodeI_b();" % suffix)
  elif arg in ["Jv"]:
    fetch_arg_code.append("w%s = decodeJ_w();" % suffix)
  elif arg in ["Jb"]:
    fetch_arg_code.append("w%s = decodeJ_b();" % suffix)

  return fetch_arg_code


DISPATCHER = "if (false) {\n"
for opcode in base_opcodes:  
  # Generate top-level switch.
  desc = opcode.name + " " + ", ".join(opcode.args)
  DISPATCHER += "} else if (opcode_ == 0x%02X) {  // %s\n" % (opcode.opcode,
                                                              desc.strip())
  
  group = None
  if opcode.name.startswith("GRP"):
    group = opcode.name
    DISPATCHER += "  int op = decodeGRP();\n"

    assert group_opcodes.has_key(group)
    op_group = group_opcodes[group]
    
    first = True
    for subopcode in op_group:
      if first:
        cppif = "if "
        first = False
      else:
        cppif = "} else if "

      args = subopcode.args or opcode.args
      method = addMethod(subopcode, args)
      desc = subopcode.name + " " + ", ".join(args)
      DISPATCHER += "  " + cppif + "(op == 0x%02X) {  // %s\n" % (subopcode.opcode, desc.strip())
      DISPATCHER += "  opcode_desc_ = \"%s\";\n" % subopcode.name

      suffixes = ["arg1", "arg2"]
      for arg in args:
        for line in getFetchArgCode(suffixes.pop(0), arg):
          DISPATCHER += "    " + line + "\n"
   
      DISPATCHER += "    %s();\n" % method.cpp_name

    DISPATCHER += "  }\n"

  else:
    DISPATCHER += "  opcode_desc_ = \"%s\";\n" % opcode.name
    method = addMethod(opcode)

    if method.name in PREFIX_OPCODES:
      # Prefix opcode.
      DISPATCHER += "  is_prefix = true;\n"

    if method.name in SEGMENT_OVERRIDE_OPCODES:
      # Segment override.
      assert len(method.name) == 3
      assert method.name[2] == ":"
      reg = method.name[:2]
      DISPATCHER += "  segment_ = *getReg16Ptr(R16_%s);\n" % reg
      DISPATCHER += "  segment_desc_ = \"%s\";\n" % method.name
  
    elif method.name in REP_OPCODES:
      # REP opcode.
      DISPATCHER += "  rep_opcode_ = opcode_;\n"
      DISPATCHER += "  rep_opcode_desc_ = \"%s \";\n" % method.name
  
    else:
      # General case opcode. Generate code to prepare the arguments.
      suffixes = ["arg1", "arg2"]
      for arg in opcode.args:
        for line in getFetchArgCode(suffixes.pop(0), arg):
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
