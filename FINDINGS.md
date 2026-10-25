# Findings and status

## Binaries (CUSA01130, version 01.00, no update applied)
- eboot.bin: ET_SCE_DYNEXEC (position independent, mapped at 0x400000),
  module "launcher", SDK 2.00 (process param 0x2000071). One executable
  segment of 22.9 MB holding .text, the PLT, .rodata and .eh_frame;
  52,238 FDEs, of which 18,642 describe code the linker discarded
  (pc_begin 0), so 33,596 functions. 3.56M instructions, 687 switch
  tables recovered. PT_TLS 0x140 bytes. 681 imports from 42 modules.
- Bundled PRXs in sce_module/: libc and libSceFios2 are needed at start
  (libc needs Fios2); libSceFace, libSceHand, libSceHeadTracker,
  libSceS3DConversion and libSceSmart (camera/VR/3D) are not, and are
  not lifted yet. libc and Fios2 use general-dynamic TLS (DTPMOD64).
- The process param sets only the libc parameters (no process name,
  main thread name, priority or stack size).

## Status
- 2026-10-24: lifts (eboot + Fios2 + libc: 40,881 functions, 3.75M
  instructions; nothing unimplemented in a function .eh_frame describes
  but dead bytes after two calls that never return) and builds (a 102 MB
  executable). Runs through libc's and Fios2's initialisers and the
  eboot's _start into the game's own startup:
  - libc: sceKernelGetProcParam, _sceKernelRtldSetApplicationHeapAPI,
    its mutexes;
  - the game maps 1 GiB, then 3 GiB and 352 MiB of direct memory
    (4.34 GiB of the 5 GiB pool), creates a worker thread (which polls
    with sceKernelUsleep(1000)) and semaphores, reads /app0/args.txt
    through Fios2, reads the system language, loads the screenshot
    system module (0x9C) and disables screenshots;
  - and stops at sceVideoOutOpen: the display, and with it the GPU path
    (VideoOut, then GnmDriver's command buffers), are next.

- 2026-10-25: past the display and into the sound engine's start-up:
  - VideoOut: opens the main port and registers three 1920x1080
    A8R8G8B8 (sRGB) buffers, tiled (tiling mode 0), flip rate 60 Hz, a
    flip event on an event queue;
  - starts its job threads, lists a directory of /app0, initialises
    PlayGo and reads its data through Fios2, which asks libSceDiscMap
    where each read lies (sce_discmap.plt: 308 entries, 32 MiB blocks,
    all 18 files of /app0);
  - creates fibers;
  - stops at libSceNgs2!sceNgs2SystemCreateWithAllocator (the sound
    engine "sndx", built 2015-01-09, has loaded NGS2).

## Next
- libSceNgs2 (sound), then the rest of the start-up's libraries as the
  run reaches them.
- The GPU front end on gfx::Device (draws, GCN shaders, tiled surfaces:
  the display buffers are tiled).
- Values the HLE reports that are not measured on the console yet:
  x86recomp platforms/orbis/docs/hle.md.
