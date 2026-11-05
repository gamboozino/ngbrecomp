# A Direct3D-level renderer (replacing xemu's NV2A emulation)

Goal: draw Ninja Gaiden Black by implementing the Xbox Direct3D 8 API (XDK 5849, the
link-time-optimised D3D8LTCG build) on Vulkan, as SCDA-XBERecomp does for Splinter Cell:
Double Agent on Direct3D 12 (github.com/TGP482/SCDA-XBERecomp). The game's Direct3D calls never become NV2A push buffers; nothing emulates the
GPU. It is the default; `NGB_RENDERER=xemu` selects xemu's renderer.

## What NGB looks like (measured)

- Direct3D lives in the XBE's `D3D` section (0x3C5260-0x3D95E0) plus a few functions LTCG
  moved into `.text` (InsertFence 0x1A0AB2). XbSymbolDatabase names 97 of them, with their
  LTCG register conventions in the name (`_4__LTCG_eax1_ebx2`: 4 bytes of stack, argument 1
  in eax, 2 in ebx).
- Game code calls them (329 calls to SetRenderState_Simple, 51 to SetPixelShader, ...).
  141 game functions outside the library touch Direct3D's globals, inlined by LTCG, but only
  to update state in memory (SetTransform's matrix copy into the device at +0xDE0 and a dirty
  bit at 0x3D83D0); no game code writes the push buffer. So the renderer can read inlined
  state from the device at draw time and hook only the functions that emit GPU work.
- Push buffer pointer and limit: 0x3D5F20 / 0x3D5F24 (the static CDevice).

## Reused from SCDA

- `gpu/shaders.cpp`: vertex programs, fixed function and the pixel combiners to HLSL. Its
  output compiles to SPIR-V with glslang's HLSL front end unmodified (tested).
- `gpu/vertex.cpp`, `gpu/texture.cpp`: vertex formats, swizzling, texture decoding.
- `xdk/d3d8.cpp`: which API entry points to implement and how (the state model).
- New: a Vulkan backend in place of `gpu/d3d12.cpp` (window and swapchain from present_sdl).

## Milestones

1. Hooks: every Direct3D entry point moved out of the generated code behind a wrapper that
   calls the recompiled body unless the new renderer is on. NGB_D3D_TRACE logs the calls with
   their decoded arguments, to check the conventions against what the game passes.
2. Backend skeleton: Vulkan device on the game window, Clear and Swap. First picture: the
   clear colours of the boot screens.
3. Draws: streams, vertex shaders and fixed function, textures, render targets. Goal: the
   title screen and menus.
4. Pixel shaders, fog, alpha test, blending, stencil, shadows, CopyRects, render to texture.
   Goal: chapter 1 gameplay.
5. Parity: every scene, the quality settings (render scale, filtering, shadow resolution),
   the recomp's menus, then the switch of default.

## Status (2026-10-12)

- `NGB_RENDERER=d3d`: the renderer draws to the window. It shares xemu's Vulkan device
  (`xboxrecomp/src/nv2a_vk/host/nv2a_vk_share.h`: the device, a lock around queue submissions,
  and `nv2a_present_set_external`, a picture the presenter shows in place of its own). xemu's
  renderer keeps executing the push buffer for the fences and flips but its draws, clears,
  blits and surface traffic are off (`nv2a_vk_set_draws_off`; `NGB_D3D_KEEP_NV2A=1` keeps
  them, unseen). Each Swap copies the front buffer to one of three images handed to the
  presenter, so the title's next frame never races the blit.
- The swap turns the back buffer's surface object to the other buffer's memory, so the render
  target is read again from that object after each Swap. The frame's first clear comes before
  any SetRenderTarget: with the address from before the swap it cleared the picture just shown,
  which a camera cut's cross-fade then copied (the whole screen the fog colour for a frame) and
  the menus' feedback backdrop (each frame the last one with the backdrop at 24% over it) drew
  over black.
- The hooks run as runtime host sections (`compat_host_enter/leave`): a guest thread parked
  by the scheduler while holding the queue lock stalled the PFIFO thread for good.
- Quality settings come from the presenter (`nv2a_quality_get`): display-sized targets render
  at the render scale, small depth-only targets (the shadow map) at the shadow scale, other
  small targets (the scarf simulation) 1:1; smooth filtering is linear/anisotropic/trilinear.
- Pixel shaders are read from the handle at SetPixelShader (the object holds the definition at
  +0xC): most were created before the hooks saw a CreatePixelShader, and without them the
  shadow pass drew with the fixed function stages and blackened the scene.
- Shadow comparison: the console tests texel OP reference, Vulkan reference OP texel; the
  function is swapped (`swap[8] = {0, 4, 2, 6, 1, 5, 3, 7}`).
- CPU: about 4 cores busy in gameplay against 3.3 for xemu alone (mirror mode, both drawing:
  8.7). Draws are recorded and submitted on the title's thread, one submission a frame.

Next: more scenes and chapters, Windows (the Proton build), Steam Deck performance, the
debug knobs out once it is the default.

- Default since 2026-10-12 (`NGB_RENDERER` unset means `d3d`; `xemu` for the old renderer).
- Fixed-function vertex blending (D3DRS_VERTEXBLEND, the NV2A skin modes 1-6): the characters
  are skinned with it, position and normal over world matrices 0-3 weighted by v1 (modes 1, 3,
  5 give one weight fewer than matrices, the last 1 - the others). Without it every vertex took
  world 0 and the joints (knees, shoulders, hips) came apart into holes.
- Pixel centres: the console's are at whole coordinates, Vulkan's at halves. Everything is drawn half a
  console pixel right and down (the vertex epilogue for programs and screen-space vertices, the viewport
  matrix for fixed function), as xemu has it: menu pieces meeting at x.5 no longer leave a hairline gap
  (a red line through the pause menu's blade at 8x), and frames line up with xemu's to the pixel. The
  SCDA full screen quad stretch (and its texture coordinate correction) is gone with it.
- At render scale 8 in chapter 1: GPU busy about 24% against xemu's 51%; CPU about 4.2 cores against
  3.4 (the hooks, the per-draw state copy and hashing every bound texture's bytes each draw are the
  likely costs to cut next).
- A setting change (render scale, MSAA) keeps each display-sized target's picture: the old image is
  blitted into the new one, and an MSAA image is refilled from it by a full screen draw before the next
  pass (FillMs; also after a copy into an MSAA target). The pause menu's still of the game no longer goes
  black until the game draws it again. `NGB_D3D_QUALITY_FLIP=n` alternates MSAA and scale every n s
  (testing).
- V8U8 bump maps (format 0x28/0x17, G8B8's codes), when the next stage bump-maps from them (texture mode 6
  or 7), are decoded in the shader texel by texel (`bumpduv%d`: Load, signed bytes, then bilinear within the
  level and linear on to the next) -- filtered as the unsigned bytes they are stored as, texels either side
  of zero blend through the wrong range. A bump map is never read from a render target at its address.
- The LOD bias (D3DTSS_MIPMAPLODBIAS) is converted as the console's D3D does, into the GPU's signed 5.8
  field (-16 to just under 16), then clamped to maxSamplerLodBias (LodBias in backend.cpp). A NaN becomes
  0x1000, -16, as xemu's register model shows (TEXFILTER0 E2063000 for the pond): the chapter 2 pond's
  ripple maps have FFFFFFFF, and the game fills only their top level -- the other two hold leftover data.
  Passed to Vulkan as a NaN, AMD took 0 and Nvidia other levels: the water was banded on Nvidia only;
  read with a bias of 0, the far water picked up the leftovers (#16). The bump decode chooses its own
  level, so it adds the stage's bias and most detailed level to the one it computes (bumplum.zw).
- Fog: D3DRS_FOGTABLEMODE 1-3 compute the factor from the depth or a vertex program's oFog.x; with fog on
  and table mode none, a vertex program's oFog.x is the factor itself (the hardware's linear fog with
  parameters 1 and 1, in xemu) -- the water's own haze, the chapter 7 pool's blue (fogfactor's mode 4).
- Anisotropic filtering on: single-level opaque textures get a generated mip chain (2:1 linear blits),
  DXT1/2/3 ones decoded to BGRA for it (DecodeBC) -- most of the game's world textures have no mipmaps
  and sparkle in the distance otherwise (xemu's smooth mode did this for BGRA only). `NGB_D3D_MIPLOG=1`
  lists single-level textures; `dump_dir/burst.now` dumps the next 40 frames.
- The shader cache (backend.cpp, ShaderCache): recipes.bin (each module's inputs, pipeline keys),
  spirv.bin (SPIR-V by the hash of its HLSL), pipelines.bin (VkPipelineCache) under
  $NV2A_SHADER_CACHE/d3d; a prewarm thread makes them all at startup, plus the shipped seed
  (share/NinjaGaidenBlack/d3d-seed.bin; NGB_D3D_SEED_RECORD=<file> records one, vertex programs as
  default.xbe offsets). First chapter-1 combat went from 24 compile hitches (300 ms in all, up to 83 ms
  in one frame) to none. NGB_D3D_HITCHLOG=1 logs frames held up by compiling. xemu's own pipeline
  prewarm is off while it draws nothing (NV2A_NO_PREWARM). recipes.bin and spirv.bin are appended to as
  the game runs, so a record cut short (the game killed or the disk full while writing) is cut off at the
  next start, with everything after it: read past it, everything recorded later was never known and was
  made late at every start (spirv.bin crashed the driver).
- Two frames in flight (EndFrame): the game records frame N+1 while the GPU draws N; Submit() is the
  full sync for callers that destroy or read back; replaced textures are retired to a graveyard until
  no frame in flight can use them; the presenter waits on the frame with a barrier (same queue). At 10x
  with 8x MSAA (GPU bound): 31 -> 36 fps; 10x with 4x MSAA holds 60.

- Shaders and pipelines a draw needs that nothing has made yet (not in the cache, the seed or the startup
  warm-up) are made at once while the frame has spent less than 4 ms making them; past that they go to a
  thread of their own and the draw is left out until they are, a frame or two, instead of the frame waiting
  for all of them (a hitch: 10-80 ms each on a first sight, dozens when an area brings many). On a cold cache
  into chapter 9 that left out 3 draws instead of 21 (#94), for about 25 ms spent over four frames.
  `NGB_D3D_COMPILE_BUDGET_MS` sets the budget (0: always on the thread), `NGB_D3D_SYNC_COMPILE=1` makes
  every one at once. The pipelines warmed at startup are also made for the anti-aliasing in use
  (and again when it changes): each holds one sample count, and the seed's were recorded without MSAA.
  A pipeline that only adds a depth bias set at the draw (the drawbridge towers, below) is drawn by the
  plain one meanwhile, so it never drops out.

- Textures the game hasn't used for 30 s are retired (EvictTextures), and an image that can't be made for
  lack of memory first retires what the last two frames didn't use (#81): each texture the game streams
  to a new address made an image that stayed for the session. `NGB_D3D_NO_EVICT=1` keeps them (comparing),
  `NGB_D3D_TEXSTATS=1` logs the count and size once a second.

- 2D quads above 1x (QuadSnap, QuadHlsl): an axis-aligned pretransformed 4-vertex quad (corners within a
  hundredth of a pixel: the pause menu's dagger has one at 181.4999) has its edges moved to the console's
  pixel boundaries, its texture coordinates moved with them, and each stage's coordinates clamped to the
  range the console's pixel centres sample -- strips of a full screen fade left uncovered, lines over the
  scrolling 2D backgrounds (#83). Also at 1x with MSAA. Its linearly filtered textures are weighted by their
  alpha (qsnap.y), so a transparent texel's colour doesn't fringe an opaque one, and an on-or-off mask keeps
  the nearest texel's alpha. Left: a faint light line across the pause menu's dagger above 1x, where the
  sheen quad's soft alpha ramps up inside the crossbar quad's last console row (at 1x that row takes half of
  it at once); taking the alpha at the console pixel's centre removes it but steps soft edges and text.
  `NGB_D3D_NO_QUADSNAP=1` turns all of it off.

- The Tairon drawbridge towers (chapter 8): two map sections lay rubble and blocks in one plane, cut into
  different triangles, and their depths tie within a few steps either way -- in xemu's renderer too, the
  game's own data. The towers' meshes (a vertex buffer whose header is in their chunk: "KMD", size 0x4FF470
  at +0xC) are pulled nearer by half a console pixel's worth of depth slope and 2 steps, as a depth bias set
  at the draw (the pipeline key's has_ds bit 2), so every pass over the same mesh -- the depth pre-pass,
  Ryu's shadow laid over it -- moves with it. Chosen by texture instead, the shadow pass wasn't and broke
  up; a rule for every overlapping section put a seam down a chapter 1 cliff. `NGB_D3D_SECTION_PULL=n`
  (steps, 0 off), `NGB_D3D_SECTION_SLOPE=f`, `NGB_D3D_SECTION_LOG=1` (draws pulled per frame).

- The gamma ramp (D3DDevice_SetGammaRamp: the game's Screen Adjustment -- gamma, brightness, contrast) is
  applied as the front buffer is copied for the presenter, through a 256-entry table per channel; at the
  game's default (an identity ramp) the plain copy as before. `NGB_D3D_GAMMALOG=1` logs each ramp.

- A fixed-function stage reading a texture with none bound is off (its output the stage before's), as the
  console's D3D sets up its combiners: the game's fades are drawn TEXTURE x TFACTOR without a texture and
  show their vertex colour. Read as white, Game Over's fade painted its sepia texture factor over the
  screen each frame and the motion blur fed it back to a flat pale yellow.

- Point lists: the vertex shader writes each point's size (PointHlsl, inserted before main's return; D3D's
  POINTSIZE, scaled with the distance when POINTSCALEENABLE is on, in the render scale's pixels) -- without
  it Vulkan draws nothing. A point sprite is stage 3 alone, as the console builds it: that stage gets the
  point's coordinates (xemu's pT3) and the combiners come from its state on the vertex colour; the other
  stages keep the vertex's coordinates and are not read (the torch embers and Ninpo sparks, #51).
  `NGB_D3D_SPRITELOG=1` logs each sprite setup's stages; xemu's `NV2A_SPRITE_LOG=1` the combiners the
  registers set for one.

- Vulkan's rules, as the validation layer checks them (`VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation`):
  a layout change ends the rendering pass first (dynamic rendering allows no barrier inside one); the
  shader helpers that read a texture (`shadow%d`, `spreadtap%d`, `bumpduv%d`) are written out per stage,
  since glslang copies a texture passed as a parameter (invalid SPIR-V); a draw that reads the colour
  target it draws into samples a copy of it made just before (sampling an attachment is undefined). AMD's
  driver let all of these through, so a difference seen only on other GPUs is worth a validation run.
  Under the headless gamescope, its own WSI layer reports a few swapchain and dma-buf messages that are
  not ours.

- Debug views and logs for the water and the sprites: `NGB_D3D_PSDEBUG=bump | duv | copy | lod | texel`
  (the water's draws only: the ripple texture's raw bytes, its decoded displacement, the screen copy it
  refracts, the level of detail and mip count the GPU reports, the texels decoded one by one) and
  `spr | sprtex` (point sprites only: their coordinates, stage 3's texture); `NGB_D3D_BUMPCHECK=1` reads a
  bump map back from the GPU and compares it with the upload; `NGB_D3D_BUMPLOG=1` the water's stage 0
  sampler state and every mip level's value range; `NGB_D3D_BUMPLEVEL=n` reads the ripple texture at
  level n everywhere. `NGB_D3D_PSCLOG=1` logs each pixel shader's combiner constants, `NGB_D3D_BIASLOG=1`
  each LOD bias used; on xemu's side `NV2A_PSC_LOG=1`, `NV2A_BUMP_FILTER_LOG=1` (the ripple map's
  TEXFILTER0) and `NGB_NV2A_FOGLOG=1` (the fog registers) give the hardware's view of the same state.
  For z-fighting: `NGB_D3D_IDVIEW=1` tints each opaque 3D surface by its draw, `=2` writes each draw's
  number exactly on the frames `dump_dir/burst.now` dumps, with `dump_dir/ids_<frame>.txt` naming each
  draw's map piece, chunk and texture; a stepped frame (`NGB_D3D_STEP`) also writes
  `dump_dir/vertices_<frame>.txt`, every fixed-function draw's matrices, positions and guest call stack. F7 traces carry each point draw's state (`pt`: sprite, scaling, size, min, max, A B C,
  the vertex's attribute bits).

## The render thread (2026-10-27: Graphics > Multithreaded Renderer, on by default on Android)

Graphics > Multithreaded Renderer (needs a restart), or `NGB_D3D_THREAD=1` (`0` off) over it, draws on a thread of its own (src/d3d/gpu/threaded.cpp, after TGP482's SDK
render_thread.cpp): the title's thread records each call -- the state that changed, the resource headers
it names (gpu/headers.h), its indices and rects, the vertices of a DrawVerticesUP, and the game's side of
it (gpu::Peek: the map piece, the towers' chunk, the 2D and HUD layout flags; gpu::QuickPeek once a frame) --
and the render thread draws the recording, a frame behind at most. xemu's push buffer reader is held back
to what has been drawn (nv2a_pfifo_gate), so every fence and semaphore the title waits on follows the
drawing; the title's progress reads (nv2a_user_read_hook) and the GPU wait hand over what it recorded.
`NGB_D3D_THREAD=capture` replays each call at once on the title's thread (a test of the recording),
`NGB_D3D_GATE=0` leaves the GPU emulation free. The title's wait for the frame before keeps the virtual CPU
(`NGB_D3D_WAIT_RELEASE=1` gives it to the title's other threads meanwhile, which made play stutter); it runs
after the Swap hook, out of every lock, since a wait under the renderer's lock froze the game when another guest
thread made a Direct3D call. A wait of 10 s is logged, then every 10 s (never ended: the F1 menu holds the
render thread on purpose). The
2D-screen decision (src/pc_menu.c) takes the last presented frame's draw counts, not the live ones. Attract demo: the title's thread 134 -> 110 ms/s.

On Windows it froze the game (#138, 0.9.41-0.9.43: at boot, in a load or in play). A GPU register read takes
the emulation's lock (nv2a_vk_mmio_read) and calls the progress hook inside it; on Windows the register handler
ran outside a host section, so the hook's own section ended at depth 0 and gave the virtual CPU away there, the
GPU's interrupt took the CPU and waited for the lock, and the title waited for the CPU. Register accesses are now
a host section around the lock. (Linux's signal handler already makes one, so it never froze.)

Kick-offs (0.9.57). With the push buffer read only as far as has been drawn, Direct3D's limit on how far it
runs ahead of the GPU -- counted in kick-offs, it seems: D3D_MakeRequestedSpace walks a table of them -- had the
title waiting on the drawing of what it had just recorded, a few hundred times a second in a heavy frame, the
title's thread and the render thread taking turns (a Pixel 4a at 40 fps where each needed 9 ms a frame and the
GPU 5-10 ms; `NV2A_PACE_LOG=1` showed PFIFO held at the gate 1.5 s of every 2). A bigger push buffer changed
nothing; bigger kick-offs did: with the render thread, Direct3D_CreateDevice gets 256 KB ones in the title's
own 1 MB buffer (0x3D9588, src/d3d/d3d.c; `NGB_D3D_KICKOFF_KB=n`, 0 the title's 32 KB). The attract demo on
that phone: 30-40 fps stretches at 55-60, 48 -> 58.6 fps on average with the register changes.

## Calls taken (stage B, 0.9.57 default)

Two of the title's most frequent calls end at the hook, their library body skipped: CDevice_SetStateVB (the
lazy state flush and the vertex arrays) and D3DDevice_SetRenderState_Simple (two words into the push buffer).
What those bodies write only feeds the NV2A model, which draws nothing while this renderer draws; the renderer
has the state from the hooks. Each was checked in the library's code for anything the title or the rest of the
library reads (src/d3d/hle.cpp kTakeable: why SetStateUP, SetLight and the resource-stamping calls are not
taken). 8% less CPU on a desktop, the pictures as they were in five chapters and the shader scenes, with the
render thread on and off. `NGB_D3D_TAKE=name,...` takes only those, `none` none; never in mirror mode or with
`NGB_D3D_KEEP_NV2A=1`.
