# GravityRush-Reshifted: Gravity Rush Remastered (PS4, CUSA01130) → native Linux/macOS static recomp

Rules only. Status, findings and next steps go in FINDINGS.md. The
lifter, the x87/SSE/AVX models, the runtime and the PS4 platform module
(platforms/orbis: the ELF loader, libkernel and the other system
libraries' HLE) live in the shared SwARc library (../../SwARc,
its own git repo with its own CLAUDE.md); this repo holds what is
specific to this game. It is the first PS4 user of SwARc
(ARCHITECTURE.md there, "Console platforms").

## Layout
- Project: ~/Documents/Programming/Recomp/PS4/GravityRush (PS4/ is for
  other PS4 games too); GitHub: JTM-rootstorm/GravityRush-Reshifted
- Library: ~/Documents/Programming/Recomp/SwARc (add_subdirectory;
  SWARC_DIR overrides the path). Library changes are made in a
  topic worktree (../../SwARc-<topic>, cut from main) and landed with
  its tools/land.py; its rules apply.
- Game: the decrypted app0 dumped from the user's own console (NEVER
  commit or copy into the repo):
  ~/Documents/Programming/Recomp/PS4-HB/dumps/games/CUSA01130/mnt/sandbox/pfsmnt/CUSA01130-app0/
  (eboot.bin, sce_module/*.prx, arc/*.psarc, sce_sys/). NID lists and
  names for it are in PS4-HB/inventory/ (local only, never commit).
- Firmware names for NIDs (PS4-HB/refs/shadps4/names.txt) come from a
  GPL project: a local reference only, never copied here or into the
  library.

## Build
- `cmake --preset release && cmake --build --preset release`
  (the PS4 platform module is turned on by this project's CMakeLists).
- Lifting is a separate step; re-run it after any change to the
  library's tools/lifter_x86 or platforms/orbis/lifter, a manifest/<module>.manifest
  (none needed now) or the binaries, then re-run cmake
  (the generated file list is globbed):
  `tools/lift.sh "$HOME/Documents/Programming/Recomp/PS4-HB/dumps/games/CUSA01130/mnt/sandbox/pfsmnt/CUSA01130-app0"`
  It lifts the eboot and every PRX in sce_module (those it starts with
  and those it could load itself), at the bases SwARc's PS4 platform
  maps them at (`out/build/release/bin/orbis_app modules
  <app0>` lists them; `orbis_app imports <app0>` shows the imports the
  HLE does not handle yet). generated/lift_report.txt lists
  unimplemented instructions and warnings.
  It needs a build first (it runs orbis_app).
- Run: `out/build/release/bin/grr --game-dir <app0>`; the dump's
  param.sfo is not in app0, so add `--param-sfo
  <dump>/system_data/priv/appmeta/CUSA01130/param.sfo`. GRR_TRACE=1 logs
  every HLE call; GRR_NID_NAMES=<names file> names unimplemented imports
  by name instead of NID; GRR_BACKGROUND=1 for automated runs, which go
  inside a memory cap:
  `systemd-run --user --scope --quiet -p MemoryMax=6G -p MemorySwapMax=0 <command>`.
