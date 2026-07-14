# Global Singleton Inventory

This inventory records global singleton-style services and manager objects found in the current source tree. It focuses on long-lived interface pointers, backing `*Local` instances, static service bridges, and editor/tool application globals. It intentionally excludes plain CVars, math constants, event/type-definition tables, OpenGL/Cg function pointers, and one-off function-local statics.

## Core Engine Services

| Public access point | Backing storage | Declaration | Definition | Notes |
| --- | --- | --- | --- | --- |
| `sys` | `sysLocal` | `neo/sys/sys_public.h:586` | `neo/sys/sys_local.cpp:40-41` | Platform/system API singleton; also assigned into `idLib::sys`. |
| `common` | `commonLocal` | `neo/framework/Common.h:213` | `neo/framework/Common.cpp:205-206` | Main engine coordinator for init, frame, printing, errors, and shutdown. |
| `cmdSystem` | `cmdSystemLocal` | `neo/framework/CmdSystem.h:128` | `neo/framework/CmdSystem.cpp:115-116` | Global command registration and dispatch. |
| `cvarSystem` | `localCVarSystem` | `neo/framework/CVarSystem.h:262` | `neo/framework/CVarSystem.cpp:486-487` | Global CVar registry and mutation point. |
| `fileSystem` | `fileSystemLocal` | `neo/framework/FileSystem.h:287` | `neo/framework/FileSystem.cpp:499-500` | Global virtual filesystem and pak search path owner. |
| `declManager` | `declManagerLocal` | `neo/framework/DeclManager.h:323` | `neo/framework/DeclManager.cpp:248-249` | Global declaration parser/cache. |
| `console` | `localConsole` | `neo/framework/Console.h:70` | `neo/framework/Console.cpp:121-122` | In-game console UI singleton. |
| `eventLoop` | `eventLoopLocal` | `neo/framework/EventLoop.h:86` | `neo/framework/EventLoop.cpp:34-35` | Global system-event pump and journal owner. |
| `session` | `sessLocal` | `neo/framework/Session.h:163` | `neo/framework/Session.cpp:47-48` | Global game/session/menu/lifecycle coordinator. |
| `usercmdGen` | `localUsercmdGen` | `neo/framework/UsercmdGen.h:168` | `neo/framework/UsercmdGen.cpp:418-419` | Global input-to-usercmd producer. |
| `networkSystem` | `networkSystemLocal` | `neo/framework/async/NetworkSystem.h:63` | `neo/framework/async/NetworkSystem.cpp:34-35` | Game-facing networking service facade. |
| `idAsyncNetwork::*` | static class members | `neo/framework/async/AsyncNetwork.h:143-196` | `neo/framework/async/AsyncNetwork.cpp:34-71` | Static server/client/network CVars and master state; no exported pointer, but acts as a process-wide network singleton. |
| `idLib::sys`, `idLib::common`, `idLib::cvarSystem`, `idLib::fileSystem` | static service pointers on `idLib` | `neo/idlib/Lib.h:50-55` | `neo/idlib/Lib.cpp:46-49` | Bridge used by idLib code. Populated by engine and DLL/tool entrypoints instead of owning services itself. |

## Rendering And Media

| Public access point | Backing storage | Declaration | Definition | Notes |
| --- | --- | --- | --- | --- |
| `renderSystem` | `tr` | `neo/renderer/RenderSystem.h:261`, `neo/renderer/tr_local.h:809` | `neo/renderer/RenderSystem.cpp:33-34` | Renderer service and the concrete frontend renderer state. |
| `backEnd` | `backEnd` | `neo/renderer/tr_local.h:808` | `neo/renderer/tr_backend.cpp:35` | Global renderer backend state, frame counters, GL state cache, and performance counters. |
| `globalImages` | `imageManager` | `neo/renderer/Image.h:445` | `neo/renderer/Image_init.cpp:73-74` | Global image manager/texture registry. |
| `renderModelManager` | `localModelManager` | `neo/renderer/ModelManager.h:97` | `neo/renderer/ModelManager.cpp:76-77` | Global render-model cache and allocation manager. |
| `vertexCache` | `vertexCache` | `neo/renderer/VertexCache.h:143` | `neo/renderer/VertexCache.cpp:41` | Global vertex/index buffer cache. |
| `soundSystem` | `soundSystemLocal` | `neo/sound/sound.h:348`, `neo/sound/snd_local.h:825` | `neo/sound/snd_system.cpp:91-92` | Global sound system and hardware/mixer owner. |

## World, UI, And Tooling Managers

| Public access point | Backing storage | Declaration | Definition | Notes |
| --- | --- | --- | --- | --- |
| `collisionModelManager` | `collisionModelManagerLocal` | `neo/cm/CollisionModel.h:146` | `neo/cm/CollisionModel_load.cpp:54-55` | Global collision-model cache and loader. |
| `uiManager` | `uiManagerLocal` | `neo/ui/UserInterface.h:160` | `neo/ui/UserInterface.cpp:39-40` | Global GUI manager. |
| `AASFileManager` | `AASFileManagerLocal` | `neo/tools/compilers/aas/AASFileManager.h:48` | `neo/tools/compilers/aas/AASFileManager.cpp:51-52` | AAS compiler/tool global file manager. |
| `gDebuggerApp` | `gDebuggerApp` | `neo/tools/debugger/DebuggerApp.h:106` | `neo/tools/debugger/debugger.cpp:38` | Debugger tool application singleton. |
| `gApp` | `gApp` | `neo/tools/guied/GEApp.h:189` | `neo/tools/guied/guied.cpp:41` | GUI editor application singleton. |

## Game DLL Globals

The base game and D3XP game DLLs each define their own imported engine pointers and game-local backing objects. These are module-local globals, not the same storage as the engine EXE globals.

| Module | Public access point | Backing storage | Declaration | Definition | Notes |
| --- | --- | --- | --- | --- | --- |
| Base game | `game` | `gameLocal` | `neo/game/Game.h:195`, `neo/game/Game_local.h:578` | `neo/game/Game_local.cpp:64-65` | Main game-world singleton exported through the game API. |
| Base game | `gameEdit` | `gameEditLocal` | `neo/game/Game.h:310` | `neo/game/GameEdit.cpp:667-668` | Editor-facing game-edit service. |
| Base game | `animationLib` | `animationLib` | `neo/game/Game_local.h:579` | `neo/game/Game_local.cpp:61` | Process-global animation manager inside the game DLL. |
| Base game | `common`, `cmdSystem`, `cvarSystem`, `fileSystem`, `networkSystem`, `renderSystem`, `soundSystem`, `renderModelManager`, `uiManager`, `declManager`, `AASFileManager`, `collisionModelManager` | import-pointer globals | `neo/game/Game.h:326-338` | `neo/game/Game_local.cpp:37-48` | Engine-service mirrors filled from `gameImport_t`; initialized to `NULL` at module load. |
| D3XP game | `game` | `gameLocal` | `neo/d3xp/Game.h:197`, `neo/d3xp/Game_local.h:638` | `neo/d3xp/Game_local.cpp:64-65` | Expansion game-world singleton exported through the game API. |
| D3XP game | `gameEdit` | `gameEditLocal` | `neo/d3xp/Game.h:312` | `neo/d3xp/GameEdit.cpp:667-668` | Expansion editor-facing game-edit service. |
| D3XP game | `animationLib` | `animationLib` | `neo/d3xp/Game_local.h:639` | `neo/d3xp/Game_local.cpp:61` | Expansion animation manager singleton. |
| D3XP game | `common`, `cmdSystem`, `cvarSystem`, `fileSystem`, `networkSystem`, `renderSystem`, `soundSystem`, `renderModelManager`, `uiManager`, `declManager`, `AASFileManager`, `collisionModelManager` | import-pointer globals | `neo/d3xp/Game.h:328-340` | `neo/d3xp/Game_local.cpp:37-48` | Engine-service mirrors filled from D3XP `gameImport_t`; initialized to `NULL` at module load. |

## Tool Entry Point Mirrors

| Tool | Global access point | Definition | Notes |
| --- | --- | --- | --- |
| TypeInfo generator | `common`, `session`, `declManager`, `eventLoop`, and `idLib::*` service pointers | `neo/TypeInfo/main.cpp:35-37`, `neo/TypeInfo/main.cpp:95-96`, `neo/TypeInfo/main.cpp:273-276` | Tool-local globals used to satisfy idLib/framework code while generating type info. |
| MayaImport | `common`, `cvarSystem`, and `idLib::*` service pointers | `neo/MayaImport/maya_main.cpp:49-50`, `neo/MayaImport/maya_main.cpp:3119-3122` | Exporter-side service mirrors; `idLib::cvarSystem` and `idLib::fileSystem` are explicitly set to `NULL`. |

## Not Classified As Singletons

- `idCVar` globals and static class CVars are global configuration state, but there are hundreds and they are registered through `cvarSystem`; this inventory treats the registry singleton as the ownership surface.
- Math constants such as `vec3_origin`, `mat3_identity`, `colorWhite`, and script `idTypeDef`/`idVarDef` tables are shared constants or schema globals, not service singletons.
- OpenGL, WGL, Cg, OpenAL, Curl, and Ogg/Vorbis function-pointer globals are dynamic API binding tables rather than engine-owned singleton services.
- Function-local statics used as scratch buffers or cached constants were excluded unless they are the sole backing instance for a named service pointer.
