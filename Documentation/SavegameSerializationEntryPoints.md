# Savegame Serialization Entry Point Inventory

Date: 2026-07-13

This inventory records savegame serialization entry points in the session, game, save/restore helper classes, and per-class `Save`/`Restore` methods. The base game and D3XP have mirrored implementations; treat `neo/game` and `neo/d3xp` as parallel serialization surfaces.

## Session-Level Save/Load Flow

| Entry point | Location | Role |
| --- | --- | --- |
| `SaveGame_f` | `neo/framework/Session.cpp:1752` | Console command entry point for manual saves. |
| `LoadGame_f` | `neo/framework/Session.cpp:1737` | Console command entry point for loading saves. |
| `idSessionLocal::SaveGame` | `neo/framework/Session.cpp:1894` | Top-level save writer. Validates state, opens `savegames/*.save`, writes header, calls `game->SaveGame`, captures preview, writes description text. |
| `idSessionLocal::LoadGame` | `neo/framework/Session.cpp:2040` | Top-level save reader. Opens `savegames/*.save`, validates game name/version/map, reads persistent player info, then triggers map load with `loadingSaveGame` state. |
| `idSessionLocal::GetSaveGameVersion` | `neo/framework/Session.cpp:3313` | Exposes the current loaded savegame version to game code. |
| `SAVEGAME_VERSION` | `neo/framework/Licensee.h:84` | Current savegame format version, currently `17`. |
| `idSessionLocal::GetSaveGameList` | `neo/framework/Session_menu.cpp:148` | UI/menu save listing. |
| `idSessionLocal::HandleSaveGameMenuCommand` | `neo/framework/Session_menu.cpp:340` | UI/menu save/load/delete dispatch. |

## Game-Level Entry Points

| Entry point | Base game | D3XP | Role |
| --- | --- | --- | --- |
| `idGame::InitFromSaveGame` | `neo/game/Game.h:109` | `neo/d3xp/Game.h:111` | Game interface contract for loading map state from a save file. |
| `idGame::SaveGame` | `neo/game/Game.h:112` | `neo/d3xp/Game.h:114` | Game interface contract for writing game state to a save file. |
| `idGameLocal::SaveGame` | `neo/game/Game_local.cpp:408` | `neo/d3xp/Game_local.cpp:482` | Main game-world save serializer. Writes game objects and global game-local state through `idSaveGame`. |
| `idGameLocal::InitFromSaveGame` | `neo/game/Game_local.cpp:1199` | `neo/d3xp/Game_local.cpp:1335` | Main game-world restore path. Creates/restores objects, repopulates map state, validates area counts. |
| `idGameLocal::MapPopulate` | `neo/game/Game_local.cpp:1129` | `neo/d3xp/Game_local.cpp:1265` | Shared map population helper used by normal and savegame initialization. |
| `idEntityPtr<type>::Save` / `Restore` | `neo/game/Game_local.h:589`, `neo/game/Game_local.h:594` | `neo/d3xp/Game_local.h:656`, `neo/d3xp/Game_local.h:661` | Entity pointer reference serialization helpers. |

## Save/Restore Helper Classes

| Helper | Base game | D3XP | Role |
| --- | --- | --- | --- |
| `idSaveGame` class | `neo/game/gamesys/SaveGame.h:40` | `neo/d3xp/gamesys/SaveGame.h:40` | Write-side serializer facade around `idFile`. |
| `idRestoreGame` class | `neo/game/gamesys/SaveGame.h:99` | `neo/d3xp/gamesys/SaveGame.h:99` | Read-side deserializer facade around `idFile`. |
| `idSaveGame::WriteObjectList` | `neo/game/gamesys/SaveGame.cpp:119` | `neo/d3xp/gamesys/SaveGame.cpp:119` | Writes class/object table for later restore. |
| `idSaveGame::CallSave_r` | `neo/game/gamesys/SaveGame.cpp:133` | `neo/d3xp/gamesys/SaveGame.cpp:133` | Recursively calls reflected class save functions. |
| `idRestoreGame::CreateObjects` | `neo/game/gamesys/SaveGame.cpp:794` | `neo/d3xp/gamesys/SaveGame.cpp:799` | Allocates objects from serialized class names. |
| `idRestoreGame::RestoreObjects` | `neo/game/gamesys/SaveGame.cpp:824` | `neo/d3xp/gamesys/SaveGame.cpp:829` | Recursively restores object state. |
| `idRestoreGame::CallRestore_r` | `neo/game/gamesys/SaveGame.cpp:891` | `neo/d3xp/gamesys/SaveGame.cpp:896` | Recursively calls reflected class restore functions. |

## Primitive And Engine-Object Serializers

| Serializer family | Base game locations | D3XP locations | Examples |
| --- | --- | --- | --- |
| Primitive values | `neo/game/gamesys/SaveGame.cpp:168-318`, `917-1068` | `neo/d3xp/gamesys/SaveGame.cpp:168-318`, `922-1073` | `WriteInt`, `WriteShort`, `WriteByte`, `WriteFloat`, `WriteBool`, `WriteString`, vectors, bounds, matrices, angles. |
| Object references | `neo/game/gamesys/SaveGame.cpp:329`, `348`, `1078`, `1093` | `neo/d3xp/gamesys/SaveGame.cpp:329`, `348`, `1083`, `1098` | `WriteObject`, `WriteStaticObject`, `ReadObject`, `ReadStaticObject`. |
| Dictionaries and decls | `neo/game/gamesys/SaveGame.cpp:357-477`, `1102-1239` | `neo/d3xp/gamesys/SaveGame.cpp:357-477`, `1107-1244` | `idDict`, material, skin, particle, FX, model def, sound shader, render model, UI. |
| Renderer state | `neo/game/gamesys/SaveGame.cpp:497`, `550`, `620`, `1264`, `1323`, `1391` | `neo/d3xp/gamesys/SaveGame.cpp:497`, `555`, `625`, `1269`, `1333`, `1401` | `renderEntity_t`, `renderLight_t`, `renderView_t`. |
| User/input/collision state | `neo/game/gamesys/SaveGame.cpp:648-739`, `1419-1509` | `neo/d3xp/gamesys/SaveGame.cpp:653-744`, `1429-1519` | `usercmd_t`, contact info, trace, trace model, clip model. |
| Sound commands/build number | `neo/game/gamesys/SaveGame.cpp:753`, `762`, `1526`, `1536` | `neo/d3xp/gamesys/SaveGame.cpp:758`, `767`, `1536`, `1546` | Deferred sound command state and build marker. |

## Per-Class Save/Restore Surfaces

The scan pattern below finds per-class savegame methods:

```powershell
rg -n "\b(Save|Restore)\s*\(\s*id(SaveGame|RestoreGame)\s*\*" neo\game neo\d3xp --glob "*.cpp" --glob "*.h"
```

### Base Game

| Area | Files |
| --- | --- |
| Core entities | `AF`, `Actor`, `AFEntity`, `BrittleFracture`, `Camera`, `Entity`, `Fx`, `Item`, `Light`, `Misc`, `Moveable`, `Mover`, `Player`, `PlayerView`, `Projectile`, `SecurityCamera`, `Sound`, `Target`, `Trigger`, `Weapon`, `WorldSpawn` |
| AI and animation | `ai/AI.*`, `anim/Anim.*`, `anim/Anim_Blend.cpp`, `anim/Anim_Testmodel.*` |
| Game systems | `gamesys/Class.h`, `gamesys/Event.*`, `Game_local.h` |
| Physics and forces | `physics/Clip.*`, `Force_Constant.*`, `Force_Field.*`, `Force_Grab.*`, `Physics.*`, `Physics_Actor.*`, `Physics_AF.*`, `Physics_Base.*`, `Physics_Monster.*`, `Physics_Parametric.*`, `Physics_Player.*`, `Physics_RigidBody.*`, `Physics_Static.*`, `Physics_StaticMulti.*` |
| Script runtime | `script/Script_Interpreter.*`, `script/Script_Program.*`, `script/Script_Thread.*` |

### D3XP

| Area | Files |
| --- | --- |
| Core entities | `AF`, `Actor`, `AFEntity`, `BrittleFracture`, `Camera`, `Entity`, `Fx`, `Grabber`, `Item`, `Light`, `Misc`, `Moveable`, `Mover`, `Player`, `PlayerView`, `Projectile`, `SecurityCamera`, `Sound`, `Target`, `Trigger`, `Weapon`, `WorldSpawn` |
| AI and animation | `ai/AI.*`, `anim/Anim.*`, `anim/Anim_Blend.cpp`, `anim/Anim_Testmodel.*` |
| Game systems | `gamesys/Class.h`, `gamesys/Event.*`, `Game_local.h` |
| Physics and forces | `physics/Clip.*`, `Force_Constant.*`, `Force_Field.*`, `Force_Grab.*`, `Physics.*`, `Physics_Actor.*`, `Physics_AF.*`, `Physics_Base.*`, `Physics_Monster.*`, `Physics_Parametric.*`, `Physics_Player.*`, `Physics_RigidBody.*`, `Physics_Static.*`, `Physics_StaticMulti.*` |
| Script runtime | `script/Script_Interpreter.*`, `script/Script_Program.*`, `script/Script_Thread.*` |

## High-Risk Savegame Compatibility Points

- `SAVEGAME_VERSION` lives in `neo/framework/Licensee.h`; changing serialized shape without version handling can break existing saves.
- `idSaveGame::WriteObjectList`, `idRestoreGame::CreateObjects`, and `idRestoreGame::RestoreObjects` are the object identity contract. Refactors must preserve object order and class-name mapping.
- `idSaveGame::CallSave_r` and `idRestoreGame::CallRestore_r` rely on generated/reflected type metadata. Modernization work in class/typeinfo code can silently break savegames.
- Renderer state serializers write `renderEntity_t`, `renderLight_t`, and `renderView_t` directly. Renderer-struct changes need explicit savegame compatibility review.
- Base game and D3XP serializers are duplicated. Behavioral changes should be applied to both unless the expansion intentionally diverges.
