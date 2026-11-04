#!/bin/sh
# Lifts the eboot and the PRXs it ships into generated/, through
# SwARc's platforms/orbis/tools/lift.sh (the modules and their bases are
# the platform's, as grr maps them). Run from the project directory.
#
# usage: tools/lift.sh APP0_DIR [BUILD_DIR]
set -e
G=$1
B=${2:-out/build/release}
if [ -z "$G" ]; then
  echo "usage: tools/lift.sh APP0_DIR [BUILD_DIR]" >&2
  exit 2
fi
S=$(sed -n 's/^SWARC_DIR:PATH=//p' "$B/CMakeCache.txt")
exec sh "$S/platforms/orbis/tools/lift.sh" "$B/swarc/tools/lifter_x86/recomp_lift" \
  "$B/bin/orbis_app" "$G" manifest generated
