#!/bin/sh
# Lifts the eboot and the PRXs it loads at start into generated/, at the
# bases src/main.cpp maps them at. Run from the project directory.
#
# usage: tools/lift.sh APP0_DIR [RECOMP_LIFT]
set -e
G=$1
L=${2:-out/build/release/x86recomp/tools/lifter_x86/recomp_lift}
if [ -z "$G" ]; then
  echo "usage: tools/lift.sh APP0_DIR [RECOMP_LIFT]" >&2
  exit 2
fi
"$L" --exe "$G/eboot.bin" --symbols "" --manifest manifest/eboot.manifest \
  --module "$G/sce_module/libSceFios2.prx" --symbols "" \
  --manifest manifest/libSceFios2.manifest --base 800000000 \
  --module "$G/sce_module/libc.prx" --symbols "" --manifest manifest/libc.manifest \
  --base 810000000 \
  --out generated
