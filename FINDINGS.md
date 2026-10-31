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

- 2026-10-25 (later): through the sound engine's start-up (NGS2:
  mastering, submixer, a 256-voice sampler and a reverb rack; 48 kHz,
  256-sample grains), PSN (signed out: "WebAPI: User is not signed
  in."), networking (no connection), save data, fonts, the pad and
  trophies into the title flow: "TITLE: EULA OK", "savedata check",
  then the attract movie mp4/ep00-01c.mp4 through AvPlayer (16 s,
  "DEMO BEGIN" to "DEMO END"), and runs its frame loop (GPU
  submissions with draws and compute dispatches, flips at 60 Hz,
  audio out). NGS2 is answered by a local, uncommitted probe that logs
  the game's use and renders silence, until its HLE exists.

- 2026-10-25 (night): sound through the NGS2 HLE. The game's use, from
  the probe:
  - racks: mastering (2 voices, 8 channels), submixer (1 voice, 8
    channels in and out), reverb (1 voice, 8 in / 8 out, I3DL2: room
    -100, room HF -1000, decay 3.5 s at HF ratio 0.83, reflections -2602
    after 7 ms, reverb +350 after 20 ms, HF reference 3700 Hz; wet
    only), a 256-voice sampler (16 waveform blocks, 4 envelope points, 8
    filters);
  - graph: sampler voices -> submixer -> mastering voice 0; reverb ->
    submixer; mastering gain 0.5, limiter at 1.0;
  - sampler voices: one-shot 24-bit PCM, stereo, 48 kHz (one block of
    dataSize/6 frames, then a terminating block call), no envelope
    points, filter off, pitch 1.0, per-voice matrix levels; the game
    reads each voice's 0x30-byte state;
  - SystemRender into 10 float buffers of 256 frames: 8 channels, 4
    mono, 4 stereo, 8 channels; the main mix is buffer 0.
  A 75 s run on lavapipe (no GPU): no fatal error; buffer 0 carries the
  title's sound (2 to 9 sampler voices, fading in from silence, peaks
  near -26 dBFS); the other buffers stay silent. The loudness is not
  checked against the console yet.

- 2026-10-26: fonts follow libSceFontFt 11.00's use of FreeType
  (x86recomp platforms/orbis/docs/hle.md). The game opens 11 fonts from
  memory: 5 CFF OpenType fonts kerned only by GPOS, 2 TrueType fonts
  with 'kern' tables, the rest (CJK, Korean, its own symbols) without
  kerning; it renders at 48 px into 1-byte surfaces (512x128, 128x256).
  Its renderer edition (SDK 2.00.121) and compiled SDK (0x02000071) put
  glyphs a row lower (libSceFont 11.00 does so for SDKs 0.93 to 2.50).
  No effects (weight, slant) used.

- 2026-10-26 (later): the GPU draws (x86recomp orbis/gpu). The game's
  shaders: 89 (31 VS, 51 PS, 3 CS, 2 ES + 2 GS), 96 distinct GCN
  opcodes; every VS/PS/CS variant translates but one PS using
  ds_swizzle_b32. Draws: DRAW_INDEX_OFFSET_2 and DRAW_INDEX_AUTO, triangle
  lists and strips, many depth-only (shadow) passes; colour targets
  RGBA16F, RGB10A2, RGBA8 (sRGB too), R32F, R8; one compute dispatch of
  262144 groups of 64. Textures are mostly tile mode 13 (1D thin, BC1/BC3
  and RGBA8, mip chains), some 14 (2D thin) and one 0 (depth). It submits
  with sceGnmSubmitCommandBuffers and flips from the CPU after waiting
  for the previous flip, so a lagging GPU let it reuse command memory
  (fixed: CPU-side flips wait for the GPU). It relies on CLEAR_STATE for
  its scissors and uses OpenGL clip space (PA_CL_CLIP_CNTL 0).
  A 170 s run on lavapipe: the logos, the attract movie's scene and the
  title prompt ("Press X to tap the Apple") draw; the 3D scene is
  framed wrongly (borders, offset) and 2D-tiled textures are scrambled.
  An intermittent host fault inside lavapipe (a memcpy past a mapping)
  is not understood yet.

- 2026-10-31: 2D-tiled (and depth- and displayable-tiled) textures are
  read, from the console's tile mode table (x86recomp orbis/gpu/tiling).
  The 2D-tiled textures the game samples in its first 300 s (mode 14:
  128x64 and 256x128 RGB10A2, 8x8 RG32F; mode 0: 1920x1080 R8) are all
  zeros in guest memory: they are written by the GPU, not uploaded, so
  what looked scrambled was not texel data (compute image stores and
  targets the renderer does not track are the likely writers).

## Next
- The GPU: compute image stores (the 2D-tiled textures' writers),
  ds_swizzle, the scene's framing, the lavapipe fault.
- Values the HLE reports that are not measured on the console yet:
  x86recomp platforms/orbis/docs/hle.md.
