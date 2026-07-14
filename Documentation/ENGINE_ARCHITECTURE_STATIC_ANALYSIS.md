# Doom 3 GPL / idTech4 Source Architecture Static Analysis

Scope: this document is extracted from the `neo/` source tree in this checkout. It cites the concrete files, classes, structs, and functions used. Where a requested name is not present in this GPL tree, the nearest source-level equivalent is identified instead of inferred.

## 1. Module Overview

### idLib

Purpose: foundational utility library shared by all engine subsystems.

Responsibilities and subsystems:
- Math, SIMD, geometry, and bounds: `neo/idlib/math/*`, `idVec*`, `idMat*`, `idPlane`, `idBounds`, `idAngles`.
- Containers and strings: `neo/idlib/containers/*`, `idList`, `idHashIndex`, `idHashTable`, `idLinkList`, `idQueue`, `idStack`, `idHierarchy`, `idStr`.
- Parsing and lexing: `neo/idlib/Lexer.*`, `neo/idlib/Parser.*`, `neo/idlib/Token.h`.
- Memory, timers, and library globals: `neo/idlib/Heap.*`, `neo/idlib/Timer.h`, `neo/idlib/Lib.h`.

Key sources: `neo/idlib/Lib.h`, `neo/idlib/Str.h`, `neo/idlib/Parser.h`, `neo/idlib/math/Vector.h`, `neo/idlib/containers/List.h`.

### Framework / idCommon

Purpose: core engine orchestration, command/cvar/console/event systems, files, decls, session, networking bootstrap.

Responsibilities and subsystems:
- `idCommon` public API and `idCommonLocal` implementation: startup, per-frame loop, shutdown, game-DLL loading.
- `idSession` / `idSessionLocal`: map/session control, screen updates, menus, game draw/frame dispatch.
- `idFileSystemLocal`: virtual file system, search paths, `.pk4` zip-backed reads, pure-server checksums.
- `idDeclManagerLocal`: declaration type registry and lazy parsed runtime assets.
- `idCVarSystemLocal`, `idCmdSystemLocal`, `idConsoleLocal`, `idEventLoop`, `idUsercmdGenLocal`.
- Async networking: `idAsyncNetwork`, `idAsyncServer`, `idAsyncClient`, `idMsgChannel`, `idNetworkSystem`.

Key sources: `neo/framework/Common.h`, `neo/framework/Common.cpp`, `neo/framework/Session.h`, `neo/framework/Session_local.h`, `neo/framework/FileSystem.*`, `neo/framework/DeclManager.*`, `neo/framework/async/*`.

### Renderer / idRenderSystem

Purpose: world rendering, material/image/model resources, render command buffering, OpenGL back-end execution, GUI surface generation.

Responsibilities and subsystems:
- Public render APIs: `idRenderSystem`, `idRenderWorld`, `renderEntity_t`, `renderLight_t`, `renderView_t`.
- Local world state: `idRenderWorldLocal`, portal areas, render/light defs, world model refs.
- Front-end scene build: portal flow, entity/light visibility, draw surf creation, subview generation.
- Back-end command execution: command list with `RC_SET_BUFFER`, `RC_DRAW_VIEW`, `RC_COPY_RENDER`, `RC_SWAP_BUFFERS`.
- Material/image/model systems: `idMaterial`, `idImage`, `idImageManager`, `srfTriangles_t`, `idRenderModel`.
- GUI draw surfaces: `idGuiModel`.

Key sources: `neo/renderer/RenderSystem.h`, `neo/renderer/RenderWorld.h`, `neo/renderer/RenderWorld_local.h`, `neo/renderer/tr_local.h`, `neo/renderer/RenderSystem.cpp`, `neo/renderer/tr_main.cpp`, `neo/renderer/tr_backend.cpp`, `neo/renderer/draw_common.cpp`.

### Game / idGame

Purpose: gameplay DLL interface, entity system, scripting/events, players/AI, map spawning, physics ownership.

Responsibilities and subsystems:
- Public game API: `idGame`, `idGameEdit` in `neo/game/Game.h`.
- Runtime game implementation: `idGameLocal` in `neo/game/Game_local.h/.cpp`.
- Entity hierarchy: `idEntity`, `idAnimatedEntity`, `idActor`, `idAI`, `idPlayer`, items, movers, projectiles, lights, FX.
- Script VM and event dispatch: `idProgram`, `idScriptObject`, `idThread`, `idEventDef`, `idEvent`, `idClass`, `idTypeInfo`.
- Local PVS and interaction with render/sound worlds: `idPVS`, `gameRenderWorld`, `gameSoundWorld`.
- Gameplay physics ownership: each entity owns an `idPhysics` implementation.

Key sources: `neo/game/Game.h`, `neo/game/Game_local.h`, `neo/game/Game_local.cpp`, `neo/game/Entity.h`, `neo/game/Entity.cpp`, `neo/game/gamesys/*`, `neo/game/script/*`.

### Physics / Collision

Purpose: game-facing rigid/actor/AF physics plus shared collision-model tracing.

Responsibilities and subsystems:
- Abstract `idPhysics` interface and concrete physics classes in `neo/game/physics`.
- Entity clip models and broadphase querying: `idClip`, `idClipModel`.
- Collision map loading/tracing/contacts: `idCollisionModelManager` and `idCollisionModelManagerLocal` in `neo/cm`.
- Rigid bodies: `idPhysics_RigidBody`.
- Articulated figures: `idAF`, `idPhysics_AF`, `idAFBody`, constraint classes.
- Player/monster/actor/parametric/static physics: `idPhysics_Player`, `idPhysics_Monster`, `idPhysics_Actor`, `idPhysics_Parametric`, `idPhysics_Static`, `idPhysics_StaticMulti`.

Key sources: `neo/game/physics/Physics.h`, `neo/game/physics/Physics_*.h/.cpp`, `neo/game/physics/Clip.h`, `neo/game/physics/Clip.cpp`, `neo/game/AF.*`, `neo/cm/CollisionModel*.cpp`.

### Sound

Purpose: sound shader declarations, sound world/emitter/channel lifecycle, spatialization, portal occlusion, mixing/hardware output.

Responsibilities and subsystems:
- Public APIs: `idSoundSystem`, `idSoundWorld`, `idSoundEmitter`, `idSoundShader`.
- Local implementations: `idSoundSystemLocal`, `idSoundWorldLocal`, `idSoundEmitterLocal`.
- Samples/decoding/cache: `idSoundSample`, `idSampleDecoder`, `idSoundCache`, Ogg/Vorbis code.
- Portal-aware propagation: `idSoundWorldLocal::ResolveOrigin` and `idSoundEmitterLocal::Spatialize` use `idRenderWorld::NumPortalsInArea` and `GetPortal`.
- Sound shaders parse distance, volume, flags, channel behavior, leadins, shake, EAX reverb data.
- Mixing/hardware update: `idSoundSystemLocal::AsyncUpdate`, `AsyncUpdateWrite`, platform audio hardware classes.

Key sources: `neo/sound/sound.h`, `neo/sound/snd_local.h`, `neo/sound/snd_system.cpp`, `neo/sound/snd_world.cpp`, `neo/sound/snd_emitter.cpp`, `neo/sound/snd_shader.cpp`, `neo/sound/snd_cache.cpp`.

### UI / GUI

Purpose: Doom 3 native GUI system: parsed `.gui` window trees, winvars, expression registers, GUI scripts, and rendering through the render system. This is Flash-like in data-driven behavior, but it is not a Flash/Scaleform runtime in this tree.

Responsibilities and subsystems:
- Public API: `idUserInterface`, `idUserInterfaceManager`.
- Local GUI instances: `idUserInterfaceLocal`, `idUserInterfaceManagerLocal`.
- Window hierarchy and widgets: `idWindow`, `idSimpleWindow`, `idListWindow`, `idEditWindow`, `idSliderWindow`, `idChoiceWindow`, `idRenderWindow`, mini-game windows.
- State-bound variables: `idWinVar` and typed derivatives.
- GUI scripting: `idGuiScript`, `idGuiScriptList`, script event slots such as `onAction`.
- Drawing abstraction: `idDeviceContext`, forwarding draw calls to `renderSystem->DrawStretchPic`.

Key sources: `neo/ui/UserInterface.h`, `neo/ui/UserInterfaceLocal.h`, `neo/ui/UserInterface.cpp`, `neo/ui/Window.h`, `neo/ui/Window.cpp`, `neo/ui/Winvar.h`, `neo/ui/DeviceContext.*`, `neo/ui/GuiScript.h`, `neo/ui/RegExp.h`.

### System / Platform

Purpose: OS abstraction for windowing, input, networking sockets, timing, threading, CPU/FPU, DLL loading, console.

Responsibilities and subsystems:
- Platform entry points call `common->Frame()`: `neo/sys/win32/win_main.cpp`, `neo/sys/posix/posix_main.cpp`, `neo/sys/stub/sys_stub.cpp`.
- `idSys` / `idSysLocal` public wrapper in `neo/sys/sys_public.h`, `neo/sys/sys_local.h`.
- Platform implementations under `neo/sys/win32`, `neo/sys/posix`, `neo/sys/linux`, `neo/sys/osx`.

Key sources: `neo/sys/sys_public.h`, `neo/sys/sys_local.h`, `neo/sys/win32/win_main.cpp`, `neo/sys/posix/posix_main.cpp`.

## 2. Class Map

### Framework Classes

- `idCommon` / `idCommonLocal`: engine bootstrap, frame loop, shutdown, printing/errors, game DLL loading. Important methods: `Init`, `Frame`, `Shutdown`, `InitGame`, `ShutdownGame`, `InitRenderSystem`. Sources: `neo/framework/Common.h`, `neo/framework/Common.cpp`.
- `idSession` / `idSessionLocal`: active session, map loading, screen update, menu/game draw, save/load, demos. Important methods: `Frame`, `UpdateScreen`, `Draw`, `Shutdown`, map transition methods. Sources: `neo/framework/Session.h`, `neo/framework/Session_local.h`, `neo/framework/Session.cpp`.
- `idFileSystem` / `idFileSystemLocal`: VFS interface and local search-path implementation. Important methods: `Init`, `BuildOSPath`, `OpenFileRead`, `OpenFileReadFlags`, `OpenFileWrite`, `AddZipFile`, `FindDLL`, `FindFile`. Sources: `neo/framework/FileSystem.h`, `neo/framework/FileSystem.cpp`.
- `idDeclBase`, `idDecl`, `idDeclManager` / `idDeclManagerLocal`: declaration object abstraction, lazy parse state, type/folder registry. Important methods: `RegisterDeclType`, `RegisterDeclFolder`, `FindType`, `DeclByIndex`, `CreateNewDecl`, `FindMaterial`, `FindSound`, `Reload`. Sources: `neo/framework/DeclManager.h`, `neo/framework/DeclManager.cpp`.
- `idCVar`, `idCVarSystem`, `idCVarSystemLocal`; `idCmdSystem`, `idCmdSystemLocal`; `idConsole`, `idConsoleLocal`: runtime console/control plane. Sources: `neo/framework/CVarSystem.*`, `neo/framework/CmdSystem.*`, `neo/framework/Console.*`.
- `idEventLoop`: platform event pumping and event time. Source: `neo/framework/EventLoop.h/.cpp`.
- `idUsercmdGen` / `idUsercmdGenLocal`: user command generation from input. Sources: `neo/framework/UsercmdGen.h/.cpp`.

### Renderer Classes and Structs

- `idRenderSystem` / `idRenderSystemLocal`: public render facade and local command-buffer/render-frame implementation. Important methods: `Init`, `BeginFrame`, `EndFrame`, `DrawStretchPic`, `SetColor`, `AllocRenderWorld`. Sources: `neo/renderer/RenderSystem.h`, `neo/renderer/tr_local.h`, `neo/renderer/RenderSystem.cpp`.
- `idRenderWorld` / `idRenderWorldLocal`: render-world lifecycle, entity/light defs, scene rendering, portal API. Important methods: `InitFromMap`, `RenderScene`, `AddEntityDef`, `UpdateEntityDef`, `AddLightDef`, `SetPortalState`, `AreasAreConnected`, `PointInArea`, `BoundsInAreas`. Sources: `neo/renderer/RenderWorld.h`, `neo/renderer/RenderWorld_local.h`, `neo/renderer/RenderWorld.cpp`, `neo/renderer/RenderWorld_load.cpp`, `neo/renderer/RenderWorld_portals.cpp`.
- `idRenderEntityLocal` and `idRenderLightLocal`: local mutable render definitions for `renderEntity_t` and `renderLight_t`. Sources: `neo/renderer/tr_local.h`, `neo/renderer/RenderEntity.cpp`, `neo/renderer/tr_lightrun.cpp`.
- `viewDef_t`: per-view frame allocation containing render view, frustum, draw surfaces, visible entity/light lists, viewport/scissor. Source: `neo/renderer/tr_local.h`.
- `drawSurf_t`: transient per-frame render surface linking geometry, space, material, shader registers, scissor, sort value. Source: `neo/renderer/tr_local.h`.
- `srfTriangles_t`: renderer geometry payload with bounds, vertices, indexes, caches, shadow data. Source: `neo/renderer/Model.h`.
- `idMaterial`: decl-backed material/shader with stages, expressions, sort, coverage, light/fog/portal flags. Sources: `neo/renderer/Material.h`, `neo/renderer/Material.cpp`.
- `idImage`, `idImageManager`: texture objects and texture manager. Sources: `neo/renderer/Image.h`, `neo/renderer/Image*.cpp`.
- `idGuiModel`: accumulates 2D GUI draw calls into surfaces, emits fullscreen/current-view draw commands. Source: `neo/renderer/GuiModel.cpp`, `neo/renderer/GuiModel.h`.

### Game Classes

- `idGame` / `idGameLocal`: public game DLL API and concrete game state. Important methods: `Init`, `InitFromNewMap`, `InitFromSaveGame`, `MapShutdown`, `LoadMap`, `SpawnMapEntities`, `SpawnEntityDef`, `RunFrame`, `Draw`, `ServerWriteSnapshot`, `ClientReadSnapshot`, `ClientPrediction`. Sources: `neo/game/Game.h`, `neo/game/Game_local.h`, `neo/game/Game_local.cpp`, `neo/game/Game_network.cpp`.
- `idEntity : idClass`: base gameplay object. Owns spawn args, render entity/light/sound state, physics pointer, targets, bind/team state. Important methods: `Spawn`, `Think`, `RunPhysics`, `Present`, `SetPhysics`, `UpdateVisuals`, `WriteToSnapshot`, `ReadFromSnapshot`. Sources: `neo/game/Entity.h`, `neo/game/Entity.cpp`.
- `idAnimatedEntity : idEntity`: adds animation update via `idAnimator`. Source: `neo/game/Entity.h/.cpp`.
- `idActor : idAFEntity_Gibbable`: animated living actor base with health, combat, attachments, IK/AF support, damage. Sources: `neo/game/Actor.h`, `neo/game/Actor.cpp`.
- `idAI : idActor`: monster AI with movement state, AAS pathing, enemy/combat state, script variables/events. Sources: `neo/game/ai/AI.h`, `neo/game/ai/AI.cpp`.
- `idPlayer : idActor`: player command processing, inventory, HUD/view, player physics, networking snapshot state. Important methods: `Think`, `ClientPredictionThink`, `CalculateRenderView`, `WritePlayerStateToSnapshot`. Sources: `neo/game/Player.h`, `neo/game/Player.cpp`.
- `idAF`, `idAFEntity_Base`, `idAFEntity_Gibbable`, vehicle/attachment AF entities: articulated figure gameplay bridge. Sources: `neo/game/AF.h/.cpp`, `neo/game/AFEntity.h/.cpp`.
- `idMultiplayerGame`: multiplayer rules/HUD/game-state companion used by `idGameLocal`. Source: `neo/game/MultiplayerGame.h/.cpp`.

### Game Type/Event/Script Classes

- `idClass`: base for run-time type info, event dispatch, save/restore. Source: `neo/game/gamesys/Class.h/.cpp`.
- `idTypeInfo`: class metadata generated/registered by `CLASS_PROTOTYPE` and `CLASS_DECLARATION`. Source: `neo/game/gamesys/Class.h`.
- `idEventDef`: event signature/name definition. Source: `neo/game/gamesys/Event.h`.
- `idEvent`: allocated/scheduled event instance; `ServiceEvents` dispatches pending callbacks. Source: `neo/game/gamesys/Event.h/.cpp`.
- `idProgram`: script compiler/runtime program, function/variable/type storage, save/restore. Source: `neo/game/script/Script_Program.h`.
- `idScriptObject`: per-entity script object with type/data pointer and function lookup. Source: `neo/game/script/Script_Program.h`.
- `idThread : idClass`: script thread/interpreter wrapper; `Execute` runs VM code and event commands. Sources: `neo/game/script/Script_Thread.h/.cpp`, `neo/game/script/Script_Interpreter.h/.cpp`.
- `idCompiler`, `idInterpreter`: script compile and bytecode execution. Sources: `neo/game/script/Script_Compiler.h/.cpp`, `neo/game/script/Script_Interpreter.h/.cpp`.

### Physics and Collision Classes

- `idPhysics : idClass`: abstract physics contract. Important methods include `Evaluate`, `SetClipModel`, `GetClipModel`, `SetOrigin`, `SetAxis`, `ClipTranslation`, `ClipRotation`, `ClipContents`, `DisableClip`, `EnableClip`, `WriteToSnapshot`, `ReadFromSnapshot`. Source: `neo/game/physics/Physics.h`.
- `idPhysics_Base : idPhysics`: shared physics base for derived movers/actors/AFs. Source: `neo/game/physics/Physics_Base.h/.cpp`.
- `idPhysics_RigidBody : idPhysics_Base`: impulse-based rigid body. Important methods: `Integrate`, `CheckForCollisions`, `Evaluate`, `EvaluateContacts`. Sources: `neo/game/physics/Physics_RigidBody.h/.cpp`.
- `idPhysics_AF : idPhysics_Base`: articulated-figure solver. Important methods: `AddBody`, `AddConstraint`, `EvaluateConstraints`, `EvaluateBodies`, `EvaluateContacts`, `Evaluate`. Sources: `neo/game/physics/Physics_AF.h/.cpp`.
- Constraint classes: `idAFConstraint` base plus fixed, ball/socket, universal, cylindrical, hinge, slider, line, plane, spring, contact, friction, cone, pyramid, suspension constraints. Source: `neo/game/physics/Physics_AF.h/.cpp`.
- `idAFBody`, `idAFTree`: bodies and solver topology. Source: `neo/game/physics/Physics_AF.h/.cpp`.
- `idPhysics_Player`, `idPhysics_Monster`, `idPhysics_Actor`: actor/player/monster movement implementations. Sources: `neo/game/physics/Physics_Player.*`, `Physics_Monster.*`, `Physics_Actor.*`.
- `idPhysics_Parametric`, `idPhysics_Static`, `idPhysics_StaticMulti`: movers/static objects. Sources: `neo/game/physics/Physics_Parametric.*`, `Physics_Static.*`, `Physics_StaticMulti.*`.
- `idClip` and `idClipModel`: game collision broadphase and per-entity clip model wrapper. Source: `neo/game/physics/Clip.h/.cpp`.
- `idCollisionModelManager` / `idCollisionModelManagerLocal`: collision map loading, model handles, contents, translation/rotation traces, contacts. Sources: `neo/cm/CollisionModel.h`, `neo/cm/CollisionModel_local.h`, `neo/cm/CollisionModel_load.cpp`, `CollisionModel_translate.cpp`, `CollisionModel_rotate.cpp`, `CollisionModel_contents.cpp`, `CollisionModel_contacts.cpp`.

### Sound Classes

- `idSoundShader : idDecl`: parsed sound decl containing `soundShaderParms_t`, entries/leadins, flags, speaker mask, alternate sound. Sources: `neo/sound/sound.h`, `neo/sound/snd_shader.cpp`.
- `idSoundEmitter` / `idSoundEmitterLocal`: emitter API and local channel owner. Important methods: `UpdateEmitter`, `StartSound`, `ModifySound`, `StopSound`, `FadeSound`, `Spatialize`, `CheckForCompletion`. Sources: `neo/sound/sound.h`, `neo/sound/snd_local.h`, `neo/sound/snd_emitter.cpp`.
- `idSoundWorld` / `idSoundWorldLocal`: sound world per map/session. Important methods: `PlaceListener`, `ForegroundUpdate`, `ResolveOrigin`, `FindAmplitude`, save/restore, demo I/O, emitter lookup/allocation. Sources: `neo/sound/sound.h`, `neo/sound/snd_local.h`, `neo/sound/snd_world.cpp`.
- `idSoundSystem` / `idSoundSystemLocal`: global sound system and mixer/hardware owner. Important methods: `Init`, `Shutdown`, `AsyncUpdate`, `AllocSoundWorld`, `SetPlayingSoundWorld`. Sources: `neo/sound/sound.h`, `neo/sound/snd_local.h`, `neo/sound/snd_system.cpp`.
- `idSoundChannel`, `idSlowChannel`, `idSoundFade`, `idSoundSample`, `idSampleDecoder`, `idSoundCache`: channel playback/mixing/sample cache/decoding. Source: `neo/sound/snd_local.h`, `neo/sound/snd_cache.cpp`, `neo/sound/snd_decoder.cpp`.

### UI Classes

- `idUserInterface` / `idUserInterfaceLocal`: GUI instance API and local desktop/state manager. Important methods: `InitFromFile`, `HandleEvent`, `Redraw`, `StateChanged`, `DrawCursor`. Sources: `neo/ui/UserInterface.h`, `neo/ui/UserInterfaceLocal.h`, `neo/ui/UserInterface.cpp`.
- `idUserInterfaceManager` / `idUserInterfaceManagerLocal`: GUI loading/cache manager. Source: `neo/ui/UserInterface.h`, `neo/ui/UserInterfaceLocal.h`.
- `idWindow`: base GUI window tree node. Important methods: `Parse`, `ParseScript`, `ParseRegEntry`, `ParseExpression`, `HandleEvent`, `Redraw`, `RunScript`, `Activate`, `DrawBackground`, `DrawBorderAndCaption`. Sources: `neo/ui/Window.h`, `neo/ui/Window.cpp`.
- `idDeviceContext`: GUI drawing context using a virtual 640x480 coordinate system and render-system draw calls. Important methods: `Init`, `SetSize`, `DrawMaterial`, `DrawRect`, `DrawText`. Sources: `neo/ui/DeviceContext.h`, `neo/ui/DeviceContext.cpp`.
- `idWinVar` and typed derivatives: GUI state-bound variables for bool/string/int/float/rect/vector/background. Source: `neo/ui/Winvar.h`, `neo/ui/Winvar.cpp`.
- `idGuiScript`, `idGuiScriptList`: parsed GUI script commands. Source: `neo/ui/GuiScript.h`.
- `idRegister`, `idRegisterList`: GUI expression register storage. Source: `neo/ui/RegExp.h`.
- Widget subclasses: `idBindWindow`, `idChoiceWindow`, `idEditWindow`, `idFieldWindow`, `idListWindow`, `idSliderWindow`, `idRenderWindow`, `idMarkerWindow`, and game windows. Sources: `neo/ui/*Window.h/.cpp`.

### Networking Classes

- `idNetworkSystem`: game-facing reliable-message bridge to async server/client. Sources: `neo/framework/async/NetworkSystem.h`, `neo/framework/async/NetworkSystem.cpp`.
- `idAsyncNetwork`: static async network coordinator and entry point. Important methods: `Init`, `Shutdown`, `RunFrame`, `ExecuteSessionCommand`. Sources: `neo/framework/async/AsyncNetwork.h/.cpp`.
- `idAsyncServer`: server connection/challenge/client loop/snapshot sender. Important methods: `RunFrame`, `ProcessMessage`, `LocalClientInput`. Sources: `neo/framework/async/AsyncServer.h/.cpp`.
- `idAsyncClient`: client connection/usercmd/snapshot/prediction loop. Important methods: `RunFrame`, `SendUsercmdsToServer`, `DuplicateUsercmds`, `ProcessMessage`. Sources: `neo/framework/async/AsyncClient.h/.cpp`.
- `idMsgChannel`, `idMsgQueue`: sequenced/reliable/fragments message transport. Source: `neo/framework/async/MsgChannel.h/.cpp`.
- `idServerScan`: LAN/master server listing. Source: `neo/framework/async/ServerScan.h/.cpp`.
- No `idLobby` class exists in this GPL tree (`rg idLobby neo` returns no matches). The lobby-like responsibilities are split across `idSessionLocal`, `idAsyncNetwork`, `idAsyncServer`, `idAsyncClient`, and server scan code.

## 3. Rendering Architecture

### High-Level Flow

Source path:

1. Game/session calls `idGameLocal::Draw`.
2. The player produces a `renderView_t` via `idPlayer::CalculateRenderView` / `GetRenderView`.
3. `idGameLocal::Draw` calls the render world to render the player view and places listener state for sound.
4. `idRenderWorldLocal::RenderScene` allocates/fills a `viewDef_t`.
5. Front-end builds visible lights/entities/surfaces and appends `RC_DRAW_VIEW`.
6. `idRenderSystemLocal::EndFrame` appends `RC_SWAP_BUFFERS` and calls `R_IssueRenderCommands`.
7. `RB_ExecuteBackEndCommands` executes commands in order.

Key sources:
- `neo/game/Game_local.cpp:idGameLocal::Draw`.
- `neo/game/Player.cpp:idPlayer::CalculateRenderView`, `idPlayer::GetRenderView`.
- `neo/renderer/RenderWorld.cpp:idRenderWorldLocal::RenderScene`.
- `neo/renderer/tr_main.cpp:R_RenderView`.
- `neo/renderer/RenderSystem.cpp:R_GetCommandBuffer`, `R_AddDrawViewCmd`, `R_IssueRenderCommands`, `idRenderSystemLocal::BeginFrame`, `EndFrame`.
- `neo/renderer/tr_backend.cpp:RB_ExecuteBackEndCommands`.

### Front-End Responsibilities

The renderer front-end is the game/main-thread scene builder. It uses the render world, current `renderView_t`, portal visibility, and dynamic entity/light state to build transient `viewDef_t`, `viewEntity_t`, `viewLight_t`, `drawSurf_t`, and render commands.

Important source functions:
- `idRenderWorldLocal::RenderScene` in `neo/renderer/RenderWorld.cpp`: copies render view, prepares viewport/scissor, transforms view, calls `R_RenderView`.
- `R_RenderView` in `neo/renderer/tr_main.cpp`: view orchestration.
- `idRenderWorldLocal::FlowViewThroughPortals` in `neo/renderer/RenderWorld_portals.cpp`: marks visible areas and gathers view entities/lights through portals.
- `R_AddLightSurfaces` in `neo/renderer/tr_light.cpp`: evaluates light shaders, creates interactions/shadow surfaces.
- `R_AddModelSurfaces` in `neo/renderer/tr_light.cpp`: instantiates dynamic models and creates ambient draw surfaces and interaction surfaces.
- `R_AddDrawSurf` in `neo/renderer/tr_light.cpp`: creates a `drawSurf_t`.
- `R_SortDrawSurfs` in `neo/renderer/tr_main.cpp`: sorts ambient draw surfaces.
- `R_GenerateSubViews` in `neo/renderer/tr_main.cpp`: handles mirrors/remote cameras/dynamic textures before main view command submission.
- `R_AddDrawViewCmd` in `neo/renderer/RenderSystem.cpp`: emits `RC_DRAW_VIEW`.

Actual front-end order in `R_RenderView`:

1. Validate view dimensions.
2. Save old view and set `tr.viewDef`.
3. Set frame/view counters and view matrix/frustum state.
4. Flow visibility through portals.
5. `R_AddLightSurfaces()`.
6. `R_AddModelSurfaces()`.
7. `R_RemoveUnecessaryViewLights()`.
8. `R_SortDrawSurfs()`.
9. `R_GenerateSubViews()`.
10. Demo visible-def write if recording.
11. `R_AddDrawViewCmd(parms)`.
12. Restore previous view.

Sources: `neo/renderer/tr_main.cpp:R_RenderView`, `neo/renderer/tr_light.cpp:R_AddLightSurfaces`, `R_AddModelSurfaces`, `R_AddDrawSurf`.

### Back-End Responsibilities

The back-end executes render commands and issues OpenGL drawing. It consumes front-end command buffers and does not modify front-end world state except through frame-owned structures.

Actual command execution order in `RB_ExecuteBackEndCommands` (`neo/renderer/tr_backend.cpp`):

```text
for each command:
  RC_NOP          -> no-op
  RC_DRAW_VIEW    -> RB_DrawView(command)
  RC_SET_BUFFER   -> RB_SetBuffer(command)
  RC_SWAP_BUFFERS -> RB_SwapBuffers(command)
  RC_COPY_RENDER  -> RB_CopyRender(command)
```

Command emission sites:
- `RC_SET_BUFFER`: `idRenderSystemLocal::BeginFrame`.
- `RC_DRAW_VIEW`: `R_AddDrawViewCmd` from `R_RenderView` and `idGuiModel::EmitFullScreen`.
- `RC_COPY_RENDER`: `idRenderSystemLocal::CaptureRenderToImage`.
- `RC_SWAP_BUFFERS`: `idRenderSystemLocal::EndFrame`.

Back-end view draw order:
- `RB_DrawView` in `neo/renderer/tr_render.cpp` sets `backEnd.viewDef`, handles skip/debug cases, then calls `RB_STD_DrawView`.
- `RB_STD_DrawView` in `neo/renderer/draw_common.cpp`:
  1. `RB_BeginDrawingView()`.
  2. `RB_DetermineLightScale()`.
  3. `RB_STD_FillDepthBuffer(drawSurfs, numDrawSurfs)`.
  4. Hardware-specific interaction pass:
     - `RB_ARB_DrawInteractions()`, or
     - `RB_ARB2_DrawInteractions()`, or
     - `RB_R200_DrawInteractions()`, etc.
  5. Disable stencil shadow test.
  6. `RB_STD_LightScale()`.
  7. `RB_STD_DrawShaderPasses(...)` for non-light-dependent material stages.
  8. Fog/blend/debug tools after the main draw path.

Key sources: `neo/renderer/tr_backend.cpp`, `neo/renderer/tr_render.cpp`, `neo/renderer/draw_common.cpp`, `neo/renderer/draw_arb.cpp`.

### Key Rendering Data Types

- `renderEntity_t`: public entity render parameters, model, bounds, GUI surfaces, shader parms, callbacks. Source: `neo/renderer/RenderWorld.h`.
- `renderLight_t`: public light parameters, origin/target/right/up/start/end, shader, prelight model. Source: `neo/renderer/RenderWorld.h`.
- `idRenderWorldLocal`: owns `portalAreas`, `areaNodes`, `doublePortals`, entity refs, light refs. Source: `neo/renderer/RenderWorld_local.h`.
- `idDrawVert`: packed render vertex type used by surfaces and GUI geometry. Sources: `neo/idlib/geometry/DrawVert.h` and render use in `neo/renderer/GuiModel.cpp`.
- `srfTriangles_t`: renderer triangle geometry. Source: `neo/renderer/Model.h`.
- `drawSurf_t`: per-frame draw item. Source: `neo/renderer/tr_local.h`.
- `idMaterial`: material decl and shader stages. Source: `neo/renderer/Material.h/.cpp`.
- `idImage`: texture object. Source: `neo/renderer/Image.h`.

## 4. idGame Framework

### Game Initialization and Map Loading

`idGameLocal::Init` initializes game-global systems and starts default scripts with `program.Startup(SCRIPT_DEFAULT)`. `idGameLocal::InitFromNewMap` stores the render/sound worlds, calls `LoadMap`, initializes map scripts, populates entities, and resets multiplayer state. `LoadMap` clears sound emitters, initializes async network data, parses the `.map`, loads collision via `collisionModelManager->LoadMap(mapFile)`, initializes `clip` and `pvs`, and loads AAS files. Sources: `neo/game/Game_local.cpp:idGameLocal::Init`, `LoadMap`, `InitFromNewMap`.

### Entity and Script Spawn

`SpawnMapEntities` reads `idMapFile` entities, spawns worldspawn first at `ENTITYNUM_WORLD`, then iterates remaining map entities, precaches media, and calls `SpawnEntityDef`. `SpawnEntityDef` looks up the `classname`, entity def, and class type; it can also call a script `spawnfunc` through `idProgram::FindFunction`. Sources: `neo/game/Game_local.cpp:SpawnMapEntities`, `SpawnEntityDef`.

### Per-Frame Game Logic

Actual `idGameLocal::RunFrame` order:

1. Handle debug/single-player stop-time case.
2. Copy client `usercmd_t` array into `usercmds`.
3. Update render view timing for GUI videos via `gameRenderWorld->SetRenderView`.
4. Clear debug lines/polygons.
5. Sort active entities if physics team masters/pushers require ordering.
6. Iterate `activeEntities` and call `ent->Think()` unless cinematic filtering updates only physics time.
7. Remove entities that became inactive.
8. `idEvent::ServiceEvents()`.
9. Free player PVS handles.
10. Multiplayer/game return command handling and debug drawing.

Sources: `neo/game/Game_local.cpp:idGameLocal::RunFrame`, `SortActiveEntityList`; `neo/game/Entity.cpp:idEntity::Think`.

`idEntity::Think` is minimal at the base level: it calls `RunPhysics()` then `Present()`. `idAnimatedEntity::Think` adds animation update and damage effects. `idAI::Think` and `idPlayer::Think` override with AI/player-specific behavior. Sources: `neo/game/Entity.cpp`, `neo/game/ai/AI.cpp`, `neo/game/Player.cpp`.

### Events

`idEventDef` defines event name/signature. `idEvent::Alloc` packages arguments into event storage, `idEvent::Schedule` queues an event against an `idClass` object and type, and `idEvent::ServiceEvents` pops due events, unmarshals arguments, calls callbacks, and returns events to the free list. Sources: `neo/game/gamesys/Event.h`, `neo/game/gamesys/Event.cpp`.

### Scripting

`idProgram` owns compiled script data, type/var/function definitions, and save/restore. `idScriptObject` binds entity instances to script object data and function lookup. `idThread` wraps interpreter execution; `idThread::Execute` runs until completion/wait/error and thread events expose script commands such as spawn, wait, trace, cvar, debug draw, and music. Sources: `neo/game/script/Script_Program.h`, `neo/game/script/Script_Thread.h/.cpp`, `neo/game/script/Script_Interpreter.h/.cpp`.

## 5. Physics System

### Ownership Model

Game entities own `idPhysics` objects. `idEntity::SetPhysics` replaces the object, clears contacts, sets self pointers, and updates clip state. `idEntity::RunPhysics` calls the physics object's `Evaluate` over the current game time and updates visuals. Sources: `neo/game/Entity.cpp:idEntity::SetPhysics`, `RunPhysics`; `neo/game/physics/Physics.h`.

### Collision and Clip

`idClip` provides game-facing broadphase and tracing. It stores/queries `idClipModel` instances, exposes `Translation`, `Rotation`, `Contents`, `EntitiesTouchingBounds`, and `ClipModelsTouchingBounds`, and delegates narrow-phase traces to `collisionModelManager`. Sources: `neo/game/physics/Clip.h`, `neo/game/physics/Clip.cpp`.

`idCollisionModelManagerLocal` is the collision model backend. It loads map collision data from `idMapFile`, builds/loads collision models, and implements point contents, trace-model contents, translation, rotation, contacts, model info, and debug drawing. Sources: `neo/cm/CollisionModel.h`, `neo/cm/CollisionModel_local.h`, `neo/cm/CollisionModel_load.cpp`, `CollisionModel_translate.cpp`, `CollisionModel_rotate.cpp`, `CollisionModel_contents.cpp`, `CollisionModel_contacts.cpp`.

### Rigid Body

`idPhysics_RigidBody` integrates a rigid-body state, checks collision between current and next state, applies impulse response at impact, and evaluates contacts. The source explicitly describes `Evaluate` as impulse-based rigid body physics where remaining time after collision is ignored. Sources: `neo/game/physics/Physics_RigidBody.h`, `neo/game/physics/Physics_RigidBody.cpp:Integrate`, `CheckForCollisions`, `Evaluate`, `EvaluateContacts`.

### Articulated Figures

`idAF` loads AF declarations, creates bodies/constraints from `idDeclAF`, starts/stops AF behavior, and updates animation joints from physics bodies. `idPhysics_AF` solves bodies and constraints, evaluates contacts, sets up collision for bodies, and exposes `AddBody` / `AddConstraint`. Constraint classes in `Physics_AF.h` model fixed, ball/socket, universal, hinge, slider, spring, contact/friction, cone/pyramid limits, and suspension constraints. Sources: `neo/game/AF.cpp`, `neo/game/AF.h`, `neo/game/physics/Physics_AF.h/.cpp`, `neo/framework/DeclAF.*`.

## 6. Sound System

### Sound Shader and Emitter Flow

`idSoundShader` is a decl type registered as `DECL_SOUND`. `idSoundShader::ParseShader` parses defaults and tokens such as `mindistance`, `maxdistance`, `volume`, `shakes`, `soundClass`, `looping`, `no_occlusion`, `private`, `global`, `omnidirectional`, and `.wav`/`.ogg` entries. Sources: `neo/sound/sound.h`, `neo/sound/snd_shader.cpp`, `neo/framework/DeclManager.cpp:idDeclManagerLocal::Init`.

`idSoundEmitterLocal::StartSound` combines shader parms with emitter overrides, applies sound flags, chooses leadin/entry samples, allocates a channel, fills `idSoundChannel` fields, and starts playback. `ModifySound` merges new parms into matching active channels. Sources: `neo/sound/snd_emitter.cpp`.

### Sound World, Propagation, and Reverb

`idSoundWorldLocal::PlaceListener` is called on the main thread with origin/axis/listener id/game time/area name. `ForegroundUpdate` updates emitters each sound frame. `idSoundEmitterLocal::Spatialize` calculates distance and asks `idSoundWorldLocal::ResolveOrigin` to find a portal path unless the sound is global/no-occlusion. Sources: `neo/sound/snd_world.cpp`, `neo/sound/snd_emitter.cpp`.

Portal propagation is implemented directly against the render-world portal API. `ResolveOrigin` recursively flows through portals to determine whether a sound is blocked by closed portals and what virtual origin/distance should be used; `sound.h` defines `SSF_NO_OCCLUSION`; `RenderWorld.h` exposes `NumPortalsInArea` and `GetPortal` for sound. Sources: `neo/sound/snd_world.cpp:idSoundWorldLocal::ResolveOrigin`, `neo/sound/sound.h`, `neo/renderer/RenderWorld.h`.

Reverb support appears in two places:
- Sound shaders parse a `reverb` token in `neo/sound/snd_shader.cpp`.
- EAX/OpenAL reverb is guarded by `ID_OPENAL` and cvars such as `s_useEAXReverb`, `s_muteEAXReverb`; `snd_world.cpp` updates EAX environment parameters from area effects when enabled.
- Software FX cvars `s_reverbTime` and `s_reverbFeedback` are used by `SoundFX_Comb` in `neo/sound/snd_system.cpp`.

### Mixing and Hardware

`idSoundSystemLocal::Init` initializes global sound state and hardware/cache. `AsyncUpdate` is called by async sound thread or by `idSessionLocal::Frame` when `com_asyncSound == 0`. `AllocSoundWorld` creates an `idSoundWorldLocal` bound to an `idRenderWorld`. Sources: `neo/sound/snd_system.cpp`, `neo/framework/Session.cpp:idSessionLocal::Frame`.

## 7. GUI / User Interface

### GUI Data Model

`idUserInterfaceLocal::InitFromFile` loads a GUI file into a desktop `idWindow`. GUI state is stored in an `idDict` exposed through `State`, `SetStateString`, `StateChanged`, and winvars. Sources: `neo/ui/UserInterface.cpp`, `neo/ui/UserInterfaceLocal.h`.

`idWindow::Parse` parses window definitions, internal vars, child windows, script blocks, and expression registers. `idWindow::ParseScript`, `ParseScriptEntry`, `ParseRegEntry`, `ParseExpression`, and `EvalRegs` implement the script/expression side. Sources: `neo/ui/Window.cpp`, `neo/ui/Window.h`, `neo/ui/RegExp.h`, `neo/ui/GuiScript.h`.

### Events and Rendering

`idUserInterfaceLocal::HandleEvent` routes system events to the desktop window. `idWindow::HandleEvent` implements mouse/key/focus/action routing and runs scripts. `idUserInterfaceLocal::Redraw` calls `desktop->Redraw`, and `idWindow::Redraw` draws background, border, text, material, child windows, and script-driven state. Sources: `neo/ui/UserInterface.cpp`, `neo/ui/Window.cpp`.

`idDeviceContext` owns the virtual 640x480 GUI coordinate system and drawing helpers. `DrawMaterial`, `DrawRect`, and `DrawText` call render-system methods such as `renderSystem->SetColor` and `DrawStretchPic`. Render-system GUI drawing is collected by `idGuiModel` and emitted into render commands. Sources: `neo/ui/DeviceContext.cpp`, `neo/renderer/RenderSystem.cpp`, `neo/renderer/GuiModel.cpp`.

### Entity GUI Integration

`renderEntity_t` has `idUserInterface *gui[MAX_RENDERENTITY_GUI]`. `idEntity` loads GUI paths from spawn args, updates GUI parms from entity state, triggers GUIs, writes GUI network state, and pushes GUI parm changes through event methods. Sources: `neo/renderer/RenderWorld.h`, `neo/game/Entity.cpp`.

## 8. Networking

### Architecture

This GPL tree uses async networking rather than an `idLobby` class. The high-level layers are:

```text
idSessionLocal
  -> idAsyncNetwork::RunFrame
       -> idAsyncServer::RunFrame
       -> idAsyncClient::RunFrame
  -> idNetworkSystem bridge for game reliable messages
  -> idGameLocal snapshot/usercmd APIs
```

Sources: `neo/framework/async/AsyncNetwork.*`, `AsyncServer.*`, `AsyncClient.*`, `NetworkSystem.*`, `neo/game/Game_network.cpp`.

### Snapshot Networking

Server snapshot flow in `idGameLocal::ServerWriteSnapshot`:
- Free snapshots older than `sequence - 64`.
- Allocate a new `snapshot_t`.
- Compute client PVS from player/spectated player bounds.
- Iterate `spawnedEntities`.
- Skip entities outside PVS or not marked `fl.networkSync`.
- For changed entities, write entity number, spawn id, class type number, remapped entityDef, and `ent->WriteToSnapshot(deltaMsg)`.
- Write end marker `ENTITYNUM_NONE`.
- Delta-write PVS words.
- Write player/game state through `WritePlayerStateToSnapshot` and `WriteGameStateToSnapshot`.

Sources: `neo/game/Game_network.cpp:idGameLocal::ServerWriteSnapshot`, `neo/game/Player.cpp:idPlayer::WriteToSnapshot`, `WritePlayerStateToSnapshot`.

Client snapshot flow in `idGameLocal::ClientReadSnapshot`:
- Allocate a new client snapshot.
- Read entity entries until `ENTITYNUM_NONE`.
- Reconstruct/create entities from spawn id, type number, and entity def.
- Call `ent->ReadFromSnapshot(deltaMsg)`.
- Read PVS delta words.
- Add unchanged entities still in snapshot PVS from base states.
- Read player/game state with `ReadPlayerStateFromSnapshot` and `ReadGameStateFromSnapshot`.
- Process snapshot events.

Sources: `neo/game/Game_network.cpp:idGameLocal::ClientReadSnapshot`, `ClientApplySnapshot`, `neo/game/Player.cpp:idPlayer::ReadFromSnapshot`, `ReadPlayerStateFromSnapshot`.

### Client Prediction and Delta Compression

Client prediction is in `idGameLocal::ClientPrediction`: it copies predicted `usercmds`, iterates `snapshotEntities`, marks `TH_PHYSICS`, calls `ent->ClientPredictionThink`, services events, and returns a `gameReturn_t`. `idAsyncClient::RunFrame` calls this while advancing snapshot game frame/time. Sources: `neo/game/Game_network.cpp:idGameLocal::ClientPrediction`, `neo/framework/async/AsyncClient.cpp`.

Delta compression is represented by `idBitMsgDelta`, used in `ServerWriteSnapshot` and `ClientReadSnapshot` with base entity states stored per client/entity in `clientEntityStates`. Sources: `neo/game/Game_network.cpp`, `neo/framework/async/MsgChannel.*`.

Reliable messages are bridged by `idNetworkSystem::ServerSendReliableMessage`, `ServerSendReliableMessageExcluding`, and `ClientSendReliableMessage`, which call into `idAsyncServer` / `idAsyncClient`. Source: `neo/framework/async/NetworkSystem.cpp`.

## 9. Engine Execution and Game Loop

### Main Loop

Platform entry points call `common->Frame()` repeatedly:
- Windows: `neo/sys/win32/win_main.cpp` calls `common->Frame()` in the main loop.
- Stub: `neo/sys/stub/sys_stub.cpp` calls `common->Async()` and `common->Frame()` in an infinite loop.
- POSIX code has the same platform-system role under `neo/sys/posix`.

### Initialization Order

`idCommonLocal::Init`:
- Sets `idLib` interface pointers.
- Initializes command line/startup infrastructure.
- Initializes system/platform services with `Sys_Init` and networking.
- Initializes cvars, commands, event loop, file/decl/game/render/sound/session subsystems through the framework startup path.
- Calls `InitGame` for file system, decls, render, sound, UI/session/game support.

`idCommonLocal::InitGame` order from source:
1. `fileSystem->Init()`.
2. `declManager->Init()`.
3. Subsystems such as render system, sound system, UI manager, session/game are initialized from this framework path.

Sources: `neo/framework/Common.cpp:idCommonLocal::Init`, `InitGame`, `InitRenderSystem`.

### Per-Frame Order

`idCommonLocal::Frame` source order:
1. `Sys_GenerateEvents()`.
2. Process console/event loop work.
3. Async/network frame work through `idAsyncNetwork`.
4. Session frame through `session->Frame()`.
5. Error/FPU checks and timing.

`idSessionLocal::Frame`:
1. If non-async sound, `soundSystem->AsyncUpdate(Sys_Milliseconds())`.
2. Tool/editor takeover checks.
3. Async networking/session state handling.
4. Local user command and map/game handling.
5. Calls `game->RunFrame`.
6. Executes session commands returned by game.
7. Calls `UpdateScreen` as needed.

`idSessionLocal::UpdateScreen`:
1. `renderSystem->BeginFrame(...)`.
2. `Draw()`.
3. `renderSystem->EndFrame(...)`.

`idSessionLocal::Draw`:
- Draws active menu GUI or calls `game->Draw(GetLocalClientNum())`, then GUI overlays/console as needed.

Sources: `neo/framework/Common.cpp:idCommonLocal::Frame`, `neo/framework/Session.cpp:idSessionLocal::Frame`, `UpdateScreen`, `Draw`.

### Shutdown Order

`idCommonLocal::Shutdown` kills async server/client, calls `ShutdownGame(false)`, and `Sys_Shutdown()`. `ShutdownGame` kills playing sound world first, then shuts down game/session/UI/sound/render/decl/filesystem style subsystems in the framework shutdown path. `idGameLocal::MapShutdown` clears debug render state, clears map entities, restarts scripts, shuts down smoke particles/PVS/clip/async network, clears render/sound world pointers, and returns to `GAMESTATE_NOMAP`. Sources: `neo/framework/Common.cpp:idCommonLocal::Shutdown`, `ShutdownGame`; `neo/game/Game_local.cpp:MapShutdown`.

## 10. File System and Resource Management

### Virtual File System

`idFileSystemLocal` stores a linked list of `searchpath_t`, each containing either a `pack_t` or `directory_t`. The source comments state that `.pk4` files are normal zip files, searched in descending pak order, and take precedence over loose files. Search order includes current game/mod, `fs_game_base`, and base game. Sources: `neo/framework/FileSystem.cpp` header comments and `searchpath_t`/`pack_t` definitions.

The requested `idPakFile` and `idZip` class names do not exist in this tree. The code uses:
- `pack_t` for loaded `.pk4` metadata.
- `fileInPack_t` for individual zip entries.
- `idFile_InZip : idFile` for reading files out of zip/pk4 packages.
- zlib/minizip-style `unz*` APIs in `neo/framework/Unzip.cpp`.

Sources: `neo/framework/FileSystem.cpp`, `neo/framework/File.h`, `neo/framework/File.cpp`, `neo/framework/Unzip.cpp`.

### File Read/Write Path

`idFileSystemLocal::OpenFileReadFlags` searches paks and directories according to flags, pure-server constraints, binary-only filters, and addon search. Pak reads call `ReadFileFromZip`, which reopens the pak, sets the zip current-file position, opens the current file, and returns `idFile_InZip`. Loose reads return permanent files. Writes go through `OpenFileWrite` under save/base path. Sources: `neo/framework/FileSystem.cpp:OpenFileReadFlags`, `ReadFileFromZip`, `OpenFileRead`, `OpenFileWrite`; `neo/framework/File.cpp:idFile_InZip`.

### Declarations

`idDeclManagerLocal::Init` registers decl types:
- `table`, `material`, `skin`, `sound`, `entityDef`, `mapDef`, `fx`, `particle`, `articulatedFigure`, `pda`, `email`, `video`, `audio`.

It registers decl folders:
- `materials/*.mtr` as material decls.
- `skins/*.skin` as skin decls.
- `sound/*.sndshd` as sound decls.

`idDeclFile` parses decl files with `DECL_LEXER_FLAGS`, identifies decl type/name, stores `idDeclLocal` entries, and `FindType` forces parse on external access. `FindMaterial` and `FindSound` are thin wrappers over `FindType`. Sources: `neo/framework/DeclManager.h`, `neo/framework/DeclManager.cpp:idDeclManagerLocal::Init`, `FindType`, `DeclByIndex`, `CreateNewDecl`, `FindMaterial`, `FindSound`.

### Asset Chain

Typical path:

```text
fileSystem -> idFile/idFile_InZip -> declManager -> idDecl subclass
  material -> idMaterial -> idImage/imageManager
  sound    -> idSoundShader -> idSoundCache/idSoundSample
  entityDef -> idGameLocal::SpawnEntityDef -> idEntity subclass
  articulatedFigure -> idDeclAF -> idAF/idPhysics_AF
```

This is visible in `DeclManager.cpp` registration, `Material.cpp`, `snd_shader.cpp`, `Game_local.cpp:SpawnEntityDef`, and `AF.cpp:Load`.

## 11. BSP + Portal System

### Source Types

The requested `idArea`, `idPortal`, and `idBSPNode` class names do not exist as classes in this tree. The renderer uses C structs:

- `portal_t`: winding, plane, target area, double portal pointer, next link.
- `doublePortal_t`: two `portal_t` sides, blocking bits, fog/light metadata.
- `portalArea_t`: per-area entity/light refs, portal list, connected-area state.
- `areaNode_t`: BSP-like area tree node used by `PointInArea` and `BoundsInAreas`.

Sources: `neo/renderer/RenderWorld_local.h`.

### Load-Time Portal Data

`idRenderWorldLocal::ParseInterAreaPortals` reads `numPortalAreas`, allocates `portalAreas` and `areaScreenRect`, reads inter-area portals, creates two directed `portal_t` entries per `doublePortal_t`, links them into each area's portal list, and then loads `areaNodes`. `ClearPortalStates` initializes all portals open. Sources: `neo/renderer/RenderWorld_load.cpp`.

### Visibility Traversal

View visibility:
- `idRenderWorldLocal::FindViewLightsAndEntities` sets the view area with `PointInArea`, handles outside/disabled portal cases, initializes connected-area state, and calls `FlowViewThroughPortals`.
- `FlowViewThroughPortals` sets up a `portalStack_t` and calls `FloodViewThroughArea_r`.
- `FloodViewThroughArea_r` skips `PS_BLOCK_VIEW` portals, clips portal windings against the current portal plane stack, tests fog occlusion via `PortalIsFoggedOut`, adds new portal planes, and recurses into the destination area.
- Entity/light culling uses `CullEntityByPortals` and `CullLightByPortals`.

Sources: `neo/renderer/RenderWorld_portals.cpp`.

### Light Portal Flow and Frustum Clipping

Light flow:
- `R_CreateLightDefFogPortals` links fogged portals to fog lights.
- `R_DeriveLightData` / light update code assigns light area and, when `r_useLightPortalFlow` and a prelight model/shadows apply, calls `light->world->FlowLightThroughPortals(light)`.
- `FlowLightThroughPortals` builds initial frustum planes from the light frustum, then recursively flows through area portals using portal plane stacks and light culling.

Sources: `neo/renderer/tr_lightrun.cpp`, `neo/renderer/RenderWorld_portals.cpp`, `neo/renderer/tr_light.cpp`.

### Game and Sound Use of Portals

Game systems use render-world portal state:
- Doors/game entities identify and close portals through `FindPortal` and `SetPortalState`.
- Location propagation uses `AreasAreConnected(..., PS_BLOCK_LOCATION)`.
- PVS uses `NumPortalsInArea`, `GetPortal`, and `PS_BLOCK_VIEW`.
- Sound propagation uses `NumPortalsInArea` and `GetPortal` to flow sound through open portals, respecting `SSF_NO_OCCLUSION`.

Sources: `neo/renderer/RenderWorld.h`, `neo/game/Game_local.cpp`, `neo/game/Pvs.cpp`, `neo/sound/snd_world.cpp`.
