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
  libSceS3DConversion and libSceSmart (camera/VR/3D) are not, and the
  game never loads them (no sceKernelLoadStartModule import). Since
  2026-11-04 SwARc's lift script lifts them too, from 0x820000000 up:
  4,258 more functions, 322K instructions, 29 more files. libc and
  Fios2 use general-dynamic TLS (DTPMOD64).
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
  (SwARc platforms/orbis/docs/hle.md). The game opens 11 fonts from
  memory: 5 CFF OpenType fonts kerned only by GPOS, 2 TrueType fonts
  with 'kern' tables, the rest (CJK, Korean, its own symbols) without
  kerning; it renders at 48 px into 1-byte surfaces (512x128, 128x256).
  Its renderer edition (SDK 2.00.121) and compiled SDK (0x02000071) put
  glyphs a row lower (libSceFont 11.00 does so for SDKs 0.93 to 2.50).
  No effects (weight, slant) used.

- 2026-10-26 (later): the GPU draws (SwARc orbis/gpu). The game's
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
  read, from the console's tile mode table (SwARc orbis/gpu/tiling).
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
  (GPU-free); after the fiber fixes below, 1 stop in about 135 runs
  (about 1 in 50 with four runs at once); after the libSceFios2 fix
  below, none in 600 runs (four at once).
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
  - Fibers move between the two worker threads. Fixed in SwARc: a
    moved fiber used the first thread's registers and TLS (crash or
    corruption), and a fiber still switching away was refused as
    running, which made the worker drop it (seen in passing runs).
  - The remaining stop was a race in libSceFios2 itself, fixed by
    replacements of its three scheduler kicks, whose comment has the
    mechanism (src/fios2_kick.cpp here at first; since 2026-11-04 in
    SwARc, platforms/orbis/src/orbis/fios2.cpp, for this build's
    SHA-256). A kick (an I/O completion, a psarc
    buffer release, a resume) does nothing to a scheduler with an empty
    queue, and the queue is empty while its only chunk is executed with
    the scheduler's mutex dropped. A chunk that answered busy there
    (data not read yet, no free psarc buffer) and whose kick came in
    that window stayed skipped: every later chunk on that scheduler
    waited for good. Seen both ways: a read run at once on the caller's
    thread found the psarc pool full while a release came (32-43 reads
    queued on one scheduler, the pool showing a free buffer), and a
    decompression on the default scheduler found its data unread while
    the read's completion came (its 105 chunks skipped). A fix of the
    buffer-release kick alone still stopped (in run 13).
  - How it was caught: four GPU-free runs at once stop about once in 50.
    A watcher process (the game's ancestor, so ptrace scope 1 lets it
    read /proc/PID/mem) waits until libSceFios2's scheduler counters
    stay unchanged for 10 s after "TITLE: first", then dumps the
    arena's non-zero pages and scans each thread's stack for return
    addresses (libgrr_lifted.so keeps its symbols). Nothing in the game
    changes. A silent event ring in the lifted allocator (0x17E00),
    kicks, AddToSchedule (0x29BA0), ExecuteChunk (0x2A3E0) and
    NextEligible (0x2BD30) then showed the interleaving.
  - libSceFios2 layout (at its 0x800000000 base): scheduler list head
    0x800064808 (next +8; mutex +0x10, condition +0x28, queue ring of
    u16 chunk indices +0xD0, its start +0x40D0, count +0x40D8, busySkip
    +0x40E8, kick counter +0x40F0, busy count +0x40F8, executing
    +0x4110, shutdown +0x4115); the default scheduler 0x80004CEE0;
    chunk table at [0x80004C408], 16-byte slots (flags +0, type +2,
    dependents +4, parents +6/+8, owning op +10), psarc buffer pool at
    an op's +64 (base +16, in-use mask +40, deferred +48); suspend count
    0x800059808.

- 2026-10-31 (night): the last untranslated pixel shader runs, and the
  scene fills the screen.
  - Its ds_swizzle_b32s are all quad mode (offsets 0x8000, 0x8055,
    0x80AA: every lane of a quad reads lane 0, 1 or 2), hand-made coarse
    derivatives (lane 1 - lane 0, lane 2 - lane 0) in straight-line code.
  - The borders and offset: the game renders at 1920x1080 into targets
    whose pitch and slice are padded (colour 1920x1088 and 2048x2048,
    depth 2048x1152; a 480x270 one in 512x320) and samples them with
    T#s of the real size, so a whole padded image under normalized
    coordinates shrank each pass by 1920/2048 and 1080/1152 (0.9375).
    Its scissors and viewports are the real size (screen scissor
    1920x1080, viewport scale 960/-540; PA_CL_VTE_CNTL 0x43F, no window
    offset). Fixed in SwARc: such a T# samples a copy of the
    target's top-left corner; the attract scene now fills the screen.
  - The 1920x1080 R8 texture at 0x145720000 (tile mode 0) is the stencil
    of depth target 0x144E20000 (DB_STENCIL_INFO on, DB_STENCIL_READ_BASE
    0x145720000): the game samples its stencil as a texture next to the
    depth (T#: 8-bit UNORM, all four channels from X). Fixed in
    SwARc: the stencil is resolved into an R8 image when sampled.
    It still reads zeros here: only draws with geometry shaders (skipped)
    write it, with REPLACE_OP of 0x80 (DB_DEPTH_CONTROL 0x737,
    DB_STENCIL_CONTROL 0x40, DB_STENCILREFMASK 0x80FFFF80), so they
    mark what they draw for the composite.

- 2026-10-31 (late): the geometry-shader draws run (SwARc: ES and GS
  as compute passes on the rings, then the copy shader).
  - Two ES/GS/copy-shader triples in the attract scene (ES 0x14F042200,
    GS 0x14F043A00, copy 0x14F043800; ES 0x14F055700, GS 0x14F055C00,
    copy 0x14F055B00), both VGT_SHADER_STAGES_EN 0xB0, VGT_GS_MODE
    0x180033 (scenario G, off chip), triangle-list input, triangle-strip
    output, at most 8 vertices (VGT_GS_MAX_VERT_OUT 8; GSVS items 0xA0
    and 0x20 dwords, ESGS items 0xC and 8).
  - The rings are the game's own: its global resource table's V#s at
    +0x20 (ESGS write, swizzled, index stride 64), +0x30 (ESGS read),
    +0x40-0x70 (GSVS write per stream, swizzled, index stride 16, stride
    4 x components x 8) and +0x80 (GSVS read), 4 MiB each, base
    0x142218500 and 0x142618500 with 0x3001 in the V#s' base-high field
    (bits above the GPU's 40-bit addresses).
  - What they draw: the flocks of birds (122 primitives a draw, each GS
    emitting a short triangle strip, up to 4 triangles); they were
    missing before.

- 2026-10-31 (shadows): the composite finds the birds' stencil marks,
  and the shadow cascades are drawn and sampled.
  - Stencil: in a 180 s GPU-free run the composite sampled the stencil
    892 times; bit 7 is set in 3-509 pixels a frame (distant birds) once
    the flocks are in view, and in none before they appear or after the
    scene cuts.
  - Shadows: depth target 0x145990000 (2048x2048, 32-bit float) holds
    four cascades, drawn one slice each (DB_DEPTH_VIEW 0x0, 0x2001,
    0x4002, 0x6003) and sampled together through one 2D array T# (type
    13, slices 0-3) with a depth-compare sampler. All four had gone into
    one image, and the T# read as black. Fixed in SwARc: depth
    targets become 2D arrays of the slices drawn or sampled. The scene
    gains the tree's shadow on its rock and small cast shadows. The same
    shader binds a second array T# at 0x4040000000 (unmapped, read as
    black).

- 2026-10-31 (depth clears): the intro's smears and its fade to black
  were depth targets never cleared.
  - The game clears depth through HTILE: each frame a compute shader
    (0x142218200) fills the HTILE with 0xFFFFFFF0 (every tile's ZMASK 0:
    cleared to DB_DEPTH_CLEAR, 1.0), for the main depth buffer
    0x144E20000 (HTILE 0x145960000, 0x30000 bytes, depth only:
    TILE_STENCIL_DISABLE) and the shadow cascades 0x145990000 (HTILE
    0x149990000, 1 MiB for the four slices). Draws with
    DB_RENDER_CONTROL 0x60 then decompress in place.
  - Without the clear, old depth stayed: the birds' shadows stayed in
    the cascades (the trails behind them), and old depth in the main
    buffer hid each new frame's geometry until the screen went black.
    Modelled in SwARc; the intro now plays to the apple and the
    title prompt.
  - Also fixed: the shadow passes' polygon offset (PA_SU_POLY_OFFSET_*
    at 0x2DE-0x2E3: slope scale 16, i.e. 1.0, offset 1, float depth)
    was read from the wrong registers; draws with CB_COLOR_CONTROL's
    MODE CB_DISABLE wrote colour.
  - The second array T# the shadow shader binds is the cascades'
    descriptor with base word 0x40400000 (3.0f as bits): a placeholder,
    never mapped.
  - Left: the sky and fog show banding (stepped gradients); see
    (banding) below.

- 2026-10-31 (banding): the sky's banding came from two translation
  bugs in the post-processing chain, found by dumping the display buffer
  after each pass of one frame.
  - The chain: the tonemap (fp16 scene 0x143E30000, bloom, stencil mask)
    writes the display buffer (8-bit sRGB, COMP_SWAP ALT); a copy pass
    writes it to 0x14C7F0000 (R8G8B8A8 sRGB) with luma in alpha; FXAA
    reads that back into the display buffer; then the overlay (the fade
    and logos).
  - FXAA's averages use v_add_f32 with output modifier div:2; it was
    translated as a + b * 0.5, so its output was about 1.2x its input:
    the sky clipped and every fourth or fifth code went missing (the
    rings). Fixed in SwARc.
  - The display buffer and the scene and bloom targets are written with
    COMP_SWAP ALT (BGRA in memory) and read through BGRA T#s; red and
    blue were swapped twice, so the copy pass swapped the image and the
    luma weights. Fixed in SwARc (targets read through their swap).
  - 0x14CFE8000 (2048x1152, CB format 8 with ALT_REV) is an alpha-only
    target the scene pass reads; it held the shader's red, not its
    alpha. Fixed in SwARc.
  - After the fixes the sky has no empty codes through the chain and no
    rings at any contrast.

- 2026-11-01: the lavapipe fault (2026-10-26) no longer shows: seven
  runs on lavapipe (four of 150 s, three of 300 s at once) ran to their
  timeouts with no fault, past the attract movie into the demo. It was
  seen before the fiber fixes (a fiber moved between threads ran with
  the other thread's registers and TLS), the likeliest cause; not
  confirmed.
- 2026-11-01 (later): the only T# of a type other than 2D the game
  binds in its first 120 s is the shadow shader's placeholder array
  (0x4040000000, 2048x2048x4, depth tile mode), which is unmapped; it
  read as black already, now without the warning. A clean run on the
  GPU warns of nothing in the renderer.

- 2026-11-01: PlayGo. The installed package (/user/app/CUSA01130/app.pkg
  on the console, read over FTP: entry 0x1001 playgo-chunk.dat and 0x1003
  playgo-manifest.xml) has 4 chunks (0-3), 1 scenario, 1 initial chunk.
  In 75 s the game asked scePlayGoGetLocus about chunks 0, 1 and 2 only,
  so the HLE's lenient bound (ids below 1000) never applies here.

- 2026-11-01 (frame rate): the attract scene's zoom ran at about 5 fps
  and the apple at 16 (RTX 4080, flips counted over 5 s). The GPU
  thread (the command processor and renderer) was the limit; the game's
  own threads were mostly idle. Fixed in SwARc:
  - texture change checks hashed every byte one at a time: 34% of all
    CPU time (now four 64-bit lanes);
  - the geometry-shader passes copied whole 4 MiB rings to the host GPU
    and back for draws of a wave or two (now sized to the draw);
  - each vertex attribute stream (base, +0xC, +0x14, ...) copied the
    same interleaved vertex buffer again (now shared within a draw);
  - shaders were walked and decoded again for every draw (now kept while
    their code words are unchanged).
  Now about 17 fps in the zoom and 50 at the apple. The demo plays in
  frames, so it also ends sooner. What is left:
  - the flocks of birds: about 24 geometry-shader draws a frame, each an
    ES and a GS compute pass that waits for the GPU and reads its rings
    back to build the copy shader's strip indices on the CPU. Skipping
    them gives 29-43 fps in the zoom. Keeping the rings and emits on the
    GPU (indices built there) needs gfx::Device support for buffers a
    dispatch leaves for a later draw;
  - the other compute dispatches (skipping them: the apple at 78 fps)
    are the same round trip;
  - presenting a frame waits until the GPU has finished it (Wayland on
    NVIDIA), and SDL keeps other submissions waiting meanwhile, so the
    GPU thread's work and the GPU's do not overlap. GPU load in the zoom
    is about 38% at 16 fps, about 24 ms of GPU work a frame.
- 2026-11-01 (frame rate, 2): the geometry-shader draws now stay on the
  GPU (SwARc 142ab5a: GPU buffers in gfx::Device, rings and emits
  kept there, strip indices built by a compute program, drawn indirect).
  That alone gave about 19 fps in the zoom: each dispatch still created
  its own transfer buffers, which was most of its cost. Reusing the
  cycled upload buffer and the shared download one: the zoom (from the
  JAPAN Studio logo to the apple) at about 31 fps, the apple at about 60,
  the loading screen at about 300 (was 140). Measured in 1 s windows;
  frames look as before. With the geometry-shader draws skipped the zoom
  was at 28 before the transfer-buffer fix, so they are no longer the
  main cost. What is left in the zoom:
  - static meshes drawn in many parts copy their whole vertex buffer
    (0.2-2 MiB, strides 0x18-0x30) for every part, about 1.3 GB/s of
    draw arena from the host; 40% of it repeats within a flush. Copying
    only the vertices a part's indices reach needs the resolver to know
    a buffer is fetched by vertex index alone;
  - about 170 flushes, 75 render passes and 85 dispatches a frame: every
    dispatch ends the render pass and submits on its own;
  - the present still waits for the GPU (above). The GPU is about 37%
    busy.
- 2026-11-02 (frame rate, 3): measured on the GPU build, the draw arena
  took about 4 GB/s from the host (2.6 in the zoom), 90% of it from
  ranges of 64 KiB or more and 40% repeating within a flush; copying
  only the vertices a part's indices reach would have saved about 42%.
  Instead (SwARc f606770) read-only ranges of 64 KiB or more stay
  in GPU buffers, compared once a submission (as textures are) and
  uploaded when they change; a draw copies them on the GPU, once a
  flush. The host now copies about 0.37 GB/s. The zoom from 36-40 to
  40-45 fps, the apple from about 70 to about 90; frames look as before.
  What is left in the zoom is the flushes, render passes and dispatches
  of (frame rate, 2).
- 2026-11-02 (frame rate, 4): a flat profile (perf record -e
  cpu-clock:u, no call graphs: -g is refused) of the zoom: one thread,
  the GPU thread, is about 77% busy; the game's threads about 12% each.
  On it, the resolution key (every code word, kilobytes per draw, then
  hashed by the program map), 16-bit indices widened one push_back at a
  time, and the arenas zeroing the room GPU buffer copies fill. Fixed in
  SwARc e0cb6c1. Measured A/B in the same runs (another session's
  jobs had the host at load 5-10, so only the quieter runs count): the
  zoom from about 41 to about 50-55 fps, the apple from about 90 to
  about 105-110; frames look as before. Not worth it: keeping
  resolutions per stage registers (15% hits: the user SGPRs carry
  per-draw pointers), a faster content hash (the current one runs at
  23 GB/s, memory-bound; textures are about 2.2 GB/s of it, 9% of the
  GPU thread, once a submission). Left on the GPU thread: Bind (most of
  it texture hashing), Resolver::Step and its maps, DrawProgram.
- 2026-11-02 (frame rate, 5): the GPU thread was never idle, but about a
  quarter of its time went to waiting for the GPU. Two causes. (a) At
  most 4 command buffers in flight against about 180 submissions a
  frame, with the upload staging buffer mapped cycling (each in-flight
  submission held a copy; after loading it was 128 MiB, so a deeper
  queue ran out of the 6 GB cap). Fixed in SwARc eaf5e68: a staging
  ring and 32 in flight (64 was about 1 GB more VRAM for little more).
  (b) Two compute dispatches a frame (262144 groups each) write a depth
  target's whole HTILE (1 MiB at 0x149990000, 192 KiB at 0x145960000),
  each followed by an EOS label and a WAIT_REG_MEM on it, and the
  target's next draw comes straight after: the fast-clear check read the
  HTILE back, draining the GPU queue, 90-240 ms a second. Fixed in
  SwARc dd76ab4: dispatch results land when the GPU is done, labels
  and EOP events wait behind them, a WAIT_REG_MEM on a queued label
  compares the value it will write, and a whole-HTILE write is applied
  on the GPU (an indirect clear draw decided by a program reading it).
  With the clear draw count forced to 0 the zoom renders dark and black,
  so that path is what clears. Measured (load 1-4): the zoom from about
  53 to about 65 fps, the apple from about 110 to about 160; the logo
  about 350. A 120 s run is clean and reaches the ep00-01c movie.
- 2026-11-02 (frame rate, 6): a profile with call stacks (perf record
  -e cpu-clock:u --call-graph dwarf --user-callchains works; plain -g
  is refused) of the GPU thread in the zoom. Fixed:
  - textures and kept ranges were hashed in every submission that used
    them, 3-8 GB/s of hashing, a third of the thread in the apple. Now
    (SwARc 846d37f, da1ac9c) a range found unchanged twice is
    write-watched (its host pages read-only until written) and not
    hashed again until a write: after loading nothing is hashed;
  - Resolve went through a map of every instruction per draw; the walk
    now keeps the instructions the scalar evaluation acts on (3024d5c).
    16-bit indices widened in a scalar loop (the count was reloaded on
    each store), 14% of the thread (same commit);
  - every flush and dispatch was its own command buffer and submission,
    a third of the thread in the driver. Now one command buffer takes
    16 of them (0045184; 64 was no faster in the zoom, slower at the
    logo). Fewer allocations per packet and key (68a675d).
  Not worth it: remembering resolutions by the stage's registers and the
  words its scalar loads read (12% hits: per-draw pointers in the user
  SGPRs, as in (frame rate, 4)); it was slower. Measured A/B against
  dd76ab4 on a quiet host: the zoom from about 66 to about 117 fps, the
  logo from about 355 to about 423, the apple from about 165 to a flat
  186 (likely no longer the CPU). Frames match the previous build's at
  the same frame numbers but for timing. A 120 s run is clean.
- 2026-11-02 (frame rate, 7): counted per second in the zoom, every
  indexed draw copied its indices (widened to 32 bits) into the upload:
  1.5 GB/s, from only about 490 index ranges, each drawn over and over.
  Index ranges are now kept like large ranges and drawn from the kept
  GPU buffer (SwARc c18c02e). Then the apple was GPU-bound, not the
  CPU: the GPU thread waited on fences about half of each second, the
  GPU read 93% busy at 125 W of 320. Kept buffers were copied into the
  draw arena on the GPU at each flush (about 105 copies a frame, 20 GB/s)
  and SDL_GPU puts pipeline barriers around every buffer copy. GPU
  buffers now live in one heap the shaders read in place (3c976e2).
  Keeping ranges from 4 KiB instead of 64 KiB was slower (108 against
  142 fps in the zoom): each kept buffer then cost copies per flush.
  Measured A/B, steps in order: the zoom about 117, 142, 160 fps, the
  apple 186, 225, 272. The logo lost about 5% (424 to about 395): GPU
  time, likely the dispatches that write GPU buffers (8 a frame there),
  whose copies into the heap now wait on shader reads of the whole heap.
  Frames match but for timing; a 120 s run is clean.

- 2026-11-02 (into the game): runs now reach episode 1's city.
  - Boot options: main() (0x4003D0) passes argc/argv to a parser
    (0x401C10) after merging app0/args.txt (-region A) in. Among its
    options: -state NAME (the start state, default "Ready", copied from
    the next argument without skipping it), -world NAME (state "Game",
    world NAME; default "Title"; worlds are arc/ xml/NAME.xml), -viewer,
    -home, -nosound, -30/-60. grr passes arguments after -- to it.
    -world title -state Ready boots as usual; -world ep01p (with or
    without -state Ready) and -world ep00_00p draw black at about
    1000 fps with no script output: episode worlds need what the
    title's new game sets up (the Lua in lua/game/world/title/).
  - So runs walk the title instead: SwARc's TEST_PAD (a file of
    timed presses for background runs), cross every 2 s from 10 s.
    That selects New Game, plays the opening movie (several minutes,
    the credits over the apple; the world loads behind it) and reaches
    "pc is in cc" in about 9 minutes.
  - Fixed on the way (SwARc): a 34-case switch at 0x1145440 whose
    bounds check was 17 instructions before its jmp, past the lifter's
    16 (27 more tables recovered, 823 to 850);
    sceKernelCheckedReleaseDirectMemory (the movie allocator's texture
    free, 0x659F50); sceAvPlayerClose read its handle after calling the
    game's deallocators and freed the last texture a second time (the
    game's deallocator 0x659D00 then faulted at 0x1C looking it up).
  - Stops at: libSceNgs2 ATRAC9 waveforms (not implemented, fatal), on
    a guest thread just after "pc is in cc".

- 2026-11-03 (past ATRAC9)
  - SwARc's NGS2 now decodes ATRAC9 (ffmpeg); the city's voices open
    the decoder and decode without errors (no audio check by ear yet:
    runs use SDL's dummy or disk driver).
  - Next stop was a GLSL compile error: a compare sample through a
    sampler whose compare function is NEVER (fixed in SwARc: reads 0).
  - Stops at: a write to guest 0 in 0x107C1D0 when the first dialogue
    scene starts ("@ KaiwaDemo sandbox OnSandboxStart()", "cleartrophy
    = 0"), on a job worker (0x8A80B0). Its marker record (TLS, offset
    at 0x21771E0) is all zeros for workers with no marker buffer
    ([obj+0x33C] = 0, so 0x8BB700 never allocates one), and the code
    skips the marker when cursor (+8) >= end (+0x18). The lifted code
    lost that branch: its jnb target 0x107C570 is a spin on a byte
    (mov al, [r12+0x30]; cmp al, 2; jnz) entered only when the byte was
    not 2 (checked at 0x107C526), so clang, assuming loops of plain
    loads terminate, dropped the path into it and stores the marker
    unconditionally. The lift has 4 such loops (loads at a fixed
    address, compares, a branch back): 0x411890, 0x411DB0, 0x107C570,
    0x10A0730. Needs the lifter to emit their loads as atomic ones.
  - Fixed (SwARc 47d71ab): the lifter finds such loops by shape and
    loads with acquire atomics (RdPoll); the lift changes at exactly
    those 4 loops.
  - Runs now get through the dialogue scene into free play in the city
    (Kat by the fountain, the camera behind her) and keep running: 18
    minutes, no fatal, about 180 fps there. Kat's own model draws wrong
    (patchy, wrong colours, pieces missing) while the city around her
    looks right: the next GPU lead.

- 2026-11-03 (characters through the geometry shader)
  - Kat and the cat (and the other skinned characters) draw through ES,
    GS and copy shader (VGT_SHADER_STAGES_EN 0xB0): the ES skins (four
    bones, three float4 rows a bone), the GS emits the triangle and, for
    draws whose user data asks, outline fins (strips of up to 8
    vertices). No vertex shader reads bone indices; skinning is not on
    the CPU.
  - Kat's draw's ESGS ring read back from the GPU held a clean figure;
    the damage came after it. Its GSVS V# swizzles with an index stride
    of 16 and a 128-byte stride (4 components of 8 vertices): the
    translation's swizzled address multiplied the stride by the element
    size too, so lanes 16-63 of each GS wave wrote over the next waves'
    vertices. Fixed in SwARc 1f16d4c (after a376ad9: compute lanes for
    ADD_TID_ENABLE, which its test needs).
  - A run now shows Kat and the cat whole, the cat with its outline,
    about 200 fps in the city.
  - Not causes (tried): kept GPU ranges (a run with none drew the same);
    landing dispatch results before a kept range is compared. That
    landing was missing all the same (a kept range already made was
    compared without the dispatch results queued there): fixed in
    SwARc 8765250, no visible difference in a run to free play.

- 2026-11-03 (a save from play)
  - The first autosave is at mission ep01_b: ep01_a (Kat's walk from the
    start to the old man's door, no jumping or gravity) skips it by name
    (lua/game/event/autosave.lua). A run now plays ep01_a: SwARc's
    TEST_PAD holds sticks and reads lines appended during the run, and a
    driver outside the game steers Kat by her position in memory (the
    float triple that starts at checkpoint_pc_start, 333.06, -102.15,
    -55.12; y is up), learning the stick's angle as the camera changes.
  - The route, from the mission's sensors (lua/game/event/ep01_a.evb:
    records of rotation, position, scale, box size at +0xB0, and the
    next record's name at +0xE8): the fountain stairs, then a spiral up
    the courtyard's walkways, (x, z) = (336, -64), (335, -68), (335, -76),
    (343.5, -66), (345.5, -60), (345, -53), (344, -49), (340, -46),
    (335.4, -45.3), (329, -47), (327.7, -52.5), (327.7, -62),
    (327.7, -70.9), (330.6, -70.8), (336, -72), (339.6, -72), then
    ojin_entry at (343, -78), y -92. Then ep01_01p and the comic ep01_02c
    (cross), and ep01_b autosaves: savedata/1/SAVEAUTO (data.bin, 18110
    bytes), kept under out/saves/ep01b.
  - Fixed on the way (SwARc): SSE flush-to-zero (the game runs with FTZ;
    walking reached a tiny result at 0x109A990), and setjmp/longjmp in
    x64 images: the script engine's protected calls (setjmp at 0x115E5E4,
    luaD_throw's longjmp at 0x115E4A4) need setjmp's PLT stub marked as
    setjmp and libc's longjmp ret as a resume site (manifest lines at
    first; since 2026-11-04 the platform's lifter reader marks them by
    NID).
  - Continue from that save: the title finds it and selects Continue;
    Savedata:load() opens the system save data dialog as a list for
    loading (mode 1, type 2; Open from 0x5FC3D5, after a
    sceSaveDataDirNameSearch), polls UpdateStatus and takes the pick
    (GetResult from 0x5FB8E9), then opens the progress message (mode 3,
    system message 5; Open from 0x5FB638, IsReadyToDisplay), which the
    load thread closes once it has mounted and read the save (Close from
    0x4041FA); then Terminate. The other opener (0x5FAE00) is a system
    message (mode 3, type 8) with a block count, not reached yet.
  - 2026-11-03: SwARc's libSceSaveDataDialog (the list and the progress
    message, drawn by the runtime over the frames) does all that: Continue
    loads the ep01_b autosave into the "cc" area at (340.31, -92.07,
    -82.99), the autosave's position, and backing out of the list
    (circle) returns to the title menu. grr links
    recomp_orbis_savedata_dialog. A scripted run: cross until the title
    has logged "TITLE: savedata check" four times, one more cross (the
    list opens), then cross again to pick.
- 2026-11-04: texture levels. The game samples an R32F chain
  (0x14A2C0000, 1920x1080 in a pitch of 2048, tile mode 14, POW2_PAD)
  through one T# of levels 0-3, the levels rendered as targets of their
  own where the layout puts them (level 1, 1024x1024, at +16 MiB: level
  0 padded to 2048x2048). SwARc now composes such a T# from the targets
  (before, it sampled level 0 alone); textures from memory now upload
  every level. Continue reaches play with no GPU fatal and no visible
  change but the levels.

- 2026-11-04 (a load fault; city shadows)
  - Intermittent fatal while Continue loads ep01_b (2 of about 9 runs,
    both in builds slowed by logging or gdb): "guest memory fault" in
    libc's memcpy (0x8100302B0) from 0x411260, copying a GNF texture
    ("GNF ", 0x41BD20) into the game's GPU heap (sub_415420/0x65E5B0 over
    the 3 GiB mapping, 0x140A30000-0x200A30000 in these runs). The page
    was mapped; the renderer had a write watch on it (an old texture's
    memory), and another thread disarmed it between the fault and its
    handling, so the handler found no watch. Fixed in SwARc 182723c
    (watch::Reopened, with a test that failed every run before).
  - Shadows: the city draws none. A frame in the "cc" area has no draw
    into the cascades (0x145990000, nor any other shadow-shaped target,
    zero-count draws included); the attract scene draws about 950 there a
    window, so the cascade path works. Game side, per frame in the city:
    the shadow setup runs (0x426AD0 -> 0x427AD0 -> 0x43CAD0, Method 2 =
    cascades, the debug menu's Shadow/Method at 0x19E82CC) and the shadow
    view collects casters (0x66A710 -> 0x66CAC0 -> 0x6696C0 -> 0x461E90:
    materials with flag 0x10, kind 47, cascade mask 1 at view+0x1B4,
    batched as instances by 0x1258A30); the pass that would draw those
    batches is never recorded. Not the cause: the depth-reduction chain
    (0x14A2C0000...) read on the CPU (filling it changed nothing), the
    scene file (area_post_cc.xml has DirLight on and the title's Shadow
    block), EOP timestamps (real clocks). Next: the consumer of the
    batches (who reads the batch's mask at +0x78 and records the shadow
    view), traced from the attract scene, where it runs.

## Next
- Values the HLE reports that are not measured on the console yet:
  SwARc platforms/orbis/docs/hle.md. PS4-HB payloads/hlevals
  and plugins/hlevals (a GoldHEN plugin in this game's process) measured
  all but the user's name, the handles' values, the flip data's low 12
  bits and PlayGo's install speed and past-count reply (2026-11-01, PS4-HB
  FINDINGS: HLE VALUES, HLE VALUES IN A GAME). In this game's process the
  default thread stack is 64 KiB (not 2 MiB) and direct memory 4.5 GiB;
  the HLE follows. A 90 s GPU run with them is clean and still finds its
  save memory in savedata/1.
- Frame rate: the GPU thread is still the limit in the zoom (about 1700
  draws a frame, at about 160 fps). Left on it: DrawProgram's recording
  (about a quarter), Resolve (the scalar evaluation per draw, about a
  fifth), Bind, the small buffers (under 64 KiB) still copied per draw
  (about 0.85 GB/s, 98% of their bytes unchanged since their last
  draw), the program key string per draw. Moving the device's recording
  and submission to a thread of its own would take the driver's share
  off it. The apple (about 272 fps) waits on the GPU: about 45 flushes a
  frame, each a copy pass then a render pass; the GPU writes into the
  heap (dispatches) serialise with its reads. See (frame rate, 2) to
  (frame rate, 7) above.
- Into the game: a run reaches free play with GRR_TEST_PAD pressing
  cross every 2 s from 10 s, in about 9 minutes (see (into the game)
  above). A save would let later runs start from Continue, but no run
  makes one: the first mission (ep01_a) starts the autosave sandbox
  ("autosave create!!!") and its finalize skips ep01_a by name
  (lua/game/event/autosave.lua); the first autosave is at the next
  mission, past episode 1's play: a run now makes one (see (a save from
  play) above), and Continue loads it (2026-11-03).
