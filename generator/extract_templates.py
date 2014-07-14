#!/usr/bin/python


CPP_OUT = "x86_base.cpp"
H_OUT = "x86_base.h"

GENERATED_CODE_PLACEHOLDER = "// GENERATED CODE"
GENERATED_CODE_BEGIN = "// BEGIN GENERATED CODE"
GENERATED_CODE_END = "// END GENERATED CODE"

CPP_TEMPLATE = CPP_OUT + ".template"
H_TEMPLATE = H_OUT + ".template"

for src, dst in [(CPP_OUT, CPP_TEMPLATE), (H_OUT, H_TEMPLATE)]:
  out = []
  begin = False
  end = False
  for line in file(src, "rb").readlines():
    line = line.rstrip()
    
    if GENERATED_CODE_BEGIN in line:
      assert not begin
      begin = True
      indent = line[:line.find(GENERATED_CODE_BEGIN)]
      out.append(indent + GENERATED_CODE_PLACEHOLDER)
    elif GENERATED_CODE_END in line:
      assert begin
      assert not end
      end = True
      begin = False
    else:
      if not begin:
        out.append(line)

  file(dst, "wb").write("\n".join(out))


