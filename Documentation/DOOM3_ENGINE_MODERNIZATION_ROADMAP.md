# Doom 3 Engine Modernization Roadmap

This roadmap is a strategy for improving the GPL Doom 3 / idTech4 engine in this repository while preserving compatibility with existing Doom 3 assets, save-game expectations, tools, game DLL boundaries, and renderer/game behavior. It assumes the current tree layout centered on `neo/idlib`, `neo/framework`, `neo/renderer`, `neo/game`, `neo/cm`, `neo/sound`, `neo/ui`, `neo/sys`, and `neo/tools`.

The work should proceed as dependency-ordered slices. Each phase below is meant to produce a buildable, testable engine state before the next phase begins.

## Modernization Principles

1. Preserve shipped content compatibility first.
   - Existing `base` assets, decl syntax, `.map`, `.proc`, `.cm`, `.aas`, `.gui`, `.md5mesh`, `.md5anim`, savegames, demos, and network snapshots must remain readable unless a migration tool and compatibility mode exist.
   - Source anchors: `neo/framework/FileSystem.*`, `neo/framework/DeclManager.*`, `neo/idlib/MapFile.*`, `neo/renderer/RenderWorld_load.cpp`, `neo/cm/CollisionModel_load.cpp`, `neo/game/SaveGame.*`, `neo/game/Game_network.cpp`.

2. Build adapters before replacing subsystems.
   - Wrap legacy surfaces such as `idFileSystem`, `idDeclManager`, `idRenderSystem`, `idSoundSystem`, `idSession`, `idNetworkSystem`, `idSys`, and `idCVarSystem` before changing internal implementation.
   - This keeps game code and tools compiling while internals are modernized.

3. Keep determinism explicit.
   - Physics, scripts, entity events, snapshots, demos, and savegames depend on stable ordering and serialization.
   - Any threading, renderer decoupling, or STL migration must not change observable game-frame order without a compatibility gate.
   - Source anchors: `neo/game/Game_local.cpp`, `neo/game/gamesys/Event.*`, `neo/game/script/Script_*.cpp`, `neo/game/physics/*`, `neo/framework/async/*`.

4. Split engine modernization from renderer feature work.
   - First stabilize build, platform, memory, resource lifetime, and test harnesses.
   - Then modernize renderer internals.
   - Only after that add new visual features such as HDR, PBR, clustered lights, temporal effects, or modern API backends.

5. Make every large change reversible.
   - Each phase should be landed behind clear compile-time or runtime selection where practical.
   - Prefer `r_*`, `s_*`, `com_*`, `fs_*`, and `net_*` cvars for migration gates when behavior may affect shipped content.

## Phase 0: Baseline Capture and Safety Rails

Goal: create a trustworthy measurement and regression baseline before changing engine behavior.

### 0.1 Build and Runtime Baselines

Implementation steps:

1. Record supported build presets and solution configurations.
2. Add a root documentation file listing exact commands for Debug/Release, game DLL, tools, and launcher builds.
3. Capture a smoke launch command that starts Doom 3 windowed, loads the main menu, and exits or is terminated after a fixed interval.
4. Capture at least one map-load smoke for `gameLocal.InitFromNewMap` and `idSessionLocal::MoveToNewMap`.
5. Save expected log markers for file system startup, decl registration, renderer init, sound init, game DLL load, and map load.

Source anchors:

- `neo/framework/Common.cpp`
- `neo/framework/Session.cpp`
- `neo/framework/FileSystem.cpp`
- `neo/framework/DeclManager.cpp`
- `neo/game/Game_local.cpp`
- `neo/sys/win32/*`

Verification:

- Release build succeeds.
- Debug build succeeds or has documented blockers.
- Windowed launch reaches menu.
- Map-load smoke reaches first rendered frame.
- Logs are archived under a reproducible test output directory.

### 0.2 Static Inventory

Implementation steps:

1. Keep `ENGINE_ARCHITECTURE_STATIC_ANALYSIS.md` as the source-derived subsystem map.
2. Add inventories for global singletons, raw ownership, direct OpenGL calls, direct Win32 calls, network serialization entry points, savegame serialization entry points, and renderer command structs.
3. Generate a call graph for `idCommonLocal::Frame`, `idSessionLocal::Frame`, `idGameLocal::RunFrame`, `R_RenderView`, and `RB_ExecuteBackEndCommands`.
4. Tag each modernization area with blast radius: framework-only, renderer-only, game ABI, savegame ABI, asset format, network protocol, or tools.

Source anchors:

- `neo/framework/Common.cpp`
- `neo/framework/Session.cpp`
- `neo/game/Game_local.cpp`
- `neo/renderer/tr_main.cpp`
- `neo/renderer/tr_backend.cpp`
- `neo/framework/async/*`
- `neo/game/SaveGame.*`

Verification:

- Inventories are generated from source search commands and checked into documentation.
- No source behavior changes in this phase.

### 0.3 Regression Harness

Implementation steps:

1. Add a scriptable smoke harness that accepts executable path, base path, map name, timeout, and log output.
2. Add CTest entries for launch smoke, map smoke, decl parse smoke, file system path smoke, and renderer command smoke where possible.
3. Add a "no content mutation" guard for base assets.
4. Add log scanning for fatal errors, common asserts, missing decls, missing images, missing sound shaders, and failed collision model loads.

Source anchors:

- `neo/framework/CmdSystem.*`
- `neo/framework/CVarSystem.*`
- `neo/framework/FileSystem.*`
- `neo/framework/DeclManager.*`
- `neo/game/Game_local.cpp`

Verification:

- `ctest --output-on-failure` can run at least documentation/static tests immediately.
- Runtime tests skip cleanly when required retail assets are unavailable.

## Phase 1: Build System and Platform Boundary

Goal: make the engine easy to build, package, and reason about on current Windows while keeping later Linux/macOS work possible.

### 1.1 CMake and Solution Hygiene

Implementation steps:

1. Treat root `CMakeLists.txt` and `CMakePresets.json` as the modern entry point.
2. Split targets into `idlib`, `framework`, `renderer`, `sound`, `game`, `d3xp`, `tools`, and launcher/executable targets.
3. Move compiler definitions into target-local declarations.
4. Replace global include-directory leakage with target include directories.
5. Make tool builds optional and off by default for runtime-only builds.
6. Keep Visual Studio project files buildable until CMake is proven complete.

Source anchors:

- Root `CMakeLists.txt`
- `neo/*.vcxproj`
- `neo/framework/*`
- `neo/game/*`
- `neo/d3xp/*`
- `neo/tools/*`

Verification:

- Configure from a clean build directory.
- Build runtime-only Release.
- Build tools configuration separately.
- Verify game DLL output path remains compatible with the launcher.

### 1.2 Platform API Isolation

Implementation steps:

1. Inventory direct Win32, OpenGL context, file dialog, thread, timer, and input calls.
2. Define a narrow platform services layer under `neo/sys`.
3. Move platform-specific includes out of common headers where possible.
4. Introduce adapters for timing, threading, filesystem paths, dynamic library loading, clipboard, window events, and input events.
5. Keep public engine interfaces unchanged while moving implementation details behind the adapter.

Source anchors:

- `neo/sys/win32/*`
- `neo/sys/sys_public.h`
- `neo/framework/EventLoop.*`
- `neo/framework/Common.cpp`
- `neo/renderer/RenderSystem_init.cpp`

Verification:

- Engine and tools compile with platform includes restricted to `neo/sys`, renderer GL bootstrap files, and tool-specific UI code.
- Launch smoke still reaches menu.

### 1.3 Third-Party Dependency Policy

Implementation steps:

1. List every bundled or expected external dependency, license, source path, and build flag.
2. Prefer vendored, pinned dependencies for runtime-critical components.
3. Keep dependency wrappers small and local.
4. Add a dependency update checklist with compile, runtime, and packaging checks.

Source anchors:

- `neo/curl`
- `neo/openal`
- `neo/MayaImport`
- `neo/tools`
- Sound backend files under `neo/sound`

Verification:

- Dependency list can reproduce a clean build on another machine.
- Runtime packaging reports all required DLLs.

## Phase 2: 64-Bit Correctness and ABI Stability

Goal: remove pointer truncation, legacy integer assumptions, and unsafe casts without breaking game DLL or asset formats.

### 2.1 Pointer and Integer Audit

Implementation steps:

1. Use `POINTER_64BIT_MIGRATION_REPORT.md` as the starting inventory.
2. Classify each issue as pointer storage, pointer hashing, serialization, renderer handle, network field, savegame field, or legacy API interop.
3. Replace pointer-to-`int` storage with `intptr_t`, `uintptr_t`, typed handles, or explicit indices.
4. Do not widen serialized savegame/network fields unless the compatibility story is written first.
5. Add compile-time assertions for struct sizes where binary formats rely on size.

Source anchors:

- `neo/idlib/*`
- `neo/framework/*`
- `neo/game/SaveGame.*`
- `neo/game/Game_network.cpp`
- `neo/renderer/tr_local.h`
- `neo/renderer/Model.h`

Verification:

- 64-bit build compiles cleanly.
- Pointer truncation warnings are either eliminated or documented with local suppression.
- Save/load smoke remains compatible with existing saves where supported.

### 2.2 Typed Handles

Implementation steps:

1. Introduce small typed handle wrappers for renderer entities, renderer lights, sound emitters, file handles, and network clients.
2. Convert internal arrays from raw integer indexes to typed handles one subsystem at a time.
3. Keep public API overloads for legacy callers during migration.
4. Add invalid-handle constants and validation helpers.

Source anchors:

- `neo/renderer/RenderWorld.h`
- `neo/renderer/RenderWorld_local.h`
- `neo/sound/sound.h`
- `neo/sound/snd_local.h`
- `neo/framework/FileSystem.h`
- `neo/framework/async/NetworkSystem.h`

Verification:

- Existing callers compile.
- Invalid handle paths are asserted in Debug and recover cleanly in Release where practical.

## Phase 3: Memory, Ownership, and Lifetime

Goal: make ownership explicit without changing frame allocation semantics that the renderer and game depend on.

### 3.1 Ownership Inventory

Implementation steps:

1. Inventory allocations through `Mem_Alloc`, `Mem_ClearedAlloc`, `new`, `delete`, frame allocators, zone allocators, and custom caches.
2. Classify lifetime as static, subsystem, map, frame, command-buffer, asset-cache, game-entity, sound-world, or tool-only.
3. Add owner comments or wrapper types only at boundary objects first.

Source anchors:

- `neo/idlib/Heap.*`
- `neo/idlib/containers/*`
- `neo/renderer/tr_local.h`
- `neo/renderer/tr_trisurf.cpp`
- `neo/renderer/VertexCache.*`
- `neo/game/Game_local.cpp`
- `neo/sound/snd_cache.cpp`

Verification:

- No behavior changes.
- Allocation inventory identifies top risk areas by subsystem.

### 3.2 RAII at Subsystem Boundaries

Implementation steps:

1. Convert obvious init/shutdown pairs to scoped owners where construction order is local.
2. Avoid converting frame-allocated renderer geometry to standard containers until frame lifetime is isolated.
3. Add destructors only where they do not change global shutdown order.
4. Add leak-checking hooks for map shutdown and renderer world teardown.

Source anchors:

- `neo/framework/Common.cpp`
- `neo/framework/Session.cpp`
- `neo/renderer/RenderWorld.cpp`
- `neo/sound/snd_world.cpp`
- `neo/game/Game_local.cpp`

Verification:

- Repeated map load/unload smoke does not increase tracked allocations.
- Shutdown order remains stable in logs.

### 3.3 Safer Containers

Implementation steps:

1. Keep `idList`, `idHashIndex`, `idDict`, and `idStr` as public compatibility types initially.
2. Add bounds-checked helper methods in Debug.
3. Convert isolated internal scratch lists to `std::vector` only where no public ABI or serialization depends on the type.
4. Convert raw C arrays to `std::array` for fixed-size local buffers.
5. Keep binary asset structs plain-old-data where file layout depends on them.

Source anchors:

- `neo/idlib/containers/*`
- `neo/idlib/Str.*`
- `neo/framework/DeclManager.*`
- `neo/renderer/Model*.cpp`
- `neo/game/*`

Verification:

- Build with high warning level.
- Run smoke tests after each subsystem conversion.

## Phase 4: idLib Modernization

Goal: modernize utility code carefully because every subsystem depends on it.

### 4.1 Math and Geometry Invariants

Implementation steps:

1. Add tests for `idVec*`, `idMat*`, `idPlane`, `idBounds`, `idWinding`, `idDrawVert`, and SIMD paths.
2. Lock down exact floating-point behavior expected by collision and renderer culling.
3. Add explicit conversion helpers instead of implicit casts where warnings appear.
4. Keep SIMD and generic implementations behavior-equivalent.

Source anchors:

- `neo/idlib/math/*`
- `neo/idlib/geometry/*`
- `neo/idlib/bv/*`
- `neo/idlib/MapFile.*`
- `neo/cm/CollisionModel*.cpp`

Verification:

- Math unit tests pass for scalar and SIMD paths.
- Collision model load smoke produces identical bounds and trace results for fixture maps.

### 4.2 String and Dictionary Safety

Implementation steps:

1. Add tests for `idStr`, `idDict`, parser tokenization, and decl key/value behavior.
2. Replace unsafe local formatting with bounded helpers.
3. Keep case sensitivity and path slash normalization exactly compatible with existing assets.
4. Add source comments where non-obvious string behavior is asset-format compatibility.

Source anchors:

- `neo/idlib/Str.*`
- `neo/idlib/Dict.*`
- `neo/idlib/Lexer.*`
- `neo/idlib/Parser.*`
- `neo/framework/DeclManager.*`

Verification:

- Decl parse fixtures produce identical canonical names and values.
- File path lookup remains case-compatible with current behavior.

## Phase 5: File System, Decls, and Resource Management

Goal: make asset loading deterministic, diagnosable, and eventually asynchronous without changing asset semantics.

### 5.1 Virtual File System Hardening

Implementation steps:

1. Document current search path order, pack-file precedence, base/game/d3xp interactions, and save path rules.
2. Add diagnostics for duplicate files across search paths.
3. Separate path normalization from file opening.
4. Add a read-only mode for shipped content and a writable mode only for saves/configs/generated data.
5. Keep `idFileSystem` public behavior stable.

Source anchors:

- `neo/framework/FileSystem.h`
- `neo/framework/FileSystem.cpp`
- `neo/framework/File.cpp`
- `neo/framework/Unzip.cpp`

Verification:

- Known files resolve to the same source path before and after changes.
- Missing file diagnostics include logical path and search roots.

### 5.2 Decl Manager Staging

Implementation steps:

1. Add decl parse fixtures for materials, sounds, entity defs, skins, particles, and GUIs.
2. Split decl discovery, parse, validation, and registration into visible stages internally.
3. Add error recovery tests for malformed decls.
4. Add optional strict mode that reports duplicate definitions, missing images, missing sounds, and invalid material stages.
5. Keep permissive legacy mode as default until content is audited.

Source anchors:

- `neo/framework/DeclManager.h`
- `neo/framework/DeclManager.cpp`
- `neo/framework/Decl*.cpp`
- `neo/renderer/Material.*`
- `neo/sound/snd_shader.cpp`

Verification:

- Full base decl scan completes.
- Strict mode report is generated but does not block launch initially.

### 5.3 Asset Cache Ownership

Implementation steps:

1. Map model, image, sound, collision, AAS, and GUI cache lifetimes.
2. Add cache stats cvars and console commands.
3. Add explicit reload paths for dev workflows.
4. Add reference tracking for assets used by a loaded map.

Source anchors:

- `neo/renderer/ModelManager.*`
- `neo/renderer/Image.*`
- `neo/sound/snd_cache.cpp`
- `neo/cm/CollisionModel_load.cpp`
- `neo/game/ai/AAS*.cpp`
- `neo/ui/UserInterface.cpp`

Verification:

- `reloadDecls`, image reload, sound reload, and map reload workflows still function.
- Cache stats are stable across repeated map loads.

## Phase 6: Renderer Foundation

Goal: preserve the Doom 3 renderer while making the front end, back end, resources, and API boundary ready for modern backends.

### 6.1 Renderer Command Boundary

Implementation steps:

1. Document every command emitted to the back end.
2. Convert command structs to typed, validated records without changing order.
3. Add asserts for command-buffer alignment and size.
4. Add a debug command trace mode that logs `RC_DRAW_VIEW`, `RC_SET_BUFFER`, `RC_SWAP_BUFFERS`, and `RC_COPY_RENDER`.
5. Keep `RB_ExecuteBackEndCommands` behavior stable until command recording tests exist.

Source anchors:

- `neo/renderer/tr_local.h`
- `neo/renderer/RenderSystem.cpp`
- `neo/renderer/tr_main.cpp`
- `neo/renderer/tr_backend.cpp`
- `neo/renderer/GuiModel.cpp`

Verification:

- Captured command sequence for a known menu frame and a known map frame stays identical.

### 6.2 Front-End Visibility and Surface Generation

Implementation steps:

1. Add debug counters for portal traversal, visible areas, visible entities, visible lights, draw surfaces, shadow surfaces, and generated subviews.
2. Preserve the current `R_RenderView` order: view setup, portal visibility, light surfaces, model surfaces, unnecessary light removal, draw-surf sort, subviews, draw command.
3. Isolate culling and surface creation into testable functions where possible.
4. Add fixtures for mirror/subview, GUI surface, animated model, particle, and static world cases.

Source anchors:

- `neo/renderer/tr_main.cpp`
- `neo/renderer/RenderWorld.cpp`
- `neo/renderer/RenderWorld_portals.cpp`
- `neo/renderer/tr_light.cpp`
- `neo/renderer/tr_lightrun.cpp`
- `neo/renderer/tr_deform.cpp`

Verification:

- Draw surface counts remain stable for fixture scenes.
- Portal visibility output remains stable for fixture maps.

### 6.3 Back-End API Abstraction

Implementation steps:

1. Inventory all direct `qgl*` calls.
2. Create a renderer device interface that initially forwards to the existing OpenGL path.
3. Move state changes, buffer binding, texture binding, shader binding, and draw calls behind the interface.
4. Keep legacy OpenGL back end as the reference implementation.
5. Add a null back end for command validation and headless tests.

Source anchors:

- `neo/renderer/tr_backend.cpp`
- `neo/renderer/draw_common.cpp`
- `neo/renderer/draw_arb.cpp`
- `neo/renderer/draw_arb2.cpp`
- `neo/renderer/draw_exp.cpp`
- `neo/renderer/RenderSystem_init.cpp`
- `neo/renderer/qgl.h`

Verification:

- Legacy OpenGL output remains visually equivalent.
- Null back end can execute command streams without a GL context.

### 6.4 Resource Lifetime and Upload Path

Implementation steps:

1. Separate CPU asset data from GPU resource objects for images, models, vertex caches, and render targets.
2. Add explicit create/destroy/reload methods for GPU resources.
3. Add lost-device style reinitialization even if OpenGL does not require it.
4. Track memory usage by resource type.

Source anchors:

- `neo/renderer/Image.h`
- `neo/renderer/Image*.cpp`
- `neo/renderer/Model*.cpp`
- `neo/renderer/VertexCache.*`
- `neo/renderer/tr_local.h`

Verification:

- `vid_restart` and asset reload still work.
- Resource counters return to baseline after map unload.

## Phase 7: Renderer Feature Modernization

Goal: add modern visual capability after the renderer foundation is stable.

### 7.1 Linear HDR Frame Pipeline

Implementation steps:

1. Add an optional floating-point main color target.
2. Convert lighting accumulation to linear space under a cvar gate.
3. Add tonemapping and exposure as a final pass.
4. Preserve legacy gamma/brightness behavior when HDR is disabled.
5. Add screenshot comparisons for legacy and HDR modes.

Source anchors:

- `neo/renderer/tr_render.cpp`
- `neo/renderer/draw_common.cpp`
- `neo/renderer/draw_arb2.cpp`
- `neo/renderer/Material.*`
- `neo/renderer/Image*`

Verification:

- Legacy mode produces unchanged screenshots within tolerance.
- HDR mode renders non-black, non-clipped frames with stable exposure.

### 7.2 Material System Extension

Implementation steps:

1. Keep existing material decl syntax valid.
2. Add optional material keywords for roughness, metalness, normal scale, emissive strength, and packed texture channels.
3. Extend `idMaterial` parse data without changing old material behavior.
4. Add material validation warnings for unsupported combinations.
5. Add sample materials in a test content folder, not in shipped base assets.

Source anchors:

- `neo/renderer/Material.h`
- `neo/renderer/Material.cpp`
- `neo/framework/DeclManager.*`
- `neo/idlib/Lexer.*`

Verification:

- Existing material decls parse unchanged.
- New material decl fixtures parse and render in a test scene.

### 7.3 Modern Lighting Path

Implementation steps:

1. Keep stencil shadow and interaction path as the compatibility renderer.
2. Add a new optional light list build step from existing `viewLight_t`.
3. Implement clustered or tiled light culling only after resource abstraction exists.
4. Add shadow-map support as an optional path for selected lights.
5. Keep portal and area visibility as CPU culling input.

Source anchors:

- `neo/renderer/tr_light.cpp`
- `neo/renderer/tr_lightrun.cpp`
- `neo/renderer/draw_arb2.cpp`
- `neo/renderer/tr_stencilshadow.cpp`
- `neo/renderer/RenderWorld_portals.cpp`

Verification:

- Legacy stencil path remains available and unchanged.
- New lighting path can be toggled per run and compared against fixture scenes.

### 7.4 Post-Processing Stack

Implementation steps:

1. Add a named pass list after main scene rendering.
2. Implement bloom, color grading, filmic tonemap, FXAA/SMAA, and optional temporal anti-aliasing as independent passes.
3. Add cvars for each pass and a debug overlay for render target inspection.
4. Keep GUI rendering order explicit so UI remains crisp and is not accidentally tonemapped unless intended.

Source anchors:

- `neo/renderer/tr_backend.cpp`
- `neo/renderer/tr_render.cpp`
- `neo/renderer/GuiModel.cpp`
- `neo/ui/*`

Verification:

- Menu and HUD remain readable at multiple resolutions.
- Disabling all post-processing returns to legacy-compatible output.

## Phase 8: Game Framework, Scripting, and Events

Goal: improve maintainability of game logic without breaking scripts, spawn args, entity defs, or savegames.

### 8.1 Event System Safety

Implementation steps:

1. Inventory all `idEventDef` declarations and entity event bindings.
2. Add validation for duplicate event names and argument mismatches.
3. Add debug tracing for delayed events and per-frame event volume.
4. Replace unsafe event argument handling with typed helpers internally.
5. Preserve script-visible event names.

Source anchors:

- `neo/game/gamesys/Event.h`
- `neo/game/gamesys/Event.cpp`
- `neo/game/Entity.cpp`
- `neo/game/Actor.cpp`
- `neo/game/Player.cpp`

Verification:

- Script startup succeeds.
- Map smoke produces no new event validation errors.

### 8.2 Script VM Isolation

Implementation steps:

1. Document script program load, compile, object binding, and thread execution.
2. Separate script compiler/parser state from runtime thread state.
3. Add bounds checks and diagnostics around script object fields.
4. Add tests for common script calls into entities.
5. Keep bytecode and script syntax compatible.

Source anchors:

- `neo/game/script/Script_Compiler.cpp`
- `neo/game/script/Script_Interpreter.cpp`
- `neo/game/script/Script_Program.cpp`
- `neo/game/script/Script_Thread.cpp`
- `neo/game/script/Script_Object.cpp`

Verification:

- Existing scripts compile.
- Scripted map interactions work in smoke scenes.

### 8.3 Entity Update Structure

Implementation steps:

1. Add instrumentation to `idGameLocal::RunFrame` for active entity count, physics count, think count, event count, and script thread count.
2. Preserve the current ordering of usercmds, entities, physics, events, scripts, and return flags.
3. Split large entity methods into local helpers only when tests cover behavior.
4. Introduce optional component-like helpers for new systems without replacing `idEntity`.

Source anchors:

- `neo/game/Game_local.cpp`
- `neo/game/Entity.h`
- `neo/game/Entity.cpp`
- `neo/game/Actor.*`
- `neo/game/Player.*`
- `neo/game/ai/AI.*`

Verification:

- Existing maps run with unchanged entity spawn counts.
- Save/load and client prediction smoke remain stable.

## Phase 9: Physics and Collision

Goal: make collision and physics more testable and robust while preserving gameplay movement.

### 9.1 Collision Model Tests

Implementation steps:

1. Add fixtures for trace, contents, bounds, rotation, translation, and contact generation.
2. Test both world collision and entity clip models.
3. Add debug output for collision model load failures.
4. Keep `.cm` format compatibility.

Source anchors:

- `neo/cm/CollisionModel.h`
- `neo/cm/CollisionModel_load.cpp`
- `neo/cm/CollisionModel_trace.cpp`
- `neo/game/physics/Clip.cpp`

Verification:

- Fixture traces return stable fractions, normals, contents, and entity numbers.

### 9.2 Physics Interface Cleanup

Implementation steps:

1. Inventory all `idPhysics` subclasses.
2. Add explicit ownership and activation rules for clip models.
3. Add Debug asserts for invalid physics state transitions.
4. Keep `idEntity::SetPhysics` behavior compatible.
5. Add lightweight profiling per physics object type.

Source anchors:

- `neo/game/physics/Physics.h`
- `neo/game/physics/Physics_*.cpp`
- `neo/game/Entity.cpp`
- `neo/game/Game_local.cpp`

Verification:

- Player movement, movers, projectiles, ragdolls, and articulated figures work in fixture maps.

### 9.3 Articulated Figure Stability

Implementation steps:

1. Add tests around AF body load, constraint setup, save/load, and ragdoll activation.
2. Add diagnostics for broken joints, invalid masses, bad constraints, and unstable iterations.
3. Preserve existing AF decl syntax.
4. Consider modern solver improvements only after baseline tests pass.

Source anchors:

- `neo/game/physics/AF.*`
- `neo/game/physics/AFEntity.*`
- `neo/game/physics/Force*.cpp`
- `neo/game/physics/Physics_RigidBody.cpp`

Verification:

- Ragdoll fixture remains stable for a fixed number of frames.
- Save/load of AF entities preserves state.

## Phase 10: Sound System

Goal: preserve sound shader behavior while modernizing backend, streaming, diagnostics, and world integration.

### 10.1 Backend Boundary

Implementation steps:

1. Isolate hardware/backend calls from `idSoundSystemLocal`.
2. Keep `idSoundWorldLocal` and `idSoundEmitterLocal` as gameplay-facing concepts.
3. Add a null sound backend for headless tests.
4. Add backend capability reporting for channels, sample format, EFX/reverb support, and streaming.
5. Keep sound shader parsing unchanged.

Source anchors:

- `neo/sound/sound.h`
- `neo/sound/snd_local.h`
- `neo/sound/snd_system.cpp`
- `neo/sound/snd_world.cpp`
- `neo/sound/snd_emitter.cpp`
- `neo/sound/snd_shader.cpp`

Verification:

- Launch smoke works with real sound backend.
- Launch smoke works with null backend.
- Sound shader scan succeeds.

### 10.2 World Propagation and Reverb

Implementation steps:

1. Document portal-area sound propagation and listener area logic.
2. Add debug overlays or console dumps for audible emitters, listener area, and reverb zone.
3. Add tests for sound world serialization and emitter state updates.
4. Preserve current use of render-world area information.

Source anchors:

- `neo/sound/snd_world.cpp`
- `neo/sound/snd_emitter.cpp`
- `neo/renderer/RenderWorld_portals.cpp`
- `neo/game/Game_local.cpp`

Verification:

- Moving across portal areas updates audible emitter sets and reverb state consistently.

## Phase 11: GUI and User Interface

Goal: keep Doom 3 `.gui` compatibility while improving resolution independence, rendering quality, and tooling.

### 11.1 GUI Runtime Safety

Implementation steps:

1. Add parser fixtures for `.gui` files.
2. Add diagnostics for missing materials, invalid scripts, bad registers, and invalid winvars.
3. Add tests for `idWinVar` parsing and synchronization.
4. Keep `idWindow` behavior compatible.

Source anchors:

- `neo/ui/UserInterface.*`
- `neo/ui/Window.*`
- `neo/ui/Winvar.*`
- `neo/ui/DeviceContext.*`
- `neo/ui/GuiScript.h`

Verification:

- Main menu, HUD, and entity GUIs render.
- GUI reload works during development.

### 11.2 High-DPI and Modern Text

Implementation steps:

1. Decouple GUI logical coordinates from physical window pixels.
2. Add scaling modes that preserve legacy layout by default.
3. Improve font atlas loading and fallback behavior.
4. Add tests for common aspect ratios.

Source anchors:

- `neo/ui/DeviceContext.cpp`
- `neo/ui/UserInterface.cpp`
- `neo/renderer/GuiModel.cpp`
- `neo/renderer/RenderSystem.cpp`

Verification:

- Menus and HUD are readable at 720p, 1080p, 1440p, and ultrawide.
- Legacy scale mode matches original layout.

## Phase 12: Networking

Goal: make networking safer and easier to maintain while preserving snapshot semantics.

### 12.1 Protocol Documentation and Guards

Implementation steps:

1. Document packet types, snapshot fields, usercmd flow, reliable messages, and delta compression.
2. Add protocol version checks and compatibility comments around serialized fields.
3. Add bounds checks to message reads.
4. Add fuzz-style tests for malformed messages where possible.

Source anchors:

- `neo/framework/async/AsyncNetwork.*`
- `neo/framework/async/AsyncServer.cpp`
- `neo/framework/async/AsyncClient.cpp`
- `neo/framework/async/MsgChannel.*`
- `neo/game/Game_network.cpp`
- `neo/game/Player.cpp`

Verification:

- Local listen-server smoke works.
- Malformed packet tests do not crash.

### 12.2 Prediction and Snapshot Tests

Implementation steps:

1. Add deterministic tests for `idGameLocal::ServerWriteSnapshot` and `ClientReadSnapshot` using fixture entities.
2. Add prediction smoke for `idGameLocal::ClientPrediction`.
3. Track mismatches between predicted and authoritative player state.
4. Keep network-visible entity state fields stable.

Source anchors:

- `neo/game/Game_network.cpp`
- `neo/game/Player.cpp`
- `neo/game/Entity.cpp`
- `neo/framework/async/AsyncClient.cpp`

Verification:

- Snapshot read/write round-trips for fixture state.
- Prediction does not diverge in a fixed local test.

## Phase 13: Tools, Map Pipeline, and Editor Compatibility

Goal: preserve the content pipeline while making tools buildable and easier to run.

### 13.1 Tool Build Separation

Implementation steps:

1. Split tool targets from runtime targets.
2. Keep `dmap`, `renderbump`, material editor, and Radiant-related code optional.
3. Document tool dependencies separately from runtime dependencies.
4. Add smoke tests for command-line tools that can run headlessly.

Source anchors:

- `neo/tools/compilers/dmap/*`
- `neo/tools/compilers/renderbump/*`
- `neo/tools/radiant/*`
- `neo/tools/materialeditor/*`

Verification:

- Runtime build does not require editor-only dependencies.
- Tool build can be enabled and built independently.

### 13.2 Map Compiler and BSP/Portal Validation

Implementation steps:

1. Add fixtures for map compile output where content license permits.
2. Validate `.proc`, portal, area, and entity output against renderer load expectations.
3. Add diagnostics for invalid portals, leaks, missing materials, and bad entity defs.
4. Keep runtime parser compatible with existing compiled maps.

Source anchors:

- `neo/tools/compilers/dmap/*`
- `neo/renderer/RenderWorld_load.cpp`
- `neo/renderer/RenderWorld_portals.cpp`
- `neo/idlib/MapFile.*`

Verification:

- Fixture map compiles.
- Runtime loads compiled output and portal traversal works.

## Phase 14: Threading and Frame Pacing

Goal: modernize concurrency only after ownership and subsystem boundaries are clear.

### 14.1 Thread Ownership Map

Implementation steps:

1. Document current main thread, async sound, async networking, renderer command handoff, and tool threads.
2. Mark data shared across threads.
3. Add thread assertions around APIs that must be called from main, render, sound, or network thread.
4. Keep single-thread mode available for debugging.

Source anchors:

- `neo/framework/Common.cpp`
- `neo/framework/Session.cpp`
- `neo/framework/async/*`
- `neo/sound/snd_system.cpp`
- `neo/renderer/RenderSystem.cpp`

Verification:

- Debug build catches wrong-thread calls.
- Single-thread and async modes both launch.

### 14.2 Job System Introduction

Implementation steps:

1. Add a small job interface but do not migrate gameplay immediately.
2. Start with isolated work: image processing, model tangent generation, sound decoding, and optional tool tasks.
3. Add deterministic completion barriers.
4. Avoid parallelizing `idGameLocal::RunFrame` until event/script/physics order is fully covered by tests.

Source anchors:

- `neo/renderer/Image*`
- `neo/renderer/tr_trisurf.cpp`
- `neo/sound/snd_decoder.cpp`
- `neo/tools/*`

Verification:

- Job-enabled and job-disabled paths produce equivalent results.

## Phase 15: Packaging, Configuration, and User-Facing Polish

Goal: make the modernized engine shippable and understandable for players, modders, and developers.

### 15.1 Configuration Cleanup

Implementation steps:

1. Inventory cvars and commands by subsystem.
2. Mark obsolete cvars with compatibility behavior.
3. Add descriptions for new modernization cvars.
4. Separate developer cvars from user-facing settings.

Source anchors:

- `neo/framework/CVarSystem.*`
- `neo/framework/CmdSystem.*`
- `neo/framework/Common.cpp`
- `neo/renderer/*`
- `neo/sound/*`

Verification:

- `listCvars` and `listCmds` are readable and categorized.
- Old configs still load.

### 15.2 Packaging

Implementation steps:

1. Create a reproducible package layout for executable, game DLLs, config defaults, runtime DLLs, and documentation.
2. Add a packaging script that validates all required files.
3. Add a first-run config path strategy that does not write into shipped content.
4. Add version metadata and crash-log collection path.

Source anchors:

- `neo/framework/FileSystem.cpp`
- `neo/framework/Common.cpp`
- `neo/sys/win32/*`
- Build scripts and CMake packaging files.

Verification:

- Fresh package launches without developer environment variables.
- Missing DLL/content errors are clear.

## Cross-Cutting Test Matrix

Run these gates after every meaningful phase:

1. Build gate.
   - Configure clean.
   - Build Debug and Release where supported.
   - Build runtime-only and tools-enabled configurations separately.

2. Static gate.
   - No new compiler warnings in touched targets.
   - No trailing whitespace or formatting churn.
   - No accidental edits to shipped content.

3. Runtime gate.
   - Windowed menu launch.
   - Map load to first frame.
   - Map restart.
   - Map shutdown.
   - Clean engine shutdown.

4. Content gate.
   - Decl scan.
   - Material scan.
   - Sound shader scan.
   - GUI parse scan.
   - Collision model load smoke.

5. Compatibility gate.
   - Save/load fixture.
   - Demo playback fixture if available.
   - Network snapshot round-trip fixture.
   - Legacy renderer screenshot comparison for renderer changes.

## Recommended Implementation Order

1. Phase 0: baseline capture and harness.
2. Phase 1: build system and platform boundary.
3. Phase 2: 64-bit correctness and typed handles.
4. Phase 3: ownership/lifetime.
5. Phase 5: file system and decl diagnostics.
6. Phase 4: idLib modernization after tests are in place.
7. Phase 6: renderer command/device/resource foundation.
8. Phase 8: game framework, script, and event safety.
9. Phase 9: physics/collision tests and cleanup.
10. Phase 10: sound backend boundary and propagation diagnostics.
11. Phase 11: GUI runtime safety and high-DPI work.
12. Phase 12: networking guards and snapshot tests.
13. Phase 13: tools pipeline modernization.
14. Phase 14: threading/job system.
15. Phase 7: modern rendering features.
16. Phase 15: packaging and polish.

The important sequencing rule is that test harnesses, build reproducibility, file/resource diagnostics, and renderer command isolation come before high-visibility feature work. That keeps modernization from becoming a pile of visual changes on top of unstable engine foundations.

## Initial High-Value Slices

These are the best first implementation slices because they reduce risk for every later phase:

1. Add launch/map smoke scripts and CTest entries.
2. Add source inventories for globals, direct GL calls, direct Win32 calls, serialization fields, and pointer truncation sites.
3. Convert obvious pointer truncation issues documented in `POINTER_64BIT_MIGRATION_REPORT.md`.
4. Add file system search-path diagnostics.
5. Add decl scan strict-report mode.
6. Add renderer command trace mode around `R_RenderView` and `RB_ExecuteBackEndCommands`.
7. Add null sound backend for headless smoke tests.
8. Add GUI parse fixtures for main menu and HUD.
9. Add collision trace fixtures.
10. Add package validation script.

## Definition of Done for a Modernized Core

The engine core should not be considered modernized until these are true:

- Clean, documented CMake-based runtime build.
- Reproducible launch/map smoke automation.
- 64-bit build has no known pointer truncation hazards in active runtime paths.
- File system, decl manager, renderer, sound, UI, physics, game, and networking have focused smoke or unit coverage.
- Renderer command stream can be validated without rendering.
- Legacy OpenGL renderer remains available as a compatibility path.
- Savegame, demo, and network protocol compatibility are explicitly documented.
- Runtime package can launch on a clean machine with clear diagnostics for missing content or dependencies.
- New rendering features are optional and do not replace compatibility rendering until they pass screenshot and gameplay gates.
