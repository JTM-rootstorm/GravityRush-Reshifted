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

- 2026-10-31 (later): compute image stores run. The game's one image
  compute pass writes an 8x8 and a 1x1 RG32F (tile mode 14), which its
  draws then sample (an exposure-like reduction): the attract scene is
  lit differently with it. Its stores' T#s set word 1 bit 6 (a cache
  policy) above the 38-bit base address, the samples' T#s do not.
  The other zero 2D-tiled textures (128x64 RGB10A2 at 0x14a290000 is
  also a colour target the game renders; the 1920x1080 R8 depth-tiled
  one at 0x145720000 is not a target the renderer tracks) remain.
  One of three runs stopped after "TITLE: first" with the loading
  spinner turning (the movie never initialised); see below.
- 2026-10-31: the stop after "TITLE: first". Was about 1 run in 15
  (GPU-free); after the fiber fixes below, 1 stop in about 135 runs.
  Any tracing (GRR_TRACE, libSceFios2's own trace, printed fiber or
  condition-variable logs) hides it; silent in-memory records dumped
  once a stop is detected do not.
  - Nothing is blocked in the HLE: flips, the main loop's frame
    semaphore and every waiter on libSceFios2's condition variables are
    as they should be. At a stop each Fios2 waiter's last wait follows
    the last broadcast (no wakeup lost in the HLE).
  - The game's fiber job system (0x400D90-0x4019xx): two workers (index
    0 and 1), each with a double-buffered list, a semaphore and a cursor
    (table at 0x21788E0). sub_400F50(task, k) queues on worker k and
    signals only when its list was empty; sub_400FD0(k) yields by
    switching to the next fiber in worker k's list, or back to the
    thread after the last. Waits re-queue themselves; async file reads
    (sub_405510, sub_4050A0, via sub_5FCFC0 / sub_5FCEE0) are re-queued
    by the libSceFios2 completion callback (sub_405240). The worker
    takes a failed sceFiberRun for a finished fiber and finalizes it.
  - Fibers move between the two worker threads. Fixed in x86recomp: a
    moved fiber used the first thread's registers and TLS (crash or
    corruption), and a fiber still switching away was refused as
    running, which made the worker drop it (seen in passing runs).
  - The remaining stop: two reads of arc0/lua/Game/Param/Init.lua are
    accepted by libSceFios2 and get their chunks (root linked under
    chunk 0), then nothing is scheduled; the scheduler woke at the last
    broadcast, found nothing eligible and slept. The fiber waiting for
    the read never resumes, nor do the title's script fibers.
  - Next lead: a silent post-mortem of libSceFios2's state at a stop
    (scheduler list at 0x800064808: queue at +208, count +16600,
    executing +16656; chunk table at [0x80004C408], 16 bytes a chunk;
    suspend count 0x800059808). The dump code was written; no stop came
    in 75 runs with it. A suspect of its own: lock-free hand-offs in
    lifted code are plain loads and stores the C++ compiler may reorder
    (x86recomp ARCHITECTURE.md, "Memory ordering", still open).

## Next
- The GPU: ds_swizzle, the scene's framing, the 1920x1080 R8 texture's
  writer, the lavapipe fault.
- The rare stop after "TITLE: first": libSceFios2's scheduler state at
  a stop (see above).
- Values the HLE reports that are not measured on the console yet:
  x86recomp platforms/orbis/docs/hle.md.
