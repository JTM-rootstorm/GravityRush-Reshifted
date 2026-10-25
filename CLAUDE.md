# Gravity Rush Remastered (PS4, CUSA01130) → native Linux/macOS static recomp

Rules only. Status, findings and next steps go in FINDINGS.md. The
lifter, the x87/SSE/AVX models, the runtime and the PS4 platform module
(platforms/orbis: the ELF loader, libkernel and the other system
libraries' HLE) live in the shared x86recomp library (../../x86recomp,
its own git repo with its own CLAUDE.md); this repo holds what is
specific to this game. It is the first PS4 user of x86recomp
(ARCHITECTURE.md there, "Console platforms").

## Layout
- Project: ~/Documents/Programming/Recomp/PS4/GravityRush (PS4/ is for
  other PS4 games too)
- Library: ~/Documents/Programming/Recomp/x86recomp (add_subdirectory;
  X86RECOMP_DIR overrides the path). Library changes are made in a
  worktree (../../x86recomp-<topic>; the PS4 work is in
  ../../x86recomp-ps4) and committed there; its rules apply.
- Game: the decrypted app0 dumped from the user's own console (NEVER
  commit or copy into the repo):
  ~/Documents/Programming/Recomp/PS4-HB/dumps/games/CUSA01130/mnt/sandbox/pfsmnt/CUSA01130-app0/
  (eboot.bin, sce_module/*.prx, arc/*.psarc, sce_sys/). NID lists and
  names for it are in PS4-HB/inventory/ (local only, never commit).
- Firmware names for NIDs (PS4-HB/refs/shadps4/names.txt) come from a
  GPL project: a local reference only, never copied here or into the
  library.

## Build
- `cmake --preset release -DX86RECOMP_DIR=$HOME/Documents/Programming/Recomp/x86recomp-ps4 && cmake --build --preset release`
  (the PS4 platform module is turned on by this project's CMakeLists).
- Lifting is a separate step; re-run it after any change to the
  library's tools/lifter, manifest/ or the binaries, then re-run cmake
  (the generated file list is globbed):
  `tools/lift.sh "$HOME/Documents/Programming/Recomp/PS4-HB/dumps/games/CUSA01130/mnt/sandbox/pfsmnt/CUSA01130-app0"`
  It lifts the eboot and the PRXs it loads at start; their bases there
  and in src/main.cpp must agree. generated/lift_report.txt lists
  unimplemented instructions and warnings.
- Run: `out/build/release/grr --game-dir <app0>`. GRR_TRACE=1 logs
  every HLE call; GRR_NID_NAMES=<names file> names unimplemented imports
  by name instead of NID; GRR_BACKGROUND=1 for automated runs, which go
  inside a memory cap:
  `systemd-run --user --scope --quiet -p MemoryMax=6G -p MemorySwapMax=0 <command>`.
