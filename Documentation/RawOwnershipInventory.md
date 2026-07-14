# Raw Ownership Inventory

Date: 2026-07-13

This inventory tracks places where the codebase owns memory or resources through raw pointers, custom allocators, Doom 3 containers of pointers, or manual acquire/release APIs. It is intended as a modernization guide, not a request to change behavior in this pass.

## Scope

Included:

- Runtime engine/game code under `neo/cm`, `neo/framework`, `neo/game`, `neo/d3xp`, `neo/idlib`, `neo/renderer`, `neo/sound`, `neo/sys`, and `neo/ui`.
- Tool/editor code under `neo/tools`, `neo/TypeInfo`, and `neo/MayaImport`.
- Vendored or third-party code that is compiled in-tree, called out separately so it is not mixed with engine-owned modernization work.

Excluded from ownership classification:

- Non-owning raw pointers used as parameters, back-pointers, linked-list links, transient stack aliases, renderer/game references, or cache lookup results.
- Raw pointers that are explicitly API handles into engine registries and are owned elsewhere.

## Scan Summary

The scan searched C/C++ sources and headers for:

- `new`, `delete`, `new[]`, `delete[]`
- `Mem_Alloc`, `Mem_ClearedAlloc`, `Mem_Alloc16`, `Mem_Free`, `Mem_Free16`
- `malloc`, `calloc`, `realloc`, `free`
- `idList<T *>`, `idHashTable<T *>`
- `DeleteContents(true)` and `DeleteContents()`

Approximate hit counts by subsystem:

| Area | `new` | `delete` | `Mem_* alloc` | `Mem_* free` | C heap alloc | C heap free | Owning pointer containers |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `neo/tools` | 398 | 293 | 98 | 77 | 6 | 7 | 58 |
| `neo/d3xp` | 214 | 141 | 12 | 12 | 0 | 0 | 66 |
| `neo/game` | 192 | 138 | 12 | 12 | 0 | 0 | 65 |
| `neo/renderer` | 89 | 48 | 106 | 132 | 10 | 10 | 20 |
| `neo/idlib` | 115 | 84 | 45 | 38 | 12 | 14 | 35 |
| `neo/framework` | 75 | 51 | 16 | 36 | 0 | 0 | 29 |
| `neo/ui` | 67 | 29 | 4 | 4 | 0 | 0 | 32 |
| `neo/sound` | 34 | 9 | 2 | 6 | 0 | 0 | 5 |
| `neo/cm` | 9 | 17 | 23 | 17 | 0 | 0 | 0 |
| `neo/sys` | 18 | 4 | 4 | 2 | 20 | 24 | 0 |
| `neo/MayaImport` | 10 | 5 | 0 | 0 | 0 | 0 | 8 |
| `neo/TypeInfo` | 7 | 9 | 1 | 2 | 0 | 0 | 3 |
| `neo/curl` | 87 | 1 | 0 | 0 | 138 | 299 | 0 |

The table is a routing map. Some `new`/`delete` hits are comments or placement-style usage, and some pointer containers are non-owning. The detailed sections below identify the ownership surfaces that matter.

## Core Ownership Mechanisms

### Global Heap And Operators

Ownership surface:

- `neo/idlib/Heap.h:68-87` declares `Mem_Alloc`, `Mem_Free`, aligned allocation helpers, and global `operator new/delete` wrappers.
- `neo/idlib/Heap.h:94-139` adds debug-file/line variants and macro redirection.
- `neo/idlib/Heap.cpp:1069-1130` implements non-debug `Mem_Alloc`, `Mem_Free`, `Mem_Alloc16`, and `Mem_Free16`.
- `neo/idlib/Heap.cpp:1565-1709` implements debug memory allocation/free paths.

Modernization concern:

- Almost every `new` in the engine routes through `Mem_Alloc`, so replacing raw ownership needs to preserve heap diagnostics, 16-byte alignment, and memory-stat behavior.

### `idBlockAlloc` And `idDynamicBlockAlloc`

Ownership surface:

- `neo/idlib/Heap.h:156-226` owns fixed-size block pages in `idBlockAlloc`.
- `neo/idlib/Heap.h:393-876` owns variable-size blocks in `idDynamicBlockAlloc`.
- `neo/sound/snd_decoder.cpp:47-78` routes Ogg decoder memory through `decoderMemoryAllocator`.
- `neo/sound/snd_cache.cpp:36-62` routes sound sample cache memory through `soundCacheAllocator`.

Modernization concern:

- These are allocator owners, not simple pointer fields. Wrap callers first; do not replace internals until tests cover alignment, reuse, fixed blocks, and memory accounting.

### `idList<T *>` And `idHashTable<T *>`

Ownership surface:

- `neo/idlib/containers/List.h:132` exposes `DeleteContents(bool clear)`.
- `neo/idlib/containers/List.h:196-207` deletes pointees in list order.
- `neo/idlib/containers/HashTable.h:58` exposes `DeleteContents`.
- `neo/idlib/containers/HashTable.h:331-335` deletes table values.

Modernization concern:

- `idList<T *>` is either owning or non-owning by convention. The ownership bit is usually at the call site: `DeleteContents(true)` means owning, plain `Clear()` usually means non-owning or externally freed.

### `idStr`

Ownership surface:

- `neo/idlib/Str.h:116-317` owns `char *data`.
- `neo/idlib/Str.cpp:37` uses `idDynamicBlockAlloc<char, ...>` for string backing storage.
- `neo/idlib/Str.cpp:100-128` reallocates and deletes owned buffers.

Modernization concern:

- `idStr` is already an ownership wrapper. Treat its internal raw pointer as contained legacy ownership, not as a site to convert in gameplay/rendering code.

## Runtime Inventory

### Collision Model Manager

Ownership surface:

- `neo/cm/CollisionModel_local.h:170-181` defines `cm_model_t` owned arrays and blocks: `vertices`, `edges`, `nodeBlocks`, `polygonBlock`, and `brushBlock`.
- `neo/cm/CollisionModel_local.h:514` stores the manager-owned `cm_model_t **models` table.
- `neo/cm/CollisionModel_load.cpp:538-555` allocates and initializes `cm_model_t`.
- `neo/cm/CollisionModel_load.cpp:572-637` allocates node, polygon-reference, and brush-reference blocks.
- `neo/cm/CollisionModel_load.cpp:660-695` allocates per-polygon and per-brush memory when block storage is unavailable.
- `neo/cm/CollisionModel_files.cpp:322-349` allocates parsed vertex and edge arrays from `.cm` files.
- `neo/cm/CollisionModel_load.cpp:366-399` frees every owned block/array and deletes `cm_model_t`.
- `neo/cm/CollisionModel_load.cpp:416-424` frees all loaded models and the `models` table.
- `neo/cm/CollisionModel_load.cpp:3370` allocates the model table with `Mem_ClearedAlloc`.

Ownership pattern:

- `idCollisionModelManagerLocal` owns the `models` table.
- Each `cm_model_t` owns its packed geometry arrays and variable-sized allocation blocks.
- References within trees are raw links into those blocks.

Risk:

- High. This code mixes block ownership, intrusive linked refs, and handle-index APIs. Convert only behind manager-level handles or private wrappers.

### Renderer

Ownership surface:

- `neo/renderer/RenderSystem.cpp:992-1009` allocates/frees `idRenderWorldLocal` instances and stores them in `worlds`.
- `neo/renderer/RenderWorld_local.h:140-155` owns `areaNodes`, `portalAreas`, `doublePortals`, `entityDefs`, and `lightDefs`.
- `neo/renderer/RenderWorld.cpp:133-153` initializes world-owned pointers and documents that shutdown frees entity/light/portal data.
- `neo/renderer/RenderWorld.cpp:263-327` owns `idRenderEntityLocal` entries.
- `neo/renderer/RenderWorld.cpp:417-471` owns `idRenderLightLocal` entries.
- `neo/renderer/RenderWorld.cpp:703` allocates per-frame `viewDef_t` through `R_ClearedFrameAlloc`.
- `neo/renderer/RenderSystem.cpp:163-184` allocates render command buffers through `R_FrameAlloc`.
- `neo/renderer/ModelManager.cpp:217-244` owns default/beam/sprite models and deletes all model entries on shutdown.
- `neo/renderer/ModelManager.cpp:293-378` creates and frees model instances by type.
- `neo/renderer/Image.h:222-254` stores image links, partial-image relationships, and cache lists; ownership is centralized in `idImageManager`.
- `neo/renderer/Model_ase.cpp:249-911`, `neo/renderer/Model_ma.cpp:148-1106`, and `neo/renderer/Model_lwo.cpp` own importer parse trees and raw mesh arrays through `Mem_Alloc`/`Mem_Free`.

Ownership pattern:

- `idRenderSystemLocal` owns render worlds.
- `idRenderWorldLocal` owns persistent world definitions and uses frame allocation for transient view/build data.
- `idRenderModelManagerLocal` owns render model objects.
- Importers own temporary raw parse graphs until converted into engine model data.

Risk:

- High for world/model/frame-data paths because lifetimes are tied to render handles and frame allocator assumptions.
- Medium for importers because they are bounded parse/build pipelines and have clearer teardown.

### Game And D3XP Gameplay

The base game and expansion trees duplicate most ownership patterns. Treat `neo/game` and `neo/d3xp` as paired modernization targets.

Ownership surface:

- `neo/game/Game_local.h` and `neo/d3xp/Game_local.h` own entity arrays, `spawnedEntities`, `aasList`, and game-local service objects.
- `neo/game/Game_local.cpp` and `neo/d3xp/Game_local.cpp` allocate and tear down map/game state; `aasList.DeleteContents(true)` is at `neo/game/Game_local.cpp:345` and `neo/d3xp/Game_local.cpp:419`.
- `neo/game/Entity.cpp:600-603` deletes owned `renderView` and `signals`.
- `neo/game/Entity.cpp:745` allocates `signalList_t`.
- `neo/game/Entity.cpp:1487` allocates entity-local `renderView_t`.
- `neo/game/Entity.cpp:2411-2470` creates `idClipModel` objects handed into physics ownership.
- `neo/game/Entity.cpp:3108-3148` creates temporary constructor/destructor script threads and deletes immediate destructor threads.
- `neo/game/Actor.cpp:63-140` owns animation-state `idThread`.
- `neo/game/Actor.cpp:472-473` owns `combatModel`.
- `neo/game/Actor.cpp:1209-1218` tears down actor script threads.
- `neo/game/Actor.cpp:1250-1255` allocates manual-control script threads.
- `neo/game/Player.h:140` owns inventory item dictionaries.
- `neo/game/Player.cpp:123-143` clears inventory and deletes item dictionaries.
- `neo/game/Player.cpp:340`, `neo/game/Player.cpp:540`, and `neo/game/Player.cpp:3202` allocate inventory `idDict` entries.
- `neo/game/Player.cpp:3428` deletes dropped/removed inventory items.
- `neo/game/anim/Anim.h:366`, `neo/game/anim/Anim.h:571`, and `neo/game/anim/Anim.h:616` own animation and joint-mod pointer containers.
- `neo/game/anim/Anim.cpp:943` deletes the animation hash table contents.
- `neo/game/anim/Anim_Blend.cpp` deletes `idAnim` and `jointMod_t` lists via `DeleteContents(true)`.
- `neo/game/script/Script_Program.cpp:1124-1245` allocates `idTypeDef`, `idVarDefName`, and `idVarDef`.
- `neo/game/script/Script_Program.cpp:1868-1877` deletes script var/type lists.
- `neo/game/script/Script_Program.cpp:2066-2074` deletes post-compile rollback types and definitions.

Expansion mirror:

- The same patterns are present under `neo/d3xp`, with matching files and usually matching line neighborhoods.

Risk:

- High. Gameplay ownership is entangled with save/restore, entity spawn order, script threads, physics handoff, and expansion parity.

### Gameplay Physics

Ownership surface:

- `neo/game/physics/Physics_Static.cpp`, `Physics_RigidBody.cpp`, `Physics_Parametric.cpp`, and `Physics_StaticMulti.cpp` own `idClipModel` pointers passed through `SetClipModel`.
- `neo/game/physics/Physics_RigidBody.cpp:463-487` owns an ODE integrator and clip model.
- `neo/game/physics/Clip.cpp:112-187` owns trace-model cache entries.
- `neo/game/physics/Clip.cpp:689-716` owns the `clipSectors` array.
- `neo/game/physics/Physics_AF.h` stores owning lists of bodies, constraints, contact constraints, and trees.
- `neo/game/physics/Physics_AF.cpp:4145-4239` makes `idAFBody` own its `idClipModel`.
- `neo/game/physics/Physics_AF.cpp:6659-6677` deletes trees, bodies, constraints, contact constraints, LCP solver, and master body.
- `neo/game/physics/Physics_AF.cpp:7280-7335` deletes removed constraints and bodies.

Expansion mirror:

- `neo/d3xp/physics/*` contains the matching ownership surfaces.

Risk:

- High. Clip-model ownership transfers are implicit and easy to double-delete if converted without a clear owner API.

### Framework

Ownership surface:

- `neo/framework/FileSystem.cpp:2130-2160` allocates `searchpath_t` and `directory_t` entries.
- `neo/framework/FileSystem.cpp:2496-2512` owns temporary buffers/lexers while parsing binary configs.
- `neo/framework/FileSystem.cpp:2928-2966` shuts down the filesystem and deletes search paths, directories, packs, addon info, build buffers, and pack handles.
- `neo/framework/FileSystem.cpp:3103-3575` returns owned `idFile` objects from open calls and deletes them in `CloseFile`.
- `neo/framework/CVarSystem.cpp:466-593` owns `idInternalCVar` entries in `cvars`.
- `neo/framework/CVarSystem.cpp:1248-1249` deletes individual cvars during removal.
- `neo/framework/DeclManager.cpp` owns decl instances and parser buffers through its decl folders/lists.
- `neo/framework/async/MsgChannel.h:151` owns an `idCompressor`.
- `neo/framework/async/MsgChannel.cpp:263-292` allocates and deletes the channel compressor.
- `neo/framework/Common.cpp:2829-2900` owns `config_compressor`.
- `neo/framework/Session.cpp:407-427` owns render worlds and sound worlds allocated for the session.
- `neo/framework/Session.cpp:800-929` owns demo-file objects.

Ownership pattern:

- Registry-style managers own pointer lists (`cvars`, decls, files, search paths).
- File APIs return raw owning pointers and rely on `CloseFile`/`delete`.

Risk:

- Medium to high. File and decl APIs are broad; ownership wrappers need gradual adapter introduction.

### UI

Ownership surface:

- `neo/ui/UserInterfaceLocal.h:101` stores the owned desktop `idWindow`.
- `neo/ui/UserInterfaceLocal.h:143-144` stores manager-owned GUI lists.
- `neo/ui/UserInterface.cpp:55-58` deletes cached GUI lists on UI manager shutdown.
- `neo/ui/UserInterface.cpp:168-225` allocates/frees GUI and list-GUI instances.
- `neo/ui/UserInterface.cpp:250-284` deletes and recreates owned desktop windows.
- `neo/ui/UserInterface.cpp:468-589` creates desktop windows during demo/save restore and routes serialization through the desktop.
- `neo/ui/Window.h:413-446` stores owned `definedVars`, `children`, `scripts`, `timeLineEvents`, `namedEvents`, and expression parse state.
- `neo/ui/Window.cpp:243-259` deletes simple draw windows, named events, child windows, defined vars, timeline events, and scripts.
- `neo/ui/Window.cpp:2190-2318` creates typed child windows.
- `neo/ui/Window.cpp:2329-2370` creates named and timeline events.
- `neo/ui/Window.cpp:2402-2459` creates defined window variables.
- `neo/ui/Window.cpp:2876-2915` stores raw `idWinVar *` and deferred `char *` names in expression ops.
- `neo/ui/Window.cpp:3458-3782` owns temporary transition name pointers during save/restore fixup.
- `neo/ui/Window.cpp:3866-3873` deletes deferred expression names after fixup.
- `neo/ui/GuiScript.cpp:282-287` deletes script lists and parameter variables.
- `neo/ui/GuiScript.cpp:387-532` creates script parameter vars.

Ownership pattern:

- UI has a tree owner (`idUserInterfaceLocal` -> desktop -> child windows).
- Script and expression state stores raw pointers plus pointer-sized encoded values.
- Some raw pointers are deferred fixup tokens and become non-owning references later.

Risk:

- High. UI pointer ownership is intertwined with demo/save formats and expression op encoding.

### Sound

Ownership surface:

- `neo/sound/snd_cache.cpp:36-62` owns cache allocator and sound sample list.
- `neo/sound/snd_cache.cpp:108` allocates `idSoundSample` entries.
- `neo/sound/snd_cache.cpp:347-648` owns sample backing data, amplitude data, OpenAL buffers, and allocator-backed decode buffers.
- `neo/sound/snd_decoder.cpp:47-78` provides Ogg decoder allocator callbacks.
- `neo/sound/snd_decoder.cpp:174-263` owns transient `OggVorbis_File`.
- `neo/sound/snd_decoder.cpp:304-345` owns sample decoder objects through `idBlockAlloc`.
- `neo/sound/snd_emitter.cpp:201-215` allocates/frees per-channel decoders.
- `neo/sound/snd_emitter.cpp:227-248` releases OpenAL sources and streaming buffers.
- `neo/sound/snd_world.cpp:61-111` owns sound emitters and deletes them on shutdown.
- `neo/sound/snd_world.cpp:144-179` allocates/reuses local emitters.
- `neo/sound/snd_world.cpp:1251-1302` recreates emitters and decoders on save restore.
- `neo/sound/snd_system.cpp` owns OpenAL source pools, mix buffers, and sound FX objects.
- `neo/sys/win32/win_snd.cpp:108-464` owns DirectSound/OpenAL compatibility hardware objects and wave/audio buffers.

Ownership pattern:

- `idSoundWorldLocal` owns emitters.
- Channels own optional decoders and OpenAL streaming buffers.
- `idSoundCache` owns sample objects and sample memory.
- Sound hardware owns API handles and adapter buffers.

Risk:

- Medium to high. Audio has strong manual resource lifetimes but relatively good owner names.

### System And Platform

Ownership surface:

- `neo/sys/win32/win_snd.cpp` owns audio hardware objects, wave-file helpers, API buffers, and OpenAL library handles.
- `neo/sys/win32/win_shared.cpp` owns explicit COM pointers and releases them manually in WMI/system-info paths.
- `neo/sys/posix` and `neo/sys/linux` contain platform allocation and handle ownership for networking, input, sound, and process utilities.

Risk:

- Medium. Platform owners are narrower but must preserve external API release order.

## Tool And Editor Inventory

### AAS Compiler

Ownership surface:

- `neo/tools/compilers/aas/AASBuild.cpp:68-83` deletes owned `file` and `ledgeMap`.
- `neo/tools/compilers/aas/AASBuild.cpp:102-190` owns `procNodes` and parser lexers.
- `neo/tools/compilers/aas/AASBuild.cpp:330-357` creates and deletes brush sides and brushes during build.
- `neo/tools/compilers/aas/AASBuild.cpp:643-836` owns expanded brush lists, map files, and output AAS files.
- `neo/tools/compilers/aas/AASFile.cpp:598-608` deletes reachability entries and clears AAS arrays.
- `neo/tools/compilers/aas/AASFile.cpp:920-924` allocates reachability objects.
- `neo/tools/compilers/aas/Brush.cpp` owns brush sides, windings, brush lists, split outputs, and maps.
- `neo/tools/compilers/aas/BrushBSP.cpp` owns BSP nodes, portals, windings, grid cells, and brush maps.

Risk:

- Medium. Mostly batch compiler lifetime, but pointer graphs are dense.

### DMAP Compiler

Ownership surface:

- `neo/tools/compilers/dmap/dmap.cpp:374-375` deletes the common `.map` representation.
- `neo/tools/compilers/dmap/optimize.cpp:369-593` owns temporary optimization vertices and length tables.
- `neo/tools/compilers/dmap/optimize.cpp:1005-1629` owns generated optimization triangles, crossings, sorted arrays, and original-edge arrays.
- `neo/tools/compilers/dmap/usurface.cpp:85-599` owns optimize groups and area arrays.
- `neo/tools/compilers/dmap/usurface.cpp:721-983` owns clipped winding fragments and replacement group lists.

Risk:

- Medium. Ownership is manual but mostly local to compiler passes.

### Radiant And Editors

Ownership surface:

- `neo/tools/radiant/EditorBrush.cpp:118-277` allocates/free brush and face data.
- `neo/tools/radiant/EditorBrush.cpp:693-1357` owns transient windings during CSG/splitting.
- `neo/tools/radiant/EditorBrush.cpp:2281-2304` frees patch, face, epair, and brush ownership.
- `neo/tools/radiant/EditorBrush.cpp:4548-4628` owns curve/swept-spline conversion temporaries.
- `neo/tools/radiant/EditorMap.cpp:314-615` allocates map epairs, patches, brushes, sides, and map entities.
- `neo/tools/guied/GEWorkspace.cpp:61`, `89-111`, and `1966-1979` own GUI editor interfaces, wrappers, and static clipboard items.
- `neo/tools/guied/GEWorkspace.cpp:1317-2008` allocates modifier objects and windows for edit operations.
- `neo/tools/guied/GEModifierStack.h:58` and `GEModifierGroup.h:57` store modifier pointer lists.
- `neo/tools/materialeditor/MaterialDoc.h:47` owns material stage pointers.
- `neo/tools/materialeditor/MaterialDoc.cpp:457-514` creates/deletes material stages.
- `neo/tools/materialeditor/MaterialDoc.cpp:835-840` clears owned edit stages.
- `neo/tools/materialeditor/MaterialDocManager.h:153-154` owns undo/redo modifier lists.
- `neo/tools/materialeditor/MaterialDocManager.cpp:575-617` deletes undo/redo modifiers.
- `neo/tools/debugger/DebuggerServer.h:72` and `DebuggerWindow.h:44-115` store debugger breakpoint/watch/script pointer lists.

Risk:

- Medium. Editor code is high-volume but usually less runtime-critical than game/renderer ownership.

### TypeInfo And Maya Import

Ownership surface:

- `neo/TypeInfo/TypeInfoGen.cpp` owns generated type-info objects and deletes list contents.
- `neo/MayaImport/maya_main.cpp:373-375`, `1465`, `1622`, and `2011-2261` owns Maya DAG nodes and export meshes.

Risk:

- Low to medium. Tool-specific, isolated, and easier to wrap after build coverage exists.

## Vendored And Third-Party Ownership

### Curl

Ownership surface:

- `neo/curl/lib/*`, `neo/curl/src/*`, `neo/curl/tests/*`, and examples use C heap ownership extensively.
- Hotspots include `neo/curl/lib/cookie.c`, `formdata.c`, `hostip.c`, `http.c`, `url.c`, and `transfer.c`.

Policy:

- Treat as vendored ownership. Do not modernize with engine wrappers unless replacing the dependency or carrying a deliberate fork.

### Ogg Vorbis

Ownership surface:

- `neo/sound/OggVorbis/oggsrc/*`, `vorbissrc/*`, and headers contain codec-internal C ownership.
- Runtime decoder entry points are wrapped by `neo/sound/snd_decoder.cpp` allocator callbacks.

Policy:

- Keep codec internals stable. Modernize only the engine wrapper layer.

### JPEG, OpenAL, Unzip, Platform SDK Code

Ownership surface:

- `neo/renderer/jpeg-6/*` uses IJG C allocation idioms.
- `neo/openal/*` is compatibility/vendor code.
- `neo/framework/Unzip.cpp` owns zlib/unzip tables and buffers.
- `neo/sys/linux/oss`, `neo/sys/linux/libXNVCtrl`, and OS-specific framework headers are external-style code.

Policy:

- Treat as third-party unless a security or build issue requires a targeted patch.

## Highest-Risk Raw Ownerships

1. `idList<T *>` ownership ambiguity across `neo/game`, `neo/d3xp`, `neo/ui`, `neo/tools`, and `neo/framework`.
2. Collision `cm_model_t` block ownership and model table lifetime.
3. Renderer frame allocation versus persistent render-world/model ownership.
4. Gameplay physics clip-model transfer into `SetClipModel`.
5. UI expression/script ownership where raw pointers are encoded in `intptr_t` fields or temporarily stored as `char *`.
6. File-system raw `idFile *` ownership returned by open calls and deleted through `CloseFile`.
7. Sound channels owning optional decoders plus external OpenAL handles.

## Recommended Modernization Order

1. Add ownership annotations and tiny wrappers around `idList<T *>` owners before converting containers.
2. Convert local, single-file temporary owners first: sort arrays, parser temporaries, importer scratch allocations, compiler pass temporaries.
3. Add explicit owner APIs for file handles, sound decoders, and clip models.
4. Convert manager-owned lists one subsystem at a time: CVar/Decl/FileSystem, SoundCache, RenderModelManager.
5. Leave collision model blocks, renderer frame data, UI expression ops, and gameplay entity/script lifetimes until dedicated tests and save/restore coverage exist.

## Verification Commands Used

Representative scans:

```powershell
rg -n --glob '*.{cpp,c,h,hpp,inl,C,CPP,H}' "\b(new|delete)\b|Mem_(Alloc|Free|ClearedAlloc)|malloc\s*\(|calloc\s*\(|realloc\s*\(|free\s*\(" neo tests CMakeLists.txt cmake
rg -n --glob '*.{cpp,c,h,hpp,inl,C,CPP,H}' "DeleteContents\s*\(|idList\s*<[^>]*\*|idHashTable\s*<[^>]*\*" neo tests
rg -n "operator new|operator delete|Mem_Alloc|Mem_Free|DeleteContents|idDynamicBlockAlloc" neo/idlib
```

No build or runtime tests were run because this pass only adds an inventory document.
