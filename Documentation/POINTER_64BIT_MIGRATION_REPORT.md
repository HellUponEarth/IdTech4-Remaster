# IdTech4 64-bit Pointer Migration Report

Status: in progress. This report tracks the codebase-wide 32-bit pointer audit, p_ pointer-variable renaming, ABI impact, and verification. It is intentionally additive so each verified slice can update the same ledger.

## Current Slice Summary

Files changed:

- `neo/sys/sys_public.h`
- `neo/sys/sys_local.h`
- `neo/sys/sys_local.cpp`
- `neo/idlib/Heap.cpp`
- `neo/sys/win32/win_local.h`
- `neo/sys/win32/win_main.cpp`
- `neo/sys/win32/win_shared.cpp`
- `neo/sys/posix/posix_public.h`
- `neo/sys/posix/posix_main.cpp`
- `neo/framework/EventLoop.h`
- `neo/framework/EventLoop.cpp`

Corrections made:

- Renamed `sysEvent_t::evPtr` to `sysEvent_t::p_evPtr`.
- Updated event-loop, Win32 queue, POSIX queue, and generated mouse-event callers to use `p_evPtr`.
- Renamed touched pointer parameters and locals in event/memory-lock APIs: `ptr` -> `p_ptr`, `event` -> `p_event`, `ev` -> `p_ev`.
- Replaced `typedef unsigned long address_t` with `typedef uintptr_t address_t`.
- Replaced heap allocator pointer metadata that used `int`/`dword` with `uintptr_t`.
- Updated `Alloc16`/`Free16`, large allocation headers, page alignment, and small free-list links to avoid pointer truncation.
- Renamed touched heap allocator pointer parameters and locals with the `p_` prefix.

## Struct Layout / ABI Notes

### `sysEvent_t`

Before this slice:

```c
typedef struct sysEvent_s {
    sysEventType_t evType;
    int evValue;
    int evValue2;
    int evPtrLength;
    void *evPtr;
} sysEvent_t;
```

After this slice:

```c
typedef struct sysEvent_s {
    sysEventType_t evType;
    int evValue;
    int evValue2;
    int evPtrLength;
    void *p_evPtr;
} sysEvent_t;
```

Layout impact: field name only; native layout is unchanged by the rename. On 32-bit builds the structure remains 20 bytes with 4-byte pointer alignment. On 64-bit builds it is expected to become 24 bytes with 8-byte pointer alignment.

Binary compatibility: `sysEvent_t` is written directly to `journal.dat` in `neo/framework/EventLoop.cpp`. A 64-bit build cannot replay old 32-bit journals without a versioned journal header and a 32-bit legacy event reader. This is not yet fixed.

### `address_t`

Before this slice:

```c
typedef unsigned long address_t;
```

After this slice:

```c
typedef uintptr_t address_t;
```

Layout impact: on MSVC x64 this expands call-stack addresses from 32 to 64 bits. On LP64 targets this is usually layout-neutral because `unsigned long` is already 64 bits.

Binary compatibility: memory debugging records that embed `address_t callStack[MAX_CALLSTACK_DEPTH]` will change size on MSVC x64. Text dumps are compatible; raw binary dumps would not be.

### `idHeap` Large Allocation Header

Before this slice:

```c
#define LARGE_HEADER_SIZE ( (int) ( sizeof( dword * ) + sizeof( byte ) ) )
dw[0] = (dword)p;
```

After this slice:

```c
#define LARGE_HEADER_SIZE ( (int) ( sizeof( uintptr_t ) + sizeof( byte ) ) )
p_pageLink[0] = reinterpret_cast<uintptr_t>( p_page );
```

Layout impact: large-allocation metadata now reserves pointer-width storage explicitly. On Win32 the aligned header remains 8 bytes. On x64 the aligned header is 16 bytes.

Binary compatibility: this is allocator-internal process memory only; no persisted binary format should depend on it. Memory debugging and heap inspection tools that assume the old 32-bit page-link slot must be updated.

### `idHeap::Alloc16` Header

Before this slice:

```c
*((int *)(alignedPtr - 4)) = (int) ptr;
```

After this slice:

```c
*( reinterpret_cast<uintptr_t *>( p_alignedPtr - sizeof( uintptr_t ) ) ) = reinterpret_cast<uintptr_t>( p_ptr );
```

Layout impact: the hidden back-pointer before 16-byte aligned allocations grows from 4 bytes to pointer width. Allocation over-reservation was updated accordingly.

Binary compatibility: allocator-internal only; live allocations cannot cross old/new allocator code boundaries.

## Audit Findings

| File + line | Problem | Correct 64-bit fix | ABI / binary impact |
| --- | --- | --- | --- |
| `neo/sys/sys_public.h:221` | `evPtrLength` is an `int` byte count used for journaling pointer data. It is not pointer storage, but it limits event payloads and is serialized through native `sysEvent_t`. | Introduce an explicit journal record format with `uint32_t payloadLength` for legacy compatibility or `uint64_t payloadLength` for a new format, plus versioning. Runtime allocation APIs should use `size_t` after the format boundary. | ABI and file-format breaking if widened in `sysEvent_t`; needs version shim. |
| `neo/sys/sys_public.h:236` | `address_t` used `unsigned long`, which truncates addresses on Windows x64. | Changed to `uintptr_t`. | ABI break for classes/records containing `address_t` on MSVC x64. |
| `neo/framework/EventLoop.cpp:84` | Journaling writes native `sizeof(sysEvent_t)`, so pointer width and padding leak into the file format. | Replace native struct write/read with versioned fixed-width journal records and never serialize raw pointer values. | Required for 32/64-bit journal compatibility. |
| `neo/idlib/Heap.cpp:48` | `LARGE_HEADER_SIZE` was based on `dword *` and assumed 32-bit page-pointer storage. | Fixed: now uses `sizeof(uintptr_t) + sizeof(byte)`. | Heap header layout changes; allocator-internal ABI change for memory tools. |
| `neo/idlib/Heap.cpp:336` | `Alloc16` cast a pointer to `int` for alignment. | Fixed: alignment is computed with `reinterpret_cast<uintptr_t>`. | No external ABI impact; fixes x64 truncation. |
| `neo/idlib/Heap.cpp:340` | `Alloc16` stored the original allocation pointer in a 32-bit `int` slot. | Fixed: stores a `uintptr_t` immediately before the aligned block; `Free16` reads the same type. | Allocator header size changes; live allocations cannot cross old/new allocator code boundaries. |
| `neo/idlib/Heap.cpp:556` | Small-block free list stored next pointers in a `dword` inside freed blocks. | Fixed: small allocations reserve at least `sizeof(uintptr_t)` and store free-list links as `uintptr_t`. | Small allocator internal layout changes for freed blocks only. |
| `neo/idlib/Heap.cpp:933` | Large allocations stored `page_s *` through `dword`, truncating on x64. | Fixed: stores the page pointer as `uintptr_t`. | Heap header layout changes. |
| `neo/idlib/Heap.cpp:961` | Large free path read the page pointer from a 32-bit `dword`. | Fixed: reads the same `uintptr_t` page-link header used by allocation. | Must remain synchronized with large allocation header. |
| `neo/cm/CollisionModel_contents.cpp:305` | Stores pointer identity by reading the low 32 bits of `cm_polygon_t *` into `trace.c.modelFeature`. | Replace with stable feature IDs or widen `modelFeature` to `uintptr_t` if it is truly an internal pointer token. | Save/network/demo ABI must be audited before widening `trace_t`. |
| `neo/cm/CollisionModel_translate.cpp:422` | Same pointer-to-`int` model feature truncation for polygon hits. | Same as above. | Same as above. |
| `neo/cm/CollisionModel_translate.cpp:475` | Same pointer-to-`int` model feature truncation for polygon hits. | Same as above. | Same as above. |
| `neo/cm/CollisionModel_rotate.cpp:973` | Same pointer-to-`int` model feature truncation for polygon hits. | Same as above. | Same as above. |
| `neo/cm/CollisionModel_load.cpp:3488` | `GetModelPolygon` reconstructs `cm_polygon_t *` from the 32-bit `polygonNum` argument. | Replace polygon pointer tokens with stable polygon IDs, or widen the collision feature API to `uintptr_t` and then add serialization shims. Stable IDs are preferred. | Public collision API and all `trace_t` save/event paths are affected. |
| `neo/game/gamesys/SaveGame.cpp:678` / `neo/d3xp/gamesys/SaveGame.cpp:683` | `contactInfo_t::modelFeature` is serialized with `WriteInt`. | Keep a 32-bit stable feature ID on disk, or add versioned `uintptr_t`/`uint64_t` fields plus legacy readers. | Savegame format break unless versioned. |
| `neo/game/gamesys/Event.cpp:809` / `neo/d3xp/gamesys/Event.cpp:814` | `trace_t::c.modelFeature` is serialized through event save with `WriteInt`; event packing also copies raw `trace_t`. | Avoid pointer tokens in `trace_t`; if widening is required, version event serialization and audit raw `sizeof(trace_t)` event buffers. | Event save/restore ABI break. |
| `neo/sys/win32/win_shared.cpp:666` | Call-stack walking uses `long _ebp`, `long` temporaries, and 32-bit inline assembly. | Replace with x64-safe `CaptureStackBackTrace` / DbgHelp or compiler intrinsics; use `uintptr_t` throughout. | Behavior change in debug call-stack capture; public `address_t` now supports it. |
| `neo/sys/win32/win_cpu.cpp:60` | CPU timing uses inline x86 assembly registers. MSVC x64 does not support inline assembly. | Replace with intrinsics such as `__rdtsc`, `__cpuid`, `_fxsave`, and `_mm_getcsr` / `_mm_setcsr`. | No file-format ABI impact, but CPU feature detection behavior must be regression tested. |
| `neo/idlib/math/Math.h:392` | x87 inline assembly writes through pointer parameters. | Replace with portable `sincos` equivalent or compiler intrinsics guarded per platform. Rename pointer parameters with `p_`. | No external ABI if signatures remain stable except parameter names. |
| `neo/idlib/math/Simd_SSE.cpp:633` and many later sites | Large MSVC inline assembly body assumes 32-bit registers for addresses and loop counters. | Prefer existing generic/SSE intrinsic implementations for x64; otherwise port hot paths to intrinsics. | Build-system and performance impact; no data format impact. |
| `neo/doom.sln:25` | Solution only declares `Win32` platforms. | Add x64 configurations and map all projects to x64; project files need corresponding `Platform` entries and x64 output paths. | Build-output layout changes; required for x64 compilation. |

## Pointer Renaming Summary

Renamed in this slice:

- `sysEvent_t::evPtr` -> `sysEvent_t::p_evPtr`
- `Sys_QueEvent(..., void *ptr)` -> `Sys_QueEvent(..., void *p_ptr)`
- `Posix_QueEvent(..., void *ptr)` -> `Posix_QueEvent(..., void *p_ptr)`
- `idEventLoop::PushEvent(sysEvent_t *event)` -> `idEventLoop::PushEvent(sysEvent_t *p_event)`
- local queue pointers `ev` -> `p_ev` in the touched queue/push functions
- `Sys_LockMemory`, `Sys_UnlockMemory`, `idSysLocal::LockMemory`, and `idSysLocal::UnlockMemory` pointer parameters `ptr` -> `p_ptr`
- `idHeap::Free`, `Free16`, `Msize`, `FreePage`, `SmallFree`, `MediumFree`, `LargeFree`, and touched global memory wrappers now use `p_` pointer parameters.
- Touched allocator locals now use `p_` names such as `p_ptr`, `p_alignedPtr`, `p_page`, `p_smallBlock`, `p_link`, `p_pageLink`, `p_mem`, and `p_debugMemory`.

Known remaining examples:

- `neo/framework/Console.cpp` and `neo/framework/Session*.{h,cpp}` still use `const sysEvent_t *event`.
- `neo/framework/FileSystem.cpp` still uses Curl callback pointer parameters `ptr` and `stream`.
- `neo/idlib/Heap.cpp` still has untouched legacy pointer names in destructor/dump/medium allocation/reporting paths that were not part of this truncation fix.
- Most renderer, game, tool, idlib, and third-party pointer variables have not yet been renamed.

## Serialization / Networking / File Format Notes

- `journal.dat` originally stored native `sysEvent_t`; later slices replaced this with a versioned fixed-width event record.
- `journaldata.dat` stores `.cfg` journal payload lengths and bytes for file-system playback; later slices switched the length field to the fixed-width `idFile::ReadInt` / `WriteInt` helpers.
- Collision trace `modelFeature` originally carried pointer-derived values; later slices converted polygon contacts to stable 32-bit polygon IDs without widening the save/event field.
- No network message format has been changed in this slice.

## Recommended Next Slices

1. Replace the native `sysEvent_t` journal record with a versioned fixed-width format and a legacy 32-bit reader.
2. Replace collision-model pointer-derived `modelFeature` values with stable feature IDs or explicitly widen the owning trace field after save/network audit.
3. Add x64 Visual Studio solution/project configurations.
4. Disable or replace MSVC inline assembly on x64 with intrinsics/generic fallbacks.
5. Continue mechanical p_ pointer renaming subsystem by subsystem with builds between slices.
6. Finish untouched `neo/idlib/Heap.cpp` pointer-name cleanup once a compiler path is available.

## Verification Plan

- Compile `idlib`, `doomdll`, `game`, and `game-d3xp` as Win32 after each source-compatible slice.
- Add x64 project configurations, then compile x64 with `/W4` pointer-conversion warnings enabled where practical.
- Add allocator tests for `Alloc16` alignment, `Free16`, small/medium/large heap paths, and memory-debug call-stack storage.
- Add journal round-trip tests for legacy 32-bit event records and new 64-bit-safe records.
- Add collision trace tests that verify `modelFeature` identity without pointer truncation.
- Run a launch smoke test after build-system and allocator slices.

## Verification Log

- `rg -n "\.evPtr\b|->evPtr\b|\bevPtr\b" neo/sys neo/framework -g "*.h" -g "*.cpp"`: no stale `evPtr` field accesses remain in the touched sys/framework event path.
- `git diff --check`: passed.
- `MSBuild.exe neo\doom.sln /m /p:Configuration=Release /p:Platform=Win32 /v:minimal`: blocked before compilation because the projects require missing Visual Studio 2010 `v100` build tools.
- `MSBuild.exe neo\idlib.vcxproj /m /p:Configuration=Release /p:Platform=Win32 /p:PlatformToolset=v143 /v:minimal`: blocked before compilation because v143 C++ build tools are not installed in this environment.
- `MSBuild.exe neo\idlib.vcxproj /m /p:Configuration=Release /p:Platform=Win32 /p:PlatformToolset=v180 /v:minimal`: blocked before compilation because v180 C++ build tools are not installed in this environment.
- `rg -n "\(int\)\s*\w|\(int\)\(|\(long\)|\(dword\)\s*\w|dword \*|\*reinterpret_cast<\s*int|reinterpret_cast<\s*int|void \*ptr|byte \*ptr" neo/idlib/Heap.cpp`: no remaining allocator pointer-to-32-bit storage patterns from this slice.
- `MSBuild.exe neo\idlib.vcxproj /m /p:Configuration=Release /p:Platform=Win32 /v:minimal`: still blocked before compilation because the project requires missing Visual Studio 2010 `v100` build tools.

## 2026-07-13 x64 Build Bring-Up and Collision Feature-ID Slice

Files changed in this slice:

- `neo/cm/CollisionModel_local.h`
- `neo/cm/CollisionModel_load.cpp`
- `neo/cm/CollisionModel_contents.cpp`
- `neo/cm/CollisionModel_translate.cpp`
- `neo/cm/CollisionModel_rotate.cpp`
- `neo/doom.sln`
- `neo/*.vcxproj`
- `neo/_Common.props`, `_Debug.props`, `_Release.props`, `_DoomDLL.props`, `_Game.props`, `_Game-d3xp.props`
- `neo/idlib/precompiled.h`
- `neo/idlib/math/Math.h`
- `neo/idlib/math/Simd.cpp`
- `neo/idlib/Lib.cpp`
- `neo/TypeInfo/main.cpp`
- `neo/TypeInfo/TypeInfoGen.{h,cpp}`
- `neo/framework/FileSystem.{h,cpp}`

Corrections made:

- Added stable `cm_polygon_t::polygonId` and `cm_model_t::nextPolygonId` fields.
- Replaced 32-bit polygon pointer tokens in `contactInfo_t::modelFeature` with stable polygon IDs assigned during collision model allocation/copy/load paths.
- Replaced `GetModelPolygon` pointer reconstruction from `int polygonNum` with a recursive polygon-ID lookup.
- Added Debug/Release x64 Visual Studio solution/project configurations and x64 `TargetMachine`.
- Removed `_USE_32BIT_TIME_T` from x64 builds and added `WIN64` / `_WIN64` definitions.
- Switched TypeInfo pre-build commands from hard-coded `build\Win32\...` to `build\$(PlatformName)\...`.
- Removed stale x64 MFC library injection from Debug/Release props and TypeInfo-specific legacy link metadata.
- Added `/FS` to common compile options to avoid parallel PDB writer failures.
- Gated x86-only SIMD and math inline assembly away from MSVC x64; x64 currently uses the generic SIMD processor.
- Excluded MFC GUI/editor tool source families from DoomDLL x64 while retaining `tools\compilers`.
- Fixed TypeInfo modern C++ string-literal/macro concatenation errors.
- Replaced UCRT-internal `FILE::_file` access with `_fileno(...)` in background download I/O.
- Renamed touched pointer parameters/locals in TypeInfo and FileSystem background-download paths with `p_`.

### Struct Layout / ABI Notes

#### `cm_polygon_t`

Before this slice:

```c
struct cm_polygon_s {
    idBounds bounds;
    ...
};
```

After this slice:

```c
struct cm_polygon_s {
    idBounds bounds;
    int polygonId;
    ...
};
```

Layout impact: in-memory collision polygon layout now includes a stable integer feature ID. Collision model file format is unchanged; IDs are assigned during load/build order.

Binary compatibility: `contactInfo_t::modelFeature` remains `int`, so savegame/event field width is unchanged. Semantics changed from truncated pointer token to stable polygon ID. Old saves/events containing pointer-derived tokens cannot be mapped reliably without a legacy compatibility table.

### Additional Audit Findings

| File + line | Problem | Correct 64-bit fix | ABI / binary impact |
| --- | --- | --- | --- |
| `neo/idlib/math/Vector.h:1760` | Casts `float *` to `int` for alignment/offset logic; warns as pointer truncation on x64. | Use `uintptr_t` for address arithmetic and keep pointer locals named with `p_`. | No file-format impact; math helper ABI unchanged if signatures stay stable. |
| `neo/idlib/math/Matrix.h:2283` | Casts `float *` to `int`; warns as pointer truncation on x64. | Use `uintptr_t` or `intptr_t` depending on signedness. | No file-format impact. |
| `neo/idlib/math/Vector.cpp:387` / `neo/idlib/math/Matrix.cpp:2939` | Round-trips `float *` through `int`, producing x64 truncation and widening warnings. | Replace with pointer-width integer math or pointer arithmetic. | Runtime correctness risk on x64. |
| `neo/idlib/Heap.h:217` | Template code casts node pointer to `int`. | Replace with `uintptr_t`; rename pointer operands with `p_`. | Heap/tree internal only; affects debug ordering/checking if pointer-derived. |
| `neo/framework/CVarSystem.h:292,301,305` | Casts `unsigned int` back to `idCVar *`. | Replace integer cookie with `uintptr_t` or avoid pointer-valued integer cookies. | CVar registration ABI/runtime behavior risk. |
| `neo/TypeInfo/TypeInfoGen.cpp:1019` | Generates `(int)(&((T *)0)->member)` offsets and the x64 TypeInfo pre-build currently crashes when generating game type info. | Generate `offsetof(T, member)` or `ptrdiff_t`/`size_t` offset fields, then version generated type-info structs and debug the x64 generator crash. | Generated type-info ABI changes if field type is widened. |
| `neo/sound/snd_system.cpp:167` | OpenAL `alGetEnumValue` expects mutable `ALubyte *` in this legacy header; string literals are `const char[]`. | Cast through `reinterpret_cast<const ALubyte *>` only if header allows const, or wrap with a helper that safely adapts the legacy OpenAL prototype. | No binary-format impact. |
| `neo/sys/win32/win_cpu.cpp:60,133,177` | MSVC x64 rejects inline x86 assembly for `rdtsc`, CPUID probing, and register reads. | Use `__rdtsc`, `__cpuid` / `__cpuidex`, and x64-safe intrinsics; rename touched pointer arrays to `p_`. | CPU feature detection behavior must be regression tested. |
| `neo/sys/win32/win_main.cpp:1292-1335` | Crash/FPU dump path reads x86 `CONTEXT` fields such as `FloatSave`, `Eax`, `Eip`, and `Esp`. | Add x64 `CONTEXT` handling with `Rip`, `Rsp`, `Rax`, etc.; gate x86-only FPU dump fields. | Crash log format/content changes on x64. |
| `neo/sys/win32/win_main.cpp:1516` | Thread trampoline uses naked function / x86 inline assembly. | Replace with a normal x64-compatible thread entry wrapper and explicit parameter struct. | Thread startup ABI changes internally. |
| `neo/sys/win32/win_shared.cpp:118` | Uses ATL smart wrappers (`CComPtr`, `CComBSTR`, `CComVariant`) while x64 path no longer imports ATL/MFC headers. | Include ATL headers directly if ATL libs are installed, or replace WMI query with plain COM calls / registry fallback. | Runtime system-info path only. |
| `neo/sys/win32/win_shared.cpp:670,697` | Call-stack capture uses x86 inline assembly and 32-bit frame assumptions. | Use `CaptureStackBackTrace` / `RtlCaptureStackBackTrace` or DbgHelp `StackWalk64` with `uintptr_t address_t`. | Debug call-stack behavior changes. |
| `neo/sys/win32/win_syscon.cpp:396` | Uses legacy `GWL_WNDPROC`, which is pointer-sized window data on x64. | Use `GWLP_WNDPROC` and `SetWindowLongPtr` / `GetWindowLongPtr`. | Required for correct window procedure pointer storage on x64. |
| `neo/sys/win32/win_taskkeyhook.cpp:37` | Includes `afxwin.h`; x64 MFC libraries are not installed in this VS environment. | Exclude this legacy MFC hook path from x64 or install matching x64 MFC payload and audit hooks for pointer-width APIs. | Tool/system integration feature unavailable in current x64 build. |

### Pointer Renaming Summary For This Slice

Renamed:

- `main(int argc, char **argv)` -> `main(int argc, char **p_argv)` in `neo/TypeInfo/main.cpp`.
- `idTypeInfoGen *generator` -> `idTypeInfoGen *p_generator`.
- `CreateTypeInfo(const char *path)` -> `CreateTypeInfo(const char *p_path)`.
- `WriteTypeInfo(const char *fileName)` -> `WriteTypeInfo(const char *p_fileName)`.
- TypeInfo locals `files`, `typeInfo`, and `file` -> `p_files`, `p_typeInfo`, and `p_file` in touched functions.
- `CurlWriteFunction(void *ptr, ..., void *stream)` -> `CurlWriteFunction(void *p_ptr, ..., void *p_stream)`.
- `CurlProgressFunction(void *clientp, ...)` -> `CurlProgressFunction(void *p_clientp, ...)`.
- Background download locals `bgl`, `session` -> `p_bgl`, `p_session` in touched paths.
- `BackgroundDownload(backgroundDownload_t *bgl)` -> `BackgroundDownload(backgroundDownload_t *p_bgl)`.

Known remaining:

- Broad p_ pointer renaming remains incomplete across most engine, renderer, game, tool, and idlib files.
- Generated TypeInfo output still emits pointer fields such as `const char * name`; changing that generated ABI needs a versioned type-info pass.

### Verification Log For This Slice

- `rg -n "\*reinterpret_cast<int \*>\(&p\)|\*reinterpret_cast<int \*>\(&poly\)|reinterpret_cast<cm_polygon_t \*\*>\(&polygonNum\)" neo/cm`: no stale collision polygon pointer-token casts found.
- `MSBuild.exe neo\idlib.vcxproj /m /p:Configuration=Debug /p:Platform=x64`: passed and produced `build\x64\Debug\idLib.lib`.
- `MSBuild.exe neo\typeinfo.vcxproj /p:Configuration=Debug /p:Platform=x64`: passed and produced `build\x64\Debug\TypeInfo.exe`.
- `MSBuild.exe neo\doomdll.vcxproj /m /p:Configuration=Debug /p:Platform=x64 /clp:ErrorsOnly`: progressed past project configuration, TypeInfo, MFC metadata, PDB contention, and GUI tool compilation; currently blocked by x64 runtime source errors in `sound\snd_system.cpp`, `sys\win32\win_cpu.cpp`, `sys\win32\win_main.cpp`, `sys\win32\win_shared.cpp`, `sys\win32\win_syscon.cpp`, and `sys\win32\win_taskkeyhook.cpp`.
- `MSBuild.exe neo\doom.sln /m /p:Configuration=Debug /p:Platform=x64 /clp:ErrorsOnly`: `CurlLib`, `idLib`, and `TypeInfo` build, and `game.vcxproj` now calls `build\x64\Debug\TypeInfo.exe`; the game pre-build is blocked because that x64 TypeInfo executable exits with access-violation code `-1073741819`. DoomDLL is independently blocked by the x64 runtime source issues listed above.

## 2026-07-13 TypeInfo Standalone and DoomDLL CPU Slice

Files changed in this slice:

- `neo/TypeInfo/main.cpp`
- `neo/TypeInfo/TypeInfoGen.cpp`
- `neo/game/gamesys/GameTypeInfo.h`
- `neo/game/gamesys/NoGameTypeInfo.h`
- `neo/d3xp/gamesys/NoGameTypeInfo.h`
- `neo/game/gamesys/SysCmds.cpp`
- `neo/d3xp/gamesys/SysCmds.cpp`
- `neo/game/gamesys/TypeInfo.cpp`
- `neo/d3xp/gamesys/TypeInfo.cpp`
- `neo/sound/snd_system.cpp`
- `neo/sound/snd_eax_guid.cpp`
- `neo/sound/OggVorbis/vorbissrc/os.h`
- `neo/sys/win32/rc/doom.rc`
- `neo/sys/win32/win_cpu.cpp`
- `neo/sys/win32/win_syscon.cpp`
- `neo/sys/win32/win_main.cpp`
- `neo/sys/win32/win_shared.cpp`
- `neo/tools/edit_stub.cpp`
- `neo/doomdll.vcxproj`

Corrections made:

- Removed TypeInfo's dependency on full engine filesystem initialization for source enumeration, source loading, and generated-header output.
- Fixed `_findfirst` handle storage in TypeInfo from `int` to `intptr_t`.
- Generated TypeInfo offsets now use `offsetof(...)`, and generated/fallback type-info metadata stores offsets and sizes as `size_t`.
- Added standalone TypeInfo default defines for x64/MSVC platform conditionals and skipped generated callback fragments that are not standalone translation units.
- Fixed TypeInfo private/protected inspection macros for modern MSVC by using `_ALLOW_KEYWORD_MACROS` only in the TypeInfo implementation files.
- Fixed C++ string literal concatenation around `S_COLOR_WHITE` in base and d3xp game command code.
- Adapted legacy OpenAL `ALubyte *` enum-name prototype at the call site without changing the OpenAL headers.
- Replaced Win64 CPU timing, CPUID, APIC ID, DAZ/FTZ, and FPU-control inline assembly in `win_cpu.cpp` with x64-safe intrinsics or x64 no-op diagnostics where the old x87 stack inspection has no x64 equivalent.
- Corrected process affinity masks in `CPUCount` from `DWORD` to `DWORD_PTR`.
- Replaced `SetWindowLong(..., GWL_WNDPROC, (long)...)` with `SetWindowLongPtr(..., GWLP_WNDPROC, LONG_PTR)` for pointer-width window procedure storage.
- Excluded the legacy MFC `win_taskkeyhook.cpp` from normal x64 DoomDLL configurations in addition to the already-excluded dedicated configs.
- Added an x64 exception-context crash dump path in `win_main.cpp` using `Rip`, `Rsp`, `Rax`, and the other 64-bit `CONTEXT` register fields. The x86 `FloatSave`/`Eax` path remains isolated to non-x64 builds.
- Removed the x64 build dependency on the legacy naked `_chkstk` patch by compiling the stack hook only for non-x64 and making `clrstk` an x64 no-op.
- Replaced the ATL wrapper-based WMI video-memory query in `win_shared.cpp` with explicit COM pointers using `IWbemLocator`, `IWbemServices`, `IEnumWbemClassObject`, `SysAllocString`, `VariantInit`, and explicit `Release`.
- Widened debug symbol and module addresses in `win_shared.cpp` from 32-bit `int`/`long`/`DWORD` storage to `address_t`/`DWORD64`, and switched DbgHelp calls to the 64-bit APIs.
- Replaced the x64 call-stack path with `CaptureStackBackTrace` into `address_t`, while retaining the legacy EBP frame walk for non-x64.
- Guarded the OggVorbis `vorbis_ftoi` x87 inline assembly out of MSVC x64 builds, using the existing portable fallback there.
- Made `doom.rc` use SDK `winres.h` and skip MFC/editor resource bundles for x64, with `WIN64;_WIN64` supplied to the x64 resource compiler.
- Linked the existing editor/GUI no-op stubs into x64 DoomDLL and added an x64 EAX GUID owner source for the EAX property GUIDs referenced by the OpenAL sound path.

### Additional ABI / Layout Notes

- `classVariableInfo_t::offset`, `classVariableInfo_t::size`, and `classTypeInfo_t::size` changed from `int` to `size_t` in generated and fallback game type-info headers. This is a debug/type-inspection metadata ABI change only; it is not a savegame or network format.
- `CPUCount` now uses `DWORD_PTR` affinity masks, matching the Win32 API's pointer-sized contract. This changes only local runtime state, not persisted formats.
- `win_cpu.cpp` no longer reports detailed x87 stack state on x64. The old code was 32-bit inline assembly and could not be represented safely in MSVC x64 inline asm. DAZ/FTZ control is preserved via MXCSR intrinsics.
- x64 builds omit the MFC task-key hook in this environment. Win32 configurations keep the original source.
- `win_main.cpp` crash logs now have different register and FPU-state text on x64. This is a diagnostic format change only; no savegame or network data is affected.
- `win_shared.cpp` symbol/cache state now stores module bases and instruction addresses as `address_t`, which is an internal debug ABI change required to avoid 64-bit address truncation.
- x64 editor invocations now bind to existing stub implementations that print the tools are Win32-only. This preserves runtime linkability without pulling MFC editor binaries into the x64 executable.
- The x64 resource payload intentionally omits editor/MFC resource bundles. The core Doom icon resource remains available; editor resource binary compatibility is not preserved for x64 in this slice.
- The OggVorbis x64 fallback may not exactly match x87 `fistp` rounding behavior, but it removes MSVC x64 inline assembly and has no persistent data-format impact.

### Additional Audit Findings

| File + line | Problem | Correct 64-bit fix | ABI / binary impact |
| --- | --- | --- | --- |
| `neo/TypeInfo/main.cpp:158` | `_findfirst` returns an `intptr_t` search handle, but the TypeInfo enumerator stored it in `int`. | Fixed to `intptr_t findhandle`. | Runtime-only fix; no file-format impact. |
| `neo/TypeInfo/TypeInfoGen.cpp:1094` | Generated member offsets used a null-pointer member-address expression narrowed to `int`. | Fixed to generate `offsetof(...)` and `size_t` metadata fields. | TypeInfo metadata ABI changes from 32-bit to pointer-width offsets. |
| `neo/sys/win32/win_cpu.cpp:405` | `GetProcessAffinityMask` results were stored in `DWORD`, truncating masks on systems with more than 32 logical processors. | Fixed to `DWORD_PTR` for process, system, and iterated affinity masks. | Runtime CPU topology detection behavior changes on x64. |
| `neo/sys/win32/win_syscon.cpp:396` | Window procedure pointer was stored through `SetWindowLong` / `GWL_WNDPROC` and cast to `long`. | Fixed to `SetWindowLongPtr` / `GWLP_WNDPROC` with `LONG_PTR`. | Required x64 correctness; no persisted ABI. |
| `neo/sys/win32/win_main.cpp:1300` | x64 exception reporting previously referenced x86-only `CONTEXT` fields such as `FloatSave`, `Eax`, and `Eip`. | Added an `_M_X64` branch that reads x64 register fields and reports unavailable x64 FPU context text. | Crash-log text changes on x64; no persisted ABI. |
| `neo/sys/win32/win_main.cpp:1172,1572` | MSVC x64 cannot compile the legacy naked `_chkstk` hook and inline assembly stack clearing. | Compiled `_chkstk` patching only for non-x64 and made `clrstk` a normal no-op function on x64. | Internal thread/stack diagnostic behavior changes on x64 only. |
| `neo/sys/win32/win_shared.cpp:118,737` | WMI video-memory query and call-stack capture depended on ATL wrappers and x86 EBP frame walking. | Replaced WMI wrappers with explicit COM pointers and x64 call-stack capture with `CaptureStackBackTrace`. | Runtime system-info/debug call-stack behavior changes; no persisted format. |
| `neo/sys/win32/win_shared.cpp:316,550,613` | Debug symbol/module addresses were stored and queried with 32-bit `int`/`long`/`DWORD`-style assumptions. | Widened address storage to `address_t`/`DWORD64` and used `IMAGEHLP_SYMBOL64`/`SymLoadModule64`/`SymGetSymFromAddr64`. | Internal debug ABI change; avoids truncating loaded module bases and instruction pointers. |
| `neo/sound/OggVorbis/vorbissrc/os.h:110` | MSVC x64 tried to compile x87 inline assembly in `vorbis_ftoi`. | Guarded the assembly path with `!defined(_M_X64)` so x64 uses the portable fallback. | Possible tiny rounding behavior difference; no binary format impact. |
| `neo/sys/win32/rc/doom.rc:10` | x64 resource compile pulled MFC `afxres.h` and editor resource bundles even though x64 editor/MFC sources are excluded. | x64 now includes SDK `winres.h` and skips editor/MFC resource includes. | x64 editor resources omitted; core executable resources still build. |
| `neo/doomdll.vcxproj:366,3878` | x64 RC had no 64-bit macro and excluded editor implementations without linking the existing stubs. | Added `WIN64;_WIN64` RC definitions and included `tools\edit_stub.cpp`, `tools\guied\GEWindowWrapper_stub.cpp`, and `sound\snd_eax_guid.cpp` for x64. | x64 runtime links with no-op editor compatibility stubs. |

### Pointer Renaming Summary For This Slice

Renamed in touched TypeInfo and helper code:

- `Sys_ListFiles(const char *directory, const char *extension, ...)` -> `Sys_ListFiles(const char *p_directory, const char *p_extension, ...)`.
- `Sys_CreateThread(xthread_t function, void *parms, ..., const char *name, xthreadInfo *threads[MAX_THREADS], int *thread_count)` -> touched pointer parameters use `p_function`, `p_parms`, `p_name`, `p_threads`, and `p_thread_count`.
- `BuildSourceCodePath(const char *relativePath)` -> `BuildSourceCodePath(const char *p_relativePath)`.
- TypeInfo source/output helpers use `p_fileName`, `p_buffer`, `p_file`, `p_lineStart`, `p_lineEnd`, `p_scan`, and `p_end`.
- `idTypeInfoOutputFile` stores its OS file handle as `FILE *p_file`.
- Newly touched COM and call-stack locals in `win_shared.cpp` use `p_` names such as `p_locator`, `p_services`, `p_enumInstances`, `p_instance`, `p_namespace`, `p_className`, `p_adapterRam`, `p_backTrace`, and `p_callStack`.
- Newly touched DbgHelp symbol pointer locals use `p_` names such as `pSymbol`.

Known remaining:

- Generated `GameTypeInfo.h` still contains original game field names such as `clipLinks`, `sector`, and `mightSee`; the global mandatory `p_` pointer field rename is not complete.
- `win_main.cpp` and `win_shared.cpp` still contain many untouched non-`p_` pointer locals outside this verified slice.

### Verification Log For This Slice

- `MSBuild.exe neo\typeinfo.vcxproj /p:Configuration=Debug /p:Platform=x64 /clp:ErrorsOnly`: passed.
- `build\x64\Debug\TypeInfo.exe` from repo root: passed, processed `neo/game`, and wrote `neo/game/gamesys/GameTypeInfo.h`.
- `MSBuild.exe neo\game.vcxproj /p:Configuration=Debug /p:Platform=x64 /clp:ErrorsOnly`: passed.
- `MSBuild.exe neo\doomdll.vcxproj /p:Configuration=Debug /p:Platform=x64 /clp:ErrorsOnly`: passed after the x64 context/call-stack/resource/stub fixes.
- `build\x64\Debug\DOOM3.exe +set fs_basepath . +quit`: launch command returned exit code 0, but a `DOOM3.exe` process remained live afterward and locked the next link. The process was stopped and the generated `base/config.spec` was removed; treat this as a launch heartbeat, not a clean runtime quit.
- `git diff --check`: passed.

## 2026-07-13 d3xp Script Compiler Const/Pointer Naming Slice

Files changed in this slice:

- `neo/d3xp/script/Script_Compiler.cpp`
- `neo/d3xp/script/Script_Compiler.h`
- `neo/game/script/Script_Compiler.cpp`
- `neo/game/script/Script_Compiler.h`
- `neo/idlib/Lexer.cpp`
- `neo/idlib/Lexer.h`
- `neo/idlib/Parser.cpp`
- `neo/idlib/Parser.h`

Corrections made:

- Fixed the d3xp x64 build break where `idCompiler::idCompiler()` assigned the `const char *` punctuation table to a mutable `char **` iterator.
- Made the base-game script compiler match d3xp by storing punctuation literals as `const char *`, removing mutable string-literal storage from the shared script compiler pattern.
- Renamed the static punctuation pointer table from `punctuation` to `p_punctuation` in both script compiler classes.
- Renamed the local punctuation iterator from `ptr` to `p_punctuationPtr`.
- Renamed the shared parser/lexer punctuation lookup parameter from `p` to `p_punctuation`.

### Additional ABI / Layout Notes

- `idCompiler::p_punctuation` remains a static pointer array and is not serialized. The symbol name changes at C++ compile/link scope only; no savegame, network, or map format changes.
- Const-correct punctuation literals prevent accidental mutation of read-only string storage. Runtime parser behavior is intended to remain unchanged.

### Additional Audit Findings

| File + line | Problem | Correct 64-bit fix | ABI / binary impact |
| --- | --- | --- | --- |
| `neo/d3xp/script/Script_Compiler.cpp:210,233` | A `const char *` punctuation table was iterated through mutable `char **ptr`, which fails modern x64/MSVC const correctness and violates pointer naming rules. | Changed to `const char **p_punctuationPtr` and iterated `p_punctuation`. | Compile/link symbol naming only; no persisted binary format impact. |
| `neo/game/script/Script_Compiler.cpp:41,210,233` | Base game still declared string-literal punctuation storage as mutable `char *` and used a non-`p_` pointer iterator. | Changed the table to `const char *idCompiler::p_punctuation[]` and the iterator to `const char **p_punctuationPtr`. | Compile/link symbol naming only; no persisted binary format impact. |
| `neo/idlib/Lexer.cpp:199` and `neo/idlib/Parser.cpp:3154` | Punctuation lookup parameters were pointer parameters named `p`, not `p_` prefixed. | Renamed both declarations and definitions to `const char *p_punctuation`. | Source/API naming change for C++ callers; call sites are unaffected because the function signature type is unchanged. |

### Pointer Renaming Summary For This Slice

- `idCompiler::punctuation` -> `idCompiler::p_punctuation` in base game and d3xp script compilers.
- `char **ptr` / `const char **ptr` -> `const char **p_punctuationPtr`.
- `GetPunctuationId(const char *p)` -> `GetPunctuationId(const char *p_punctuation)` in `idLexer` and `idParser`.

Known remaining:

- Most script compiler pointer locals and fields outside the punctuation table still need the mandatory `p_` naming pass.
- The broader engine still has many mutable string-literal pointer declarations and non-`p_` pointer members that require staged migration.

### Verification Log For This Slice

- `MSBuild.exe neo\game-d3xp.vcxproj /p:Configuration=Debug /p:Platform=x64 /clp:ErrorsOnly`: passed.
- `MSBuild.exe neo\game.vcxproj /p:Configuration=Debug /p:Platform=x64 /clp:ErrorsOnly`: passed after the shared parser/lexer rename.

## 2026-07-13 Game Event Argument Pointer-Width Slice

Files changed in this slice:

- `neo/game/gamesys/Class.h`
- `neo/game/gamesys/Class.cpp`
- `neo/game/gamesys/Event.h`
- `neo/game/gamesys/Event.cpp`
- `neo/game/script/Script_Interpreter.cpp`
- `neo/d3xp/gamesys/Class.h`
- `neo/d3xp/gamesys/Class.cpp`
- `neo/d3xp/gamesys/Event.h`
- `neo/d3xp/gamesys/Event.cpp`
- `neo/d3xp/script/Script_Interpreter.cpp`

Corrections made:

- Replaced `idEventArg::value` from `int` to `intptr_t` in both base game and d3xp, removing pointer truncation for vector, string, entity, and trace event arguments.
- Replaced `reinterpret_cast<int>( pointer )` event argument construction with `reinterpret_cast<intptr_t>( pointer )`.
- Renamed touched pointer parameters to `p_data` in pointer-taking `idEventArg` constructors and in `ProcessEventArgPtr`.
- Widened transient event callback arrays from `int data[D_EVENT_MAXARGS]` / `int args[D_EVENT_MAXARGS]` to `intptr_t p_data[D_EVENT_MAXARGS]` or `intptr_t args[D_EVENT_MAXARGS]`.
- Widened `idEvent::CopyArgs` and `idClass::ProcessEventArgPtr` to accept pointer-sized transient event argument arrays.
- Replaced pointer writes through `*(T **)&args[i]` / `*(T **)&data[i]` with explicit `reinterpret_cast<intptr_t>(...)` assignments in queued-event and script-event dispatch.
- Widened the d3xp fast-event queue dispatch path to the same pointer-sized transient argument lane.

### Additional ABI / Layout Notes

- `idEventArg` changed from an `int type` + `int value` layout to `int type` + `intptr_t value`. On x64 this grows the transient argument object and changes C++ call ABI for `idEventArg` by-value helper overloads.
- `idClass::ProcessEventArgPtr` and `idEvent::CopyArgs` now use `intptr_t *` / `intptr_t[]` for transient dispatch storage. This is an internal source/ABI change inside the game DLLs.
- Savegame event serialization remains typed: floats, ints/entities, vectors, strings, and traces are still written through the existing save/restore format. This slice does not serialize raw pointer values as 64-bit data.
- The CPU easy-args callback bridge now forwards pointer-sized values for generic argument slots. Integer callbacks still receive their low 32-bit value through the existing callback function casts.

### Additional Audit Findings

| File + line | Problem | Correct 64-bit fix | ABI / binary impact |
| --- | --- | --- | --- |
| `neo/game/gamesys/Class.h:60-69` and `neo/d3xp/gamesys/Class.h:60-69` | `idEventArg::value` was `int`, and pointer arguments were stored through `reinterpret_cast<int>`, truncating addresses on x64. | Changed `value` to `intptr_t` and pointer constructors to `reinterpret_cast<intptr_t>`. | `idEventArg` layout and C++ helper-call ABI change; no savegame format change. |
| `neo/game/gamesys/Event.h:107` and `neo/d3xp/gamesys/Event.h:107` | `idEvent::CopyArgs` copied event argument values into `int data[D_EVENT_MAXARGS]`. | Changed transient copy buffer to `intptr_t p_data[D_EVENT_MAXARGS]`. | Internal event dispatch ABI change. |
| `neo/game/gamesys/Class.cpp:831,939` and `neo/d3xp/gamesys/Class.cpp:831,939` | Immediate event dispatch and `ProcessEventArgPtr` used `int *data` for callback arguments that may be pointers. | Changed to `intptr_t p_data[D_EVENT_MAXARGS]` and `intptr_t *p_data`. | Internal callback bridge ABI change; pointer truncation removed. |
| `neo/game/gamesys/Event.cpp:463-530` and `neo/d3xp/gamesys/Event.cpp:496-661` | Queued event service rebuilt pointer arguments by writing pointers into `int args[]` slots. | Changed event service arrays to `intptr_t args[]` and assigned pointer arguments with explicit `reinterpret_cast<intptr_t>`. | Runtime event dispatch behavior changes on x64 by preserving full addresses. |
| `neo/game/script/Script_Interpreter.cpp:689-924` and `neo/d3xp/script/Script_Interpreter.cpp:689-924` | Script event calls stored vector/string/entity pointers in `int data[]`. | Changed script dispatch buffers to `intptr_t p_data[]` and used pointer-sized casts for vector/string/entity arguments. | Script-to-event bridge now preserves x64 pointer values; script bytecode format unchanged. |

### Pointer Renaming Summary For This Slice

- Pointer constructor parameters `data` -> `p_data` for `const char *`, `const idEntity *`, and `const trace_s *` event arguments.
- `ProcessEventArgPtr(..., int *data)` -> `ProcessEventArgPtr(..., intptr_t *p_data)`.
- Immediate/script transient event buffers named `p_data` where they carry raw pointer-sized callback values.
- Debug trigger local `ent` -> `p_ent` in both game paths.

Known remaining:

- Many event-system locals still use legacy non-`p_` names, including existing `idEvent *event`, `byte *data`, and `const char *format` locals.
- The generated `Callbacks.cpp` non-`CPU_EASYARGS` path is covered by the follow-up generated callback slice below.

### Verification Log For This Slice

- `MSBuild.exe neo\game.vcxproj /p:Configuration=Debug /p:Platform=x64 /clp:ErrorsOnly`: passed.
- `MSBuild.exe neo\game-d3xp.vcxproj /p:Configuration=Debug /p:Platform=x64 /clp:ErrorsOnly`: passed.
- `MSBuild.exe neo\doom.sln /p:Configuration=Debug /p:Platform=x64 /clp:ErrorsOnly`: passed.

## 2026-07-13 Generated Event Callback Pointer-Width Slice

Files changed in this slice:

- `neo/game/gamesys/Callbacks.cpp`
- `neo/game/gamesys/Event.cpp`
- `neo/d3xp/gamesys/Callbacks.cpp`
- `neo/d3xp/gamesys/Event.cpp`

Corrections made:

- Replaced generated non-float callback bridge slots from `const int` to `const intptr_t` in both generated `Callbacks.cpp` files.
- Renamed generated callback argument expressions from `data[...]` to `p_data[...]`, matching the widened `idClass::ProcessEventArgPtr(..., intptr_t *p_data)` signature.
- Updated both `CreateEventCallbackHandler` generator blocks so regenerated callback source preserves the pointer-sized type and `p_`-prefixed data name.

### Additional ABI / Layout Notes

- The non-`CPU_EASYARGS` callback bridge now treats generic callback slots as pointer-sized values. On 32-bit, `intptr_t` remains the same width as the old `int`; on 64-bit non-easyargs targets it preserves full pointer values.
- Callback typedef names such as `eventCallback_i_t` are unchanged because they are type names, not pointer variables.
- No savegame, network, demo, or asset file format changes were made in this slice.
- Current Win64 builds use `CPU_EASYARGS`, so this source path was compile-checked indirectly through solution consistency and targeted scans rather than executed by the Debug x64 build.

### Additional Audit Findings

| File + line | Problem | Correct 64-bit fix | ABI / binary impact |
| --- | --- | --- | --- |
| `neo/game/gamesys/Callbacks.cpp:36` and `neo/d3xp/gamesys/Callbacks.cpp:36` | Generated generic callback typedefs used `const int` for argument slots that may carry pointer-sized event data. | Changed generated typedef slots to `const intptr_t`. | Internal callback bridge ABI change for non-`CPU_EASYARGS` builds; 32-bit width is unchanged, 64-bit pointer truncation is removed. |
| `neo/game/gamesys/Callbacks.cpp:37` and `neo/d3xp/gamesys/Callbacks.cpp:37` | Generated callback expressions used legacy `data[...]` names after the dispatch parameter was renamed and widened. | Changed generated expressions to `p_data[...]`, including float reinterpret paths. | Source-level rename only; no persisted binary format impact. |
| `neo/game/gamesys/Event.cpp:849-852` and `neo/d3xp/gamesys/Event.cpp:1043-1046` | The callback generator still emitted `const int` and `data[...]`, so regeneration would reintroduce the truncation/naming bug. | Updated generator strings to emit `const intptr_t` and `p_data[...]`. | Future regenerated callback source keeps the same internal ABI fix. |

### Pointer Renaming Summary For This Slice

- Generated callback expressions `data[...]` -> `p_data[...]` in base game and d3xp.
- Generator output strings now emit `p_data[...]` for both direct generic slots and float reinterpret slots.

Known remaining:

- Event-system locals outside this slice still include non-`p_` pointer names such as `idEvent *event`, `byte *data`, and `const char *format`.
- The generated callback include still uses legacy type-name encodings such as `eventCallback_i_t`; those are not pointer variables, but a separate readability cleanup could rename them after functional migration.
- More non-event subsystems still need the mandatory pointer-prefix and pointer-width audit.

### Verification Log For This Slice

- `rg -n "p_p_data|\bdata\[|const int\b" neo/game/gamesys/Callbacks.cpp neo/d3xp/gamesys/Callbacks.cpp`: no matches.
- `MSBuild.exe neo\game.vcxproj /p:Configuration=Debug /p:Platform=x64 /clp:ErrorsOnly`: passed.
- `MSBuild.exe neo\game-d3xp.vcxproj /p:Configuration=Debug /p:Platform=x64 /clp:ErrorsOnly`: initially hit `LNK1104` from a parallel shared-output collision, then passed when rerun sequentially.
- `MSBuild.exe neo\doom.sln /p:Configuration=Debug /p:Platform=x64 /clp:ErrorsOnly`: passed.
- `git diff --check`: passed.

## 2026-07-13 idlib Intrusive Container Offset Slice

Files changed in this slice:

- `neo/idlib/Heap.h`
- `neo/idlib/containers/Stack.h`
- `neo/idlib/containers/Queue.h`

Corrections made:

- Replaced intrusive member-offset macros that cast member addresses through `int` with `offsetof(...)` and `size_t` template parameters.
- Replaced the fixed block allocator's free-path offset calculation from `(int)&((element_t *)0)->t` to `offsetof( element_t, t )`.
- Renamed touched intrusive pointer fields and parameters with the mandatory `p_` prefix.

### Additional ABI / Layout Notes

- `idStackTemplate` and `idQueueTemplate` now use `size_t nextOffset` as the non-type template parameter. This is a source/template ABI change only; no serialized data or runtime object layout is persisted by these helpers.
- `idBlockAlloc` private field names changed from `blocks` / `free` to `p_blocks` / `p_free`, and internal link fields changed from `next` to `p_next`. The allocator object layout still contains the same pointer fields in the same order.
- The fixed block allocator no longer truncates the member offset through 32-bit `int` while recovering an allocation header from an object pointer.

### Additional Audit Findings

| File + line | Problem | Correct 64-bit fix | ABI / binary impact |
| --- | --- | --- | --- |
| `neo/idlib/containers/Stack.h:40-43` | `idStack` computed intrusive `next` offsets with `(int)&(((type*)NULL)->next)` and stored the result in an `int` template parameter. | Changed to `offsetof( type, next )` with `size_t nextOffset`. | Source/template ABI change; no persisted binary format impact. |
| `neo/idlib/containers/Queue.h:40-43` | `idQueue` used the same 32-bit member-offset pattern. | Changed to `offsetof( type, next )` with `size_t nextOffset`. | Source/template ABI change; no persisted binary format impact. |
| `neo/idlib/Heap.h:162-233` | `idBlockAlloc::Free` subtracted a member offset after truncating it through `int`; private pointer fields and link fields lacked `p_` prefixes. | Used `offsetof( element_t, t )`; renamed `blocks` -> `p_blocks`, `free` -> `p_free`, `next` -> `p_next`, and pointer params/locals to `p_` names. | Private allocator implementation change; pointer field order is unchanged and no serialized format changes. |

### Pointer Renaming Summary For This Slice

- `idBlockAlloc::blocks` -> `p_blocks`.
- `idBlockAlloc::free` -> `p_free`.
- `element_s::next` / `block_s::next` -> `p_next`.
- `idBlockAlloc::Free(type *element)` -> `Free(type *p_element)`.
- `idStackTemplate` fields `top` / `bottom` -> `p_top` / `p_bottom`; local and parameter `element` -> `p_element`.
- `idQueueTemplate` fields `first` / `last` -> `p_first` / `p_last`; local and parameter `element` -> `p_element`.

Known remaining:

- `idDynamicBlockAlloc` byte-size accounting still uses `int` in several places; pointer fields and touched pointer locals are covered by the later dynamic-block allocator slice.
- Other alignment code still casts pointers through `int`, including SIMD alignment code and renderer paths.

### Verification Log For This Slice

- `MSBuild.exe neo\idlib.vcxproj /p:Configuration=Debug /p:Platform=x64 /clp:ErrorsOnly`: passed.
- `MSBuild.exe neo\doom.sln /p:Configuration=Debug /p:Platform=x64 /clp:ErrorsOnly`: passed.
- `git diff --check`: passed.

## 2026-07-13 idlib Dynamic Block Allocator size_t Slice

Files changed in this slice:

- `neo/idlib/Heap.h`
- `neo/idlib/Str.cpp`
- `neo/renderer/tr_trisurf.cpp`
- `neo/sound/snd_decoder.cpp`
- `neo/sound/snd_local.h`
- `neo/sound/snd_system.cpp`

Corrections made:

- Split `idDynamicBlock::size` away from the base-block flag: block byte size is now `size_t`, while `isBaseBlock` is a separate `bool`.
- Changed the dynamic-block free-tree key from `int` to `size_t` so free blocks are ordered by full byte size.
- Changed dynamic-block allocator byte totals (`baseBlockMemory`, `usedBlockMemory`, `freeBlockMemory`) and memory-stat getters to `size_t`.
- Changed allocation and resize byte math (`alignedBytes`, `allocSize`, split thresholds, merge sizes) to `size_t`.
- Fixed the split-space test to avoid unsigned underflow after widening from signed `int`.
- Updated string, renderer, and sound memory display call sites to cast display-only kilobyte values explicitly for existing `%d` formatting.
- Changed `idSampleDecoder::GetUsedBlockMemory` to return `size_t`.

### Struct Layout / ABI Notes

### `idDynamicBlock<type>`

Before this slice:

```c
int size; // negative meant base block
```

After this slice:

```c
size_t size;
bool isBaseBlock;
```

Layout impact: this is an ABI-breaking template metadata layout change. On x64 the size field widens and a new bool flag is stored separately instead of overloading the sign bit.

Binary compatibility: runtime heap metadata only; no savegame, network, or file-format data is serialized from this struct. All users of the template must be rebuilt.

### `idDynamicBlockAlloc<type, baseBlockSize, minBlockSize>`

Before this slice:

```c
idBTree<idDynamicBlock<type>, int, 4> freeTree;
int baseBlockMemory;
int usedBlockMemory;
int freeBlockMemory;
```

After this slice:

```c
idBTree<idDynamicBlock<type>, size_t, 4> freeTree;
size_t baseBlockMemory;
size_t usedBlockMemory;
size_t freeBlockMemory;
```

Layout impact: private template layout changes on x64 because free-tree key type and memory counters are widened.

Binary compatibility: source/template ABI break only; all dependent modules were rebuilt in the x64 solution verification.

### Additional Audit Findings

| File + line | Problem | Correct 64-bit fix | ABI / binary impact |
| --- | --- | --- | --- |
| `neo/idlib/Heap.h:376-388` | `idDynamicBlock` stored byte size in signed `int` and encoded base-block state by negating the size. | Use `size_t size` and a separate `bool isBaseBlock`. | Runtime allocator metadata layout changes; no serialized format impact. |
| `neo/idlib/Heap.h:408-432` | Dynamic-block memory totals and free-tree key used `int`, limiting allocator byte accounting and free-block ordering. | Use `size_t` getters, counters, and `idBTree` key type. | Private template ABI/layout change. |
| `neo/idlib/Heap.h:718-814` | Allocation, resize, and split byte math used `int`; after widening, the old signed negative split test would wrap as `size_t`. | Use `size_t` for byte math and compare `p_block->GetSize() < alignedBytes + minSplitSize` before splitting. | Fixes a runtime crash observed in `TypeInfo.exe` after initial widening. |
| `neo/idlib/Heap.h:820-870` | Merge/free accounting added block sizes and headers as `int` expressions. | Use `size_t` block sizes and `sizeof` header math. | Runtime behavior preserved with wider accounting. |
| `neo/idlib/Str.cpp:1656-1658` | String allocator stats printed widened byte totals through `%d` without an explicit display-boundary cast. | Cast KB display values to `int` after shifting. | Display-only; no layout or format impact. |
| `neo/renderer/tr_trisurf.cpp:234-280` | Renderer tri-surface allocator stats had the same display-boundary issue. | Cast per-allocator and total KB values explicitly. | Display-only; no layout or format impact. |
| `neo/sound/snd_local.h:884`, `neo/sound/snd_decoder.cpp:363-365`, `neo/sound/snd_system.cpp:255` | Decoder allocator memory API returned `int` even though allocator memory totals are now `size_t`. | Return `size_t` from `idSampleDecoder::GetUsedBlockMemory` and cast only for `%d` display. | Public static helper signature changes; callers in this tree updated. |

### Pointer Renaming Summary For This Slice

- No new pointer renames beyond the prior dynamic-block allocator pointer rename slice.
- This slice focused on byte-size correctness and the allocator metadata layout.

Known remaining:

- `Mem_Alloc`, `Mem_Alloc16`, and platform lock/unlock APIs still accept `int` byte counts, so this slice casts at those existing API boundaries. A later heap API slice should widen those signatures.
- `idBTree` itself still has non-`p_` pointer fields and locals; only its `idDynamicBlockAlloc` instantiation key type was changed here.
- Renderer and sound memory displays still use `%d` formatting with explicit KB casts; a broader formatting modernization could add size-aware formatting helpers.

### Verification Log For This Slice

- `MSBuild.exe neo\idlib.vcxproj /p:Configuration=Debug /p:Platform=x64 /clp:ErrorsOnly`: passed.
- `MSBuild.exe neo\typeinfo.vcxproj /p:Configuration=Debug /p:Platform=x64 /clp:ErrorsOnly`: passed.
- `TypeInfo.exe`: initially crashed after the first size_t widening; after fixing the split underflow guard, direct execution passed and regenerated `neo/game/gamesys/GameTypeInfo.h`.
- `MSBuild.exe neo\doom.sln /p:Configuration=Debug /p:Platform=x64 /clp:ErrorsOnly`: passed.
- `git diff --check`: passed.
- `rg -n "idBTree(Node)?<idDynamicBlock<type>,int|\bint\s+(alignedBytes|allocSize|baseBlockMemory|usedBlockMemory|freeBlockMemory)|\bint\s+Get(BaseBlockMemory|UsedBlockMemory|FreeBlockMemory)|\bint\s+GetSize\( void \)|void\s+SetSize\( int|size = isBaseBlock|size < 0|\(int\)sizeof\( idDynamicBlock<type> \)" neo/idlib/Heap.h`: no stale int size/key patterns.

## 2026-07-13 idlib Dynamic Block Allocator Pointer Rename Slice

Files changed in this slice:

- `neo/idlib/Heap.h`

Corrections made:

- Renamed `idDynamicBlock` pointer fields from `prev`, `next`, `node`, and debug `allocator` to `p_prev`, `p_next`, `p_node`, and `p_allocator`.
- Renamed `idDynamicBlockAlloc` owner fields from `firstBlock` and `lastBlock` to `p_firstBlock` and `p_lastBlock`.
- Renamed dynamic-block allocator pointer parameters and locals including `ptr`, `block`, `next`, `base`, `nextBlock`, `oldBlock`, `newBlock`, and `prevBlock` to `p_` forms.
- Renamed simple dynamic allocator pointer parameters and header memory API pointer parameters touched in this file.
- Replaced C-style allocation-header pointer recovery with typed `reinterpret_cast` byte-pointer arithmetic.
- Rewrote the dormant base-block range-check example to use `uintptr_t` instead of documenting pointer comparisons through `int`.

### Struct Layout / ABI Notes

### `idDynamicBlock<type>`

Before this slice:

```c
template<class type>
class idDynamicBlock {
    int size;
    idDynamicBlock<type> *prev;
    idDynamicBlock<type> *next;
    idBTreeNode<idDynamicBlock<type>,int> *node;
};
```

After this slice:

```c
template<class type>
class idDynamicBlock {
    int size;
    idDynamicBlock<type> *p_prev;
    idDynamicBlock<type> *p_next;
    idBTreeNode<idDynamicBlock<type>,int> *p_node;
};
```

Layout impact: field names changed, but the pointer fields remain in the same order and have the same native pointer width. `DYNAMIC_BLOCK_ALLOC_CHECK` also renames the debug-only `void *allocator` field to `void *p_allocator` without changing layout.

Binary compatibility: this allocator state is runtime heap metadata and is not serialized. All C++ translation units using this template must be rebuilt because source field names changed.

### `idDynamicBlockAlloc<type, baseBlockSize, minBlockSize>`

Before this slice:

```c
idDynamicBlock<type> *firstBlock;
idDynamicBlock<type> *lastBlock;
```

After this slice:

```c
idDynamicBlock<type> *p_firstBlock;
idDynamicBlock<type> *p_lastBlock;
```

Layout impact: field names changed only; pointer order and object size are unchanged.

Binary compatibility: private template implementation change; no savegame, network, or file-format impact.

### Additional Audit Findings

| File + line | Problem | Correct 64-bit fix | ABI / binary impact |
| --- | --- | --- | --- |
| `neo/idlib/Heap.h:70-139` | Heap header pointer parameters still used generic names like `ptr`, `in`, `fileName`, and delete overload local `p`. | Renamed touched pointer parameters to `p_ptr`, `p_in`, `p_fileName`, and `p_str`. | Header source change only; function types are unchanged. |
| `neo/idlib/Heap.h:372-389` | `idDynamicBlock` pointer fields did not follow the required `p_` convention. | Renamed `prev`, `next`, `node`, and debug `allocator` to `p_prev`, `p_next`, `p_node`, and `p_allocator`. | Template field-name ABI/source break; native layout unchanged. |
| `neo/idlib/Heap.h:391-445` | `idDynamicBlockAlloc` pointer owner fields and internal pointer parameters used non-`p_` names. | Renamed fields and internal function parameters to `p_` names. | Template source break; private object layout unchanged. |
| `neo/idlib/Heap.h:601-662` | Allocation-header recovery used C-style casts and non-`p_` pointer names when moving from user memory back to `idDynamicBlock`. | Renamed user pointer params to `p_ptr` and used typed `reinterpret_cast<byte *>` arithmetic. | Runtime behavior unchanged; pointer arithmetic remains native-width. |
| `neo/idlib/Heap.h:676-688` | The disabled base-block validation snippet documented live-address comparisons by casting block/base pointers through `int`. | Rewrote the snippet to compare `uintptr_t` addresses. | Diagnostic-only commented code; documents the correct 64-bit repair. |
| `neo/idlib/Heap.h:716-888` | Dynamic block split/merge code used many non-`p_` block pointer locals and fields. | Renamed block pointer locals/fields throughout allocation, resize, split, merge, link, unlink, and check paths. | Template source break; runtime metadata layout unchanged. |

### Pointer Renaming Summary For This Slice

- `idDynamicBlock::prev` -> `p_prev`.
- `idDynamicBlock::next` -> `p_next`.
- `idDynamicBlock::node` -> `p_node`.
- `idDynamicBlock::allocator` -> `p_allocator`.
- `idDynamicBlockAlloc::firstBlock` -> `p_firstBlock`.
- `idDynamicBlockAlloc::lastBlock` -> `p_lastBlock`.
- Dynamic allocator locals/params: `ptr`, `block`, `next`, `base`, `nextBlock`, `oldBlock`, `newBlock`, `prevBlock` -> `p_ptr`, `p_block`, `p_next`, `p_base`, `p_nextBlock`, `p_oldBlock`, `p_newBlock`, `p_prevBlock`.
- Header memory API pointer parameters: `ptr`, `in`, `fileName`, delete local `p`, and macro pointer args -> `p_ptr`, `p_in`, `p_fileName`, `p_str`.

Known remaining:

- `idDynamicBlockAlloc` still uses `int` for byte-size accounting (`alignedBytes`, `allocSize`, `baseBlockMemory`, and related stats). A later size-type slice should audit whether these can become `size_t` without breaking B-tree key semantics.
- The simple `idDynamicAlloc` wrapper still tracks byte totals as `int`.
- Other allocator and container templates outside this focused slice still need a full pointer-prefix audit.

### Verification Log For This Slice

- `MSBuild.exe neo\idlib.vcxproj /p:Configuration=Debug /p:Platform=x64 /clp:ErrorsOnly`: passed.
- `MSBuild.exe neo\doom.sln /p:Configuration=Debug /p:Platform=x64 /clp:ErrorsOnly`: passed.
- `git diff --check`: passed.
- `rg -n "\b(firstBlock|lastBlock|prev|next|node|allocator|ptr|nextBlock|oldBlock|newBlock|prevBlock)\b|\(\s*int\s*\)\s*(p_block|p_base|p_ptr|p_newBlock|p_prevBlock|p_nextBlock|firstBlock|lastBlock|block|base|ptr)|\bvoid \*p\b|\bconst char \*in\b|\bfileName\b" neo/idlib/Heap.h`: no stale pointer identifiers or pointer-through-int casts except ordinary prose and already-prefixed names.
- `rg -n "\(\s*int\s*\)p_block|\(\s*int\s*\)p_base|\(\s*int\s*\)p_ptr|\(\s*int\s*\)\s*p_firstBlock|\(\s*int\s*\)\s*p_lastBlock|\(\s*byte \*\s*\)\s*p_ptr|\(\s*byte \*\s*\)\s*p_block" neo/idlib/Heap.h`: no matches.

## 2026-07-13 SIMD Static Buffer and Legacy Alignment Slice

Files changed in this slice:

- `neo/idlib/math/Vector.cpp`
- `neo/idlib/math/Matrix.cpp`
- `neo/idlib/math/Simd_MMX.cpp`
- `neo/idlib/math/Simd_SSE.cpp`
- `neo/idlib/math/Simd_SSE2.cpp`

Corrections made:

- Replaced static `idVecX` and `idMatX` scratch-buffer alignment expressions that truncated array addresses through `int`.
- Replaced MMX `Memcpy`/`Memset` pointer-alignment masks that compared or masked `void *`/`byte *` through `int`.
- Replaced SSE comparison macro source-alignment tests and aligned-pointer calculations that cast `SRC0`/`aligned` through `int`.
- Replaced the legacy Mac SSE2 `CmpLT` alignment calculation with `uintptr_t` and renamed touched pointer parameters/locals with the `p_` prefix.
- Preserved existing byte/element loop counters as `int`; these are counts, not pointer storage.

### Additional ABI / Layout Notes

- No struct or class layout changed in this slice.
- `idVecX::tempPtr` and `idMatX::tempPtr` still point into their existing static scratch arrays; only the address arithmetic used to compute the 16-byte-aligned pointer was widened.
- `idSIMD_MMX::Memcpy` and `idSIMD_MMX::Memset` signatures are type-identical except for parameter names, so the C++ call ABI is unchanged.
- The SSE macro changes affect 32-bit inline-assembly code paths; the Windows x64 solution build does not compile those inline-assembly blocks, but the source no longer truncates addresses before alignment decisions.
- The legacy `MACOS_X && __i386__` SSE2 path is source-updated but was not compiled by the Windows x64 verification.

### Additional Audit Findings

| File + line | Problem | Correct 64-bit fix | ABI / binary impact |
| --- | --- | --- | --- |
| `neo/idlib/math/Vector.cpp:386-388` | `idVecX::tempPtr` aligned `idVecX::temp` by casting its address to `int`. | Use `reinterpret_cast<uintptr_t>` and a pointer-width 16-byte mask. | No layout or serialized format impact; static scratch pointer is computed without truncation. |
| `neo/idlib/math/Matrix.cpp:2938-2940` | `idMatX::tempPtr` used the same 32-bit address alignment pattern. | Use `reinterpret_cast<uintptr_t>` and a pointer-width 16-byte mask. | No layout or serialized format impact; static scratch pointer is computed without truncation. |
| `neo/idlib/math/Simd_MMX.cpp:226-267` | `Memcpy` compared destination/source alignment by XORing pointers after `int` casts and masked a destination pointer through `int`. | Use `uintptr_t` for the XOR and low-bit mask; rename touched pointer params/locals. | Function type ABI unchanged; x86 MMX behavior preserved with pointer-width address checks. |
| `neo/idlib/math/Simd_MMX.cpp:281-359` | `Memset` tested destination alignment by casting `dest` through `int`. | Use `uintptr_t` for the low-bit mask and rename the touched destination pointer. | Function type ABI unchanged; x86 MMX behavior preserved with pointer-width address checks. |
| `neo/idlib/math/Simd_SSE.cpp:3275-3459` | SSE comparison macros tested and rounded source pointers through `int`, then compared `(int)aligned` with `(int)src0 + COUNT`. | Use `uintptr_t` for rounding, `const float *p_aligned` for the aligned pointer, and typed pointer comparisons. | Macro-generated code changes in legacy SSE source; Windows x64 does not compile the inline assembly path. |
| `neo/idlib/math/Simd_SSE2.cpp:66-245` | Legacy Mac SSE2 `CmpLT` used `int` address masks/rounding and non-`p_` pointer parameters/locals. | Rename touched pointers to `p_dst`, `p_src0`, `p_aligned`, `p_src0Bytes`, `p_constantBytes`, and `p_dstBytes`; use `uintptr_t` for alignment. | Source-only legacy path in this verification environment; no Windows x64 object impact. |

### Pointer Renaming Summary For This Slice

- `idSIMD_MMX::Memcpy( dest0, src0 )` -> `idSIMD_MMX::Memcpy( p_dest0, p_src0 )`.
- MMX locals `dest` / `src` -> `p_dest` / `p_src`.
- `idSIMD_MMX::Memset( dest0 )` -> `idSIMD_MMX::Memset( p_dest0 )`.
- SSE comparison macro local `aligned` -> `p_aligned`.
- Legacy SSE2 `CmpLT` parameters `dst` / `src0` -> `p_dst` / `p_src0`.
- Legacy SSE2 locals `aligned`, `src0_p`, `constant_p`, and `dst_p` -> `p_aligned`, `p_src0Bytes`, `p_constantBytes`, and `p_dstBytes`.

Known remaining:

- `neo/idlib/math/Simd_SSE.cpp` still contains many pointer parameters and locals without the `p_` prefix outside the two comparison macros changed here, including older `constant_p` and `dst_p` locals near the top of the file.
- `neo/idlib/math/Simd_SSE.cpp`, `Simd_SSE3.cpp`, and related files still contain offset asserts written as `(int)&((type *)0)->field`; these are offset checks rather than live pointer storage, but they should be converted to `offsetof`/`uintptr_t` in a future cleanup.
- `neo/idlib/Heap.h` still has dynamic-block pointer comparisons through `int` and should be the next allocator-focused slice.

### Verification Log For This Slice

- `MSBuild.exe neo\idlib.vcxproj /p:Configuration=Debug /p:Platform=x64 /clp:ErrorsOnly`: passed.
- `MSBuild.exe neo\doom.sln /p:Configuration=Debug /p:Platform=x64 /clp:ErrorsOnly`: passed.
- `git diff --check`: passed.
- `rg -n "\(\s*int\s*\)\s*(idVecX::temp|idMatX::temp|dest0|src0|dest\b|src\b|SRC0|aligned\b)|\(int\)aligned|\bfloat \*aligned\b|\bsrc0_p\b|\bdst_p\b|\bconstant_p\b" neo/idlib/math/Vector.cpp neo/idlib/math/Matrix.cpp neo/idlib/math/Simd_MMX.cpp neo/idlib/math/Simd_SSE2.cpp`: no matches.
- `rg -n "\(\s*int\s*\)\s*(idVecX::temp|idMatX::temp|dest0|src0|dest\b|src\b|SRC0|aligned\b)|\(int\)aligned|\bfloat \*aligned\b" neo/idlib/math/Simd_SSE.cpp`: no matches.

## 2026-07-13 System DLL and Thread Handle Width Slice

Files changed in this slice:

- `neo/sys/sys_public.h`
- `neo/sys/sys_local.h`
- `neo/sys/sys_local.cpp`
- `neo/sys/win32/win_main.cpp`
- `neo/sys/posix/posix_main.cpp`
- `neo/sys/posix/posix_threads.cpp`
- `neo/framework/Common.cpp`
- `neo/framework/FileSystem.cpp`
- `neo/TypeInfo/main.cpp`

Corrections made:

- Added `sysHandle_t` as a native pointer-width handle type backed by `uintptr_t`.
- Widened DLL load/proc/unload APIs from `int` handles to `sysHandle_t`.
- Widened `xthreadInfo::threadHandle` from `int` to `sysHandle_t` and renamed it to `p_threadHandle`.
- Replaced Win32 `HANDLE` and `HINSTANCE` casts through `int` with `reinterpret_cast<sysHandle_t>` and typed casts back to the native handle type.
- Replaced POSIX `dlopen` handle casts through `int` with `reinterpret_cast<sysHandle_t>`.
- Updated `idCommonLocal`'s loaded-game-DLL member from `int gameDLL` to `sysHandle_t p_gameDLL`.
- Renamed touched pointer/native-handle parameters and locals with the `p_` prefix, including DLL handles, DLL names, proc names, POSIX handles, and thread handles.

### Struct Layout / ABI Notes

### `xthreadInfo`

Before this slice:

```c
typedef struct {
    const char *name;
    int threadHandle;
    unsigned long threadId;
} xthreadInfo;
```

After this slice:

```c
typedef struct {
    const char *name;
    sysHandle_t p_threadHandle;
    unsigned long threadId;
} xthreadInfo;
```

Layout impact: `threadHandle` expanded from 32 bits to pointer width. On MSVC x64 this preserves full `HANDLE` values and changes the native struct layout/alignment.

Binary compatibility: `xthreadInfo` is runtime-only thread state and is not serialized. Any binary module built against the old header must be rebuilt because the field name and layout changed.

### `idSys` DLL Handle Interface

Before this slice:

```c
virtual int DLL_Load( const char *dllName ) = 0;
virtual void *DLL_GetProcAddress( int dllHandle, const char *procName ) = 0;
virtual void DLL_Unload( int dllHandle ) = 0;
```

After this slice:

```c
virtual sysHandle_t DLL_Load( const char *p_dllName ) = 0;
virtual void *DLL_GetProcAddress( sysHandle_t p_dllHandle, const char *p_procName ) = 0;
virtual void DLL_Unload( sysHandle_t p_dllHandle ) = 0;
```

Layout impact: no object data layout change in `idSys`, but the virtual function signatures changed.

Binary compatibility: this is an engine interface ABI break. Engine, tool, and DLL modules using `idSys` must be rebuilt together.

### `idCommonLocal`

Before this slice:

```c
int gameDLL;
```

After this slice:

```c
sysHandle_t p_gameDLL;
```

Layout impact: internal `idCommonLocal` object layout changes on x64. The field now preserves full native DLL handles.

Binary compatibility: internal engine object layout change only; no savegame, network, or file-format impact.

### Additional Audit Findings

| File + line | Problem | Correct 64-bit fix | ABI / binary impact |
| --- | --- | --- | --- |
| `neo/sys/sys_public.h:247,342-344,503,574-576` | DLL and thread handles were represented as `int`, which truncates native `HANDLE`, `HINSTANCE`, and `dlopen` handles on 64-bit builds. | Added `sysHandle_t`, changed DLL APIs to use it, and changed `xthreadInfo` to store `sysHandle_t p_threadHandle`. | `xthreadInfo` layout changes; `idSys` virtual signatures change and require a synchronized rebuild. |
| `neo/sys/win32/win_main.cpp:93-122` | `CreateThread` returned a `HANDLE` that was stored in an `int` field and later cast back to `HANDLE`. | Store the value as `sysHandle_t p_threadHandle` with `reinterpret_cast`, then cast back only at Win32 API boundaries. | Runtime thread handle preservation fixed; field rename requires source rebuild. |
| `neo/sys/win32/win_main.cpp:658-692` | `LoadLibrary`, `GetProcAddress`, and `FreeLibrary` moved `HINSTANCE` values through `int` DLL handles. | Use `sysHandle_t p_dllHandle` and typed `reinterpret_cast` conversions at the platform API boundary. | DLL loader ABI changes from 32-bit int handles to pointer-width handles. |
| `neo/sys/posix/posix_main.cpp:388-415` | `dlopen` handles were cast through `int`. | Store and pass `dlopen` handles through `sysHandle_t`; reinterpret only at `dlsym`/`dlclose` boundaries. | POSIX source updated; this Windows x64 slice did not compile the POSIX path. |
| `neo/sys/posix/posix_threads.cpp:164-228` | POSIX thread handle storage used the same 32-bit `xthreadInfo::threadHandle` field. | Store the handle in `sysHandle_t p_threadHandle` and rename the touched local to `p_threadHandle`. | POSIX source updated; verify on a POSIX compiler because `pthread_t` representation is platform-specific. |
| `neo/framework/Common.cpp:196,228,2644-2709` | Loaded game DLL handle was stored as `int gameDLL`. | Store it as `sysHandle_t p_gameDLL` and pass it to the widened loader APIs. | Internal `idCommonLocal` layout changes; no serialized format impact. |
| `neo/framework/FileSystem.cpp:3748-3750` | Background thread checks referenced the old thread field name after the struct layout change. | Updated uses to `backgroundThread.p_threadHandle`. | Source compatibility update only. |
| `neo/TypeInfo/main.cpp:244-246` and `neo/sys/sys_local.cpp:111-120` | System stubs and wrappers still exposed the old DLL handle type. | Updated wrapper/stub signatures to `sysHandle_t` and renamed touched pointer/native-handle parameters. | TypeInfo/sys-local ABI stays aligned with `idSys`. |

### Pointer Renaming Summary For This Slice

- `xthreadInfo::threadHandle` -> `xthreadInfo::p_threadHandle`.
- Win32 local `HANDLE temp` -> `HANDLE p_threadHandle`.
- POSIX local `pthread_t threadHandle` -> `pthread_t p_threadHandle`.
- `idCommonLocal::gameDLL` -> `idCommonLocal::p_gameDLL`.
- DLL parameters `dllName`, `dllHandle`, and `procName` -> `p_dllName`, `p_dllHandle`, and `p_procName` in touched declarations/definitions.
- POSIX DLL parameters/locals `path`, `handle`, `sym`, `error`, and `ret` -> `p_path`, `p_handle`, `p_sym`, `p_error`, and `p_ret` in touched definitions.

Known remaining:

- POSIX thread handle casts should be compiled on a POSIX target; `pthread_t` is not guaranteed to be integer-like on every platform.
- Other handle-like integer fields, especially renderer/windowing handles and file checksums with misleading names, still require a broader audit.
- This slice did not touch serialized savegame/network formats because these system DLL/thread handles are runtime-only.

### Verification Log For This Slice

- `MSBuild.exe neo\doom.sln /p:Configuration=Debug /p:Platform=x64 /clp:ErrorsOnly`: first rerun timed out at the command limit with no errors; second rerun with a longer timeout passed.
- `git diff --check`: passed.
- `rg -n "\bint\s+Sys_DLL_Load|Sys_DLL_GetProcAddress\( int|Sys_DLL_Unload\( int|virtual int\s+DLL_Load|DLL_GetProcAddress\( int|DLL_Unload\( int|threadHandle = \(int\)|\(int\).*libHandle|\(HINSTANCE\)dllHandle|\(HANDLE\).*threadHandle|\( pthread_t \).*threadHandle|\(void \*\)handle" neo/sys neo/framework neo/TypeInfo --glob "*.h" --glob "*.cpp"`: no matches.
- `rg -n "\bthreadHandle\b|\bgameDLL\b" neo/sys neo/framework/Common.cpp --glob "*.h" --glob "*.cpp"`: no matches.

## 2026-07-13 UI Transition Member Offset Slice

Files changed in this slice:

- `neo/ui/Window.h`
- `neo/ui/Window.cpp`
- `neo/ui/SimpleWindow.h`
- `neo/ui/SimpleWindow.cpp`

Corrections made:

- Replaced `idWindow` / `idSimpleWindow` member-offset calculations that used `(int)&((Type*)0)->field` with `offsetof(...)`.
- Widened `idTransitionData::offset` from `int` to `ptrdiff_t` for in-memory transition state.
- Renamed `idTransitionData::data` to `p_data` and updated transition helper pointer parameters/locals with `p_` names.
- Updated `GetWinVarOffset` to return `ptrdiff_t` and to use `p_`-prefixed pointer parameters.
- Kept demo/savegame transition offset I/O explicitly 32-bit for this slice by reading/writing `offset32` locals, making the compatibility boundary visible instead of implicit.

### Additional ABI / Layout Notes

- `idTransitionData` layout changed: the pointer field is now named `p_data`, and `offset` is pointer-difference sized in memory. On x64 this can alter padding/layout for any compiled code that embeds `idTransitionData`.
- `idWindow::GetWinVarOffset` and `idSimpleWindow::GetWinVarOffset` have source/ABI signature changes from `int` to `ptrdiff_t`.
- The demo/savegame byte format still stores transition offsets as 32-bit integers in this slice. This preserves existing file compatibility but leaves a documented format-version follow-up for fully 64-bit serialized offsets.
- The offset values are member offsets inside `idWindow`/`idSimpleWindow`, not raw process pointers. They are expected to remain small, but the in-memory representation no longer assumes that through a 32-bit type.

### Additional Audit Findings

| File + line | Problem | Correct 64-bit fix | ABI / binary impact |
| --- | --- | --- | --- |
| `neo/ui/Window.h:160-162` | `idTransitionData` stored a pointer field named `data` and a member offset in `int`. | Renamed the pointer field to `p_data` and widened `offset` to `ptrdiff_t`. | Struct layout and C++ ABI change; no direct persisted struct dump. |
| `neo/ui/Window.cpp:1717-1766` | `idWindow::GetWinVarOffset` returned `int` offsets computed through null-pointer member address casts. | Changed the return type to `ptrdiff_t`, renamed pointer params, and used `offsetof( idWindow, field )`. | Source/ABI signature change; removes pointer-derived 32-bit cast. |
| `neo/ui/SimpleWindow.cpp:257-290` | `idSimpleWindow::GetWinVarOffset` used the same 32-bit null-pointer offset pattern. | Changed the return type to `ptrdiff_t`, renamed pointer params, and used `offsetof( idSimpleWindow, field )`. | Source/ABI signature change; removes pointer-derived 32-bit cast. |
| `neo/ui/Window.cpp:3238-3241,3361,3423-3453` | Demo/save paths read or wrote transition offsets straight through the in-memory `int` field. | Used explicit `int offset32` locals for the compatibility format, then assigned to/from the widened `ptrdiff_t` field. | File format remains 32-bit compatible for now; follow-up needed for versioned 64-bit serialized offsets. |
| `neo/ui/Window.cpp:3777-3815` | Transition fixup compared stored offsets to 32-bit null-pointer member casts and wrote through a non-prefixed pointer field. | Compared against `offsetof(...)` and assigned the renamed `p_data` field. | Runtime behavior intended unchanged; pointer-derived truncation removed. |

### Pointer Renaming Summary For This Slice

- `idTransitionData::data` -> `idTransitionData::p_data`.
- `GetWinVarOffset( idWinVar *wv, drawWin_t *owner/dw )` -> `GetWinVarOffset( idWinVar *p_wv, drawWin_t *p_owner/p_dw )`.
- `AddTransition(idWinVar *dest, ...)` -> `AddTransition(idWinVar *p_dest, ...)`.
- Touched transition locals: `data` -> `p_transition`, `v4` -> `p_v4`, `r` -> `p_rect`, `val` -> `p_val`, `dw` -> `p_drawWin`, `fdw` -> `p_foundDrawWin`, `strVar` -> `p_strVar`.

Known remaining:

- UI demo/save transition offsets still use 32-bit serialized fields for compatibility. A later versioned format should add 64-bit offset records and a backward-compatible reader.
- Other UI expression paths still store pointer-like values through integer fields, including `wexpOp_t::a` casts in `Window.cpp`.
- Many `idWindow` and `idSimpleWindow` pointer fields remain non-`p_` prefixed outside this focused transition slice.

### Verification Log For This Slice

- `MSBuild.exe neo\doom.sln /p:Configuration=Debug /p:Platform=x64 /clp:ErrorsOnly`: passed.
- `git diff --check`: passed.
- `rg -n "\.data|->data|\(int\)&\( \( id(Window|SimpleWindow) \* \) 0 \)->|ReadInt \( td.offset|WriteInt\( transitions\[i\]\.offset|int idWindow::GetWinVarOffset|int idSimpleWindow::GetWinVarOffset|idWinVar \*dest|idTransitionData \*data|idTransitionData data" neo/ui/Window.cpp neo/ui/Window.h neo/ui/SimpleWindow.cpp neo/ui/SimpleWindow.h`: no matches.

## 2026-07-13 UI Expression Op Pointer Slot Slice

Files changed in this slice:

- `neo/ui/Window.h`
- `neo/ui/Window.cpp`

Corrections made:

- Widened `wexpOp_t::a` from `int` to `intptr_t` because it carries both register/table indices and raw `idWinVar *` / deferred `char *` values.
- Updated `EmitOp` and `ParseEmitOp` to accept `intptr_t a`.
- Replaced expression parser pointer casts from `(int)var` / `(int)p` with `reinterpret_cast<intptr_t>(...)`.
- Replaced expression evaluator pointer reads from C-style casts of `op->a` with explicit `reinterpret_cast<... *>( op->a )`.
- Renamed touched expression parser/evaluator pointer parameters and locals to `p_` names.
- Kept legacy demo expression-op reads explicit by reading `a` through an `int a32` compatibility local before assigning to the widened slot.

### Additional ABI / Layout Notes

- `wexpOp_t` layout changed because `a` is now pointer-sized. This affects in-memory UI expression op storage.
- The demo reader still consumes 32-bit `a` values for compatibility with the existing render-demo format. That path can only safely restore register/table indices from serialized data; raw process pointer values are fixed up from names at runtime.
- Savegame/demo format versioning for UI expression ops remains a follow-up if serialized expression pointer/index records are re-enabled or expanded.

### Additional Audit Findings

| File + line | Problem | Correct 64-bit fix | ABI / binary impact |
| --- | --- | --- | --- |
| `neo/ui/Window.h:108-112` | `wexpOp_t::a` was `int` even though `WOP_TYPE_VAR*` stores raw `idWinVar *` values and deferred `char *` names in it. | Changed `a` to `intptr_t`. | `wexpOp_t` in-memory layout/ABI changes on x64. |
| `neo/ui/Window.cpp:2766-2819` | `EmitOp` / `ParseEmitOp` accepted the pointer-carrying slot as `int`. | Changed the slot parameter to `intptr_t`. | Source/ABI signature change for internal UI expression parsing. |
| `neo/ui/Window.cpp:2830-2909` | Expression parsing cast `idWinVar *` and deferred `char *` names to `int`. | Used `reinterpret_cast<intptr_t>` and renamed touched pointer variables to `p_var` / `p_name`. | Runtime expression ops preserve full x64 addresses. |
| `neo/ui/Window.cpp:3069-3144` | Expression evaluation cast `op->a` back from a 32-bit integer to typed pointers. | Reinterpreted from `intptr_t`, renamed locals to `p_var`, and kept table indices explicitly cast to `int`. | Pointer truncation removed; register/table index behavior preserved. |
| `neo/ui/Window.cpp:3276-3283` | Demo reading previously read directly into `w.a` while it was an `int`. | Read the serialized field through `int a32`, then assigned to the widened `intptr_t` slot. | Existing demo byte format remains 32-bit compatible. |
| `neo/ui/Window.cpp:3867-3874` | Deferred-name fixup cast `ops[i].a` to `const char *`, then wrote the resolved pointer back through `(int)var`. | Used `reinterpret_cast<const char *>` for the name and `reinterpret_cast<intptr_t>` for the resolved `idWinVar *`. | Runtime fixup now preserves x64 pointers. |

### Pointer Renaming Summary For This Slice

- `ParseExpression(..., idWinVar *var)` -> `ParseExpression(..., idWinVar *p_var)`.
- `ParseTerm(..., idWinVar *var)` -> `ParseTerm(..., idWinVar *p_var)`.
- `ParseExpressionPriority(..., idWinVar *var)` -> `ParseExpressionPriority(..., idWinVar *p_var)`.
- Deferred expression local `char *p` -> `char *p_name`.
- Evaluator locals `table` / `var` -> `p_table` / `p_var`.

Known remaining:

- Other UI code outside the expression-op path still has non-`p_` pointer locals and parameters.
- The render-demo expression-op format still stores `a` as a 32-bit field; a future format bump should add a 64-bit record or avoid serializing pointer-carrying expression ops entirely.

### Verification Log For This Slice

- `MSBuild.exe neo\doom.sln /p:Configuration=Debug /p:Platform=x64 /clp:ErrorsOnly`: passed after correcting `idWinVec4` component access through its `const idVec4&` conversion.
- `git diff --check`: passed.
- `rg -n "\(int\)var|\(int\)p\b|\(const char \*\)\(ops\[i\]\.a\)|ops\[i\]\.a = \(int\)|f->ReadInt\( w\.a \)|idWin(Vec4|Float|Int|Bool|Str) \*var|int EmitOp\( int|int ParseEmitOp\( idParser \*src, int a" neo/ui/Window.cpp neo/ui/Window.h`: no matches in the touched expression-op path.

## 2026-07-13 Sound Alignment and Savegame Pointer Sentinel Slice

Files changed in this slice:

- `neo/sound/snd_local.h`
- `neo/sound/snd_system.cpp`
- `neo/sound/snd_world.cpp`

Corrections made:

- Replaced stack/static sound buffer alignment casts that converted pointers through `int` with `uintptr_t` alignment expressions.
- Renamed touched local sound mix pointers to `p_mix` and `p_alignedInputSamples`.
- Stopped writing `idSoundChannel` pointer values as 32-bit savegame integers. The save format still writes three 32-bit marker fields, but they now contain explicit 0/1 presence markers instead of truncated addresses.
- Stopped reading 32-bit marker fields directly into pointer storage. The read path consumes marker ints into integer locals, clears the pointer fields, and uses a per-channel decoder marker to allocate a fresh decoder only for active saved channels.
- Tightened the active-channel savegame bounds check from `channel > SOUND_MAX_CHANNELS` to `channel >= SOUND_MAX_CHANNELS` before indexing the per-channel marker array.

### Additional ABI / Layout Notes

- `idSoundWorldLocal::ReadFromSaveGameSoundChannel` now returns a `bool` decoder-presence marker and uses `p_`-prefixed pointer parameters. This is a C++ virtual signature change inside the sound world implementation.
- The savegame byte layout for these channel fields remains 32-bit marker compatible: older saves that wrote truncated nonzero pointer values still read as nonzero markers; new saves write deterministic 0/1 markers.
- Runtime pointer fields `leadinSample`, `soundShader`, and `decoder` are no longer populated with invalid 32-bit pointer fragments during load.
- Sound mix buffer layout is unchanged; only the alignment arithmetic was widened to pointer-sized unsigned math.

### Additional Audit Findings

| File + line | Problem | Correct 64-bit fix | ABI / binary impact |
| --- | --- | --- | --- |
| `neo/sound/snd_system.cpp:324` | `realAccum` was aligned by casting its address through `int`, truncating the pointer on x64. | Changed to `reinterpret_cast<uintptr_t>( realAccum )` with a `uintptr_t` mask. | Runtime behavior preserved; pointer truncation removed. |
| `neo/sound/snd_world.cpp:591` | AVI mix stack buffer alignment used `(int)mix`. | Changed to `uintptr_t` alignment and renamed the local pointer `p_mix`. | Runtime behavior preserved; pointer truncation removed. |
| `neo/sound/snd_world.cpp:1192-1203` | Sound channel save wrote `leadinSample`, `soundShader`, and `decoder` pointers as 32-bit ints. | Wrote explicit 0/1 marker ints instead of pointer values. | Savegame field semantics changed from raw pointer fragments to markers while keeping the same 32-bit field width. |
| `neo/sound/snd_world.cpp:1277-1301` and `neo/sound/snd_world.cpp:1364-1395` | Sound channel load read 32-bit values directly into pointer fields and tested the resulting invalid pointer. | Read marker ints into integer locals, returned the decoder marker, cleared pointer fields, and allocated a fresh decoder for active marked channels. | Backward-compatible with old nonzero markers; avoids invalid pointer values in object state. |
| `neo/sound/snd_world.cpp:1729` | Channel input sample alignment used `(int)inputSamples`. | Changed to `uintptr_t` alignment and renamed the local pointer `p_alignedInputSamples`. | Runtime behavior preserved; pointer truncation removed. |

### Pointer Renaming Summary For This Slice

- `mix_p` -> `p_mix`.
- `alignedInputSamples` -> `p_alignedInputSamples`.
- Sound save helper parameters `saveGame` / `ch` / `params` -> `p_saveGame` / `p_ch` / `p_params`.
- Active sound channel load local `chan` -> `p_chan`.

Known remaining:

- Sound system class fields such as `finalMixBuffer` and many sound-channel locals still need a broader pointer-prefix pass.
- Other savegame systems still contain enum casts through `int&`; these are not pointer writes but should be audited separately.
- Additional pointer-to-`int` alignment casts remain in math/SIMD and renderer code.

### Verification Log For This Slice

- `MSBuild.exe neo\doom.sln /p:Configuration=Debug /p:Platform=x64 /clp:ErrorsOnly`: passed.
- `git diff --check`: passed.
- `rg -n "\(int\)ch->|\(int&\)ch->|\(\s*int\s*\)\s*(mix|inputSamples|realAccum)|mix_p" neo/sound/snd_world.cpp neo/sound/snd_system.cpp neo/sound/snd_local.h`: no matches.

## 2026-07-13 idlib Dynamic Math SetData Alignment Slice

Files changed in this slice:

- `neo/idlib/math/Vector.h`
- `neo/idlib/math/Matrix.h`

Corrections made:

- Replaced `idVecX::SetData` and `idMatX::SetData` alignment asserts that cast caller-owned float buffers through `int`.
- Renamed the touched `SetData` pointer parameters from `data` to `p_data`.
- Kept the existing external-buffer ownership semantics: `alloced == -1` still means the vector/matrix points at caller-owned data.

### Additional ABI / Layout Notes

- `idVecX` and `idMatX` object layouts are unchanged.
- Function signatures are type-identical except for parameter names, so C++ call ABI is unchanged.
- The assert now evaluates the full pointer width on x64 before applying the 16-byte alignment mask.

### Additional Audit Findings

| File + line | Problem | Correct 64-bit fix | ABI / binary impact |
| --- | --- | --- | --- |
| `neo/idlib/math/Vector.h:1468,1756-1761` | `idVecX::SetData` accepted `float *data` and asserted 16-byte alignment by truncating the pointer through `int`. | Renamed the parameter to `p_data` and used `reinterpret_cast<uintptr_t>( p_data ) & 15`. | No layout or persisted format impact; debug alignment check now works on x64 addresses. |
| `neo/idlib/math/Matrix.h:1823,2278-2284` | `idMatX::SetData` used the same 32-bit alignment assert and non-`p_` pointer parameter. | Renamed the parameter to `p_data` and used `reinterpret_cast<uintptr_t>( p_data ) & 15`. | No layout or persisted format impact; debug alignment check now works on x64 addresses. |

### Pointer Renaming Summary For This Slice

- `idVecX::SetData(..., float *data)` -> `idVecX::SetData(..., float *p_data)`.
- `idMatX::SetData(..., float *data)` -> `idMatX::SetData(..., float *p_data)`.

Known remaining:

- `idVecX` still has the legacy pointer field name `p`; fully satisfying the pointer-prefix rule will require a broader class-field rename across vector/matrix call sites.
- `idMatX` still has the legacy pointer field name `mat`; fully satisfying the pointer-prefix rule will require a broader class-field rename across matrix call sites.
- SIMD alignment code still contains pointer-through-`int` casts in `Simd_SSE2.cpp`, `Simd_SSE.cpp`, and `Simd_MMX.cpp`.

### Verification Log For This Slice

- `MSBuild.exe neo\idlib.vcxproj /p:Configuration=Debug /p:Platform=x64 /clp:ErrorsOnly`: passed.
- `MSBuild.exe neo\doom.sln /p:Configuration=Debug /p:Platform=x64 /clp:ErrorsOnly`: passed.
- `git diff --check`: passed.

## 2026-07-13 idlib BTree Pointer Rename Slice

Files changed in this slice:

- `neo/idlib/containers/BTree.h`

Corrections made:

- Renamed `idBTreeNode` pointer fields from `object`, `parent`, `next`, `prev`, `firstChild`, and `lastChild` to `p_object`, `p_parent`, `p_next`, `p_prev`, `p_firstChild`, and `p_lastChild`.
- Renamed `idBTree` root pointer field from `root` to `p_root`.
- Renamed touched B-tree pointer parameters and locals including `object`, `node`, `child`, `newNode`, `oldRoot`, `node1`, `node2`, and `lastNode` to `p_` forms.
- Updated all tree insert/remove/find/split/merge/check logic to use the prefixed fields.
- Preserved the widened `size_t` key instantiation used by `idDynamicBlockAlloc`.

### Struct Layout / ABI Notes

### `idBTreeNode<objType, keyType>`

Before this slice:

```c
objType *object;
idBTreeNode *parent;
idBTreeNode *next;
idBTreeNode *prev;
idBTreeNode *firstChild;
idBTreeNode *lastChild;
```

After this slice:

```c
objType *p_object;
idBTreeNode *p_parent;
idBTreeNode *p_next;
idBTreeNode *p_prev;
idBTreeNode *p_firstChild;
idBTreeNode *p_lastChild;
```

Layout impact: field names changed only; pointer order and native layout are unchanged.

Binary compatibility: this is a source/template ABI break for code that directly names these fields. Runtime layout is unchanged and no serialized format is affected.

### `idBTree<objType, keyType, maxChildrenPerNode>`

Before this slice:

```c
idBTreeNode<objType,keyType> *root;
```

After this slice:

```c
idBTreeNode<objType,keyType> *p_root;
```

Layout impact: field name changed only; pointer layout is unchanged.

Binary compatibility: private template implementation/source break only; all dependent x64 targets were rebuilt.

### Additional Audit Findings

| File + line | Problem | Correct 64-bit fix | ABI / binary impact |
| --- | --- | --- | --- |
| `neo/idlib/containers/BTree.h:42-53` | B-tree node pointer fields did not use the required `p_` prefix. | Renamed all node pointer fields to `p_` names. | Source/template field-name break; layout unchanged. |
| `neo/idlib/containers/BTree.h:65-86` | B-tree pointer parameters in declarations did not use `p_`. | Renamed declaration parameters to `p_object`, `p_node`, `p_node1`, and `p_node2`. | Function types unchanged; source names updated. |
| `neo/idlib/containers/BTree.h:78` | B-tree root pointer field did not use `p_`. | Renamed `root` to `p_root`. | Private field-name break; layout unchanged. |
| `neo/idlib/containers/BTree.h:114-518` | B-tree implementation locals and member accesses used non-prefixed pointer names. | Renamed locals and updated all member accesses through insert, remove, find, split, merge, and validation paths. | Runtime behavior preserved; source/template names changed. |

### Pointer Renaming Summary For This Slice

- `idBTreeNode::object` -> `p_object`.
- `idBTreeNode::parent` -> `p_parent`.
- `idBTreeNode::next` -> `p_next`.
- `idBTreeNode::prev` -> `p_prev`.
- `idBTreeNode::firstChild` -> `p_firstChild`.
- `idBTreeNode::lastChild` -> `p_lastChild`.
- `idBTree::root` -> `p_root`.
- B-tree locals/params: `object`, `node`, `child`, `newNode`, `oldRoot`, `node1`, `node2`, and `lastNode` -> `p_object`, `p_node`, `p_child`, `p_newNode`, `p_oldRoot`, `p_node1`, `p_node2`, and `p_lastNode`.

Known remaining:

- `idBlockAlloc` and `idDynamicBlockAlloc` are now substantially covered, but other container templates still need a full pointer-prefix scan.
- `idBTree` still uses `int numChildren` and `int numNodes` for counts; those are counts rather than pointer-sized values, but a future large-container pass may choose `size_t`.
- No serialization or file-format changes were involved in this slice.

### Verification Log For This Slice

- `MSBuild.exe neo\idlib.vcxproj /p:Configuration=Debug /p:Platform=x64 /clp:ErrorsOnly`: passed.
- `TypeInfo.exe`: passed and regenerated `neo/game/gamesys/GameTypeInfo.h`.
- `MSBuild.exe neo\doom.sln /p:Configuration=Debug /p:Platform=x64 /clp:ErrorsOnly`: passed.
- `git diff --check`: passed.
- `rg -n "objType\s*\*\s*object\b|idBTreeNode\s*\*\s*(parent|next|prev|firstChild|lastChild)\b|idBTreeNode<objType,keyType>\s*\*\s*(root|node|child|newNode|oldRoot|node1|node2|lastNode)\b|->(object|parent|next|prev|firstChild|lastChild)\b|\b(root|node|child|newNode|oldRoot|node1|node2|lastNode)\s*=" neo/idlib/containers/BTree.h`: no matches.

## 2026-07-13 idlib HashTable Pointer Rename Slice

Files changed in this slice:

- `neo/idlib/containers/HashTable.h`

Corrections made:

- Renamed the private hash bucket array field from `heads` to `p_heads`.
- Renamed the private linked-list field from `hashnode_s::next` to `hashnode_s::p_next`.
- Renamed pointer parameters in `Set`, `Get`, `Remove`, and `GetHash` to `p_key` / `p_value`.
- Renamed internal pointer locals in copy, lookup, insert, remove, clear, delete-contents, index, and spread paths to `p_` forms.
- Updated all member accesses through the sorted per-bucket linked lists to use the renamed pointer fields.

### Struct Layout / ABI Notes

### `idHashTable<Type>::hashnode_s`

Before this slice:

```c
idStr key;
Type value;
hashnode_s *next;
```

After this slice:

```c
idStr key;
Type value;
hashnode_s *p_next;
```

Layout impact: field name changed only; pointer order and native alignment are unchanged.

Binary compatibility: `hashnode_s` is a private nested template implementation type, so the change is a source/template field-name break only. Runtime layout is unchanged and no serialized format is affected.

### `idHashTable<Type>`

Before this slice:

```c
hashnode_s **heads;
```

After this slice:

```c
hashnode_s **p_heads;
```

Layout impact: field name changed only; pointer order and native alignment are unchanged.

Binary compatibility: private field-name source break only; all dependent x64 targets were rebuilt.

### Additional Audit Findings

| File + line | Problem | Correct 64-bit fix | ABI / binary impact |
| --- | --- | --- | --- |
| `neo/idlib/containers/HashTable.h:53-55` | Pointer parameters used non-prefixed names. | Renamed `key` to `p_key` and `value` to `p_value` where the parameter is a pointer. | Function types unchanged; source names updated. |
| `neo/idlib/containers/HashTable.h:71-77` | Hash node linkage and bucket array fields used non-prefixed pointer names. | Renamed `next` to `p_next` and `heads` to `p_heads`. | Private source/template field-name break; layout unchanged. |
| `neo/idlib/containers/HashTable.h:115-388` | Hash table implementation locals used non-prefixed pointer names. | Renamed traversal, insertion, deletion, and output pointer locals to `p_` names. | Runtime behavior preserved; source/template names changed. |

### Pointer Renaming Summary For This Slice

- `hashnode_s::next` -> `p_next`.
- `idHashTable::heads` -> `p_heads`.
- Parameters: `key` -> `p_key` in pointer-taking functions; `value` -> `p_value` for the `Type **` output parameter.
- Locals: `node`, `prev`, `nextPtr`, `head`, and `next` -> `p_node`, `p_prev`, `p_nextPtr`, `p_head`, and `p_next`.

Known remaining:

- `idHashTable` still uses `int` for table sizes, entry counts, and indexes; these are count/index values rather than pointer storage, but a future large-container pass may evaluate `size_t`.
- `Allocated()` and `Size()` still preserve the existing accounting expression shape; this slice only renamed pointer surfaces.
- No serialization, networking, or file-format changes were involved in this slice.

### Verification Log For This Slice

- `C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\amd64\MSBuild.exe neo\idlib.vcxproj /p:Configuration=Debug /p:Platform=x64 /clp:ErrorsOnly`: passed.
- `.\build\x64\Debug\TypeInfo.exe`: passed and regenerated `neo/game/gamesys/GameTypeInfo.h`.
- `C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\amd64\MSBuild.exe neo\doom.sln /p:Configuration=Debug /p:Platform=x64 /clp:ErrorsOnly`: passed.
- `git diff --check`: passed.
- `rg -n "const char \*key\b|Type \*\*value\b|hashnode_s \*next\b|hashnode_s \*\*\s*heads\b|hashnode_s\s+\*node\b|hashnode_s\s+\*\*prev\b|hashnode_s\s+\*\*nextPtr\b|hashnode_s\s+\*\*head\b|hashnode_s\s+\*prev\b|hashnode_s\s+\*next\b|->next\b|\bheads\b|\bnode\b|\bnextPtr\b" neo/idlib/containers/HashTable.h`: no matches.

## 2026-07-13 idlib LinkList Pointer Rename Slice

Files changed in this slice:

- `neo/idlib/containers/LinkList.h`

Corrections made:

- Renamed the private circular-list pointer fields from `head`, `next`, `prev`, and `owner` to `p_head`, `p_next`, `p_prev`, and `p_owner`.
- Renamed the `SetOwner` pointer parameter from `object` to `p_object`.
- Renamed the `Num()` traversal local from `node` to `p_node`.
- Updated all circular-list insertion, removal, traversal, and owner-return paths to use the prefixed fields.

### Struct Layout / ABI Notes

### `idLinkList<type>`

Before this slice:

```c
idLinkList *head;
idLinkList *next;
idLinkList *prev;
type *owner;
```

After this slice:

```c
idLinkList *p_head;
idLinkList *p_next;
idLinkList *p_prev;
type *p_owner;
```

Layout impact: field names changed only; pointer order and native alignment are unchanged.

Binary compatibility: private template field-name source break only. Runtime layout is unchanged and no serialized format is affected.

### Additional Audit Findings

| File + line | Problem | Correct 64-bit fix | ABI / binary impact |
| --- | --- | --- | --- |
| `neo/idlib/containers/LinkList.h:64` | Pointer parameter used non-prefixed name `object`. | Renamed to `p_object`. | Function type unchanged; source name updated. |
| `neo/idlib/containers/LinkList.h:71-74` | Private list and owner pointer fields did not use the required `p_` prefix. | Renamed to `p_head`, `p_next`, `p_prev`, and `p_owner`. | Private source/template field-name break; layout unchanged. |
| `neo/idlib/containers/LinkList.h:138-340` | Traversal and member accesses used non-prefixed pointer names. | Renamed the pointer local to `p_node` and updated all field references. | Runtime behavior preserved; source/template names changed. |

### Pointer Renaming Summary For This Slice

- `idLinkList::head` -> `p_head`.
- `idLinkList::next` -> `p_next`.
- `idLinkList::prev` -> `p_prev`.
- `idLinkList::owner` -> `p_owner`.
- `SetOwner(type *object)` -> `SetOwner(type *p_object)`.
- Local `node` -> `p_node`.

Known remaining:

- `idLinkList` still returns raw pointers through public APIs; the method return types are unchanged because the rule applies to variable/field/parameter names, not public method names.
- No serialization, networking, or file-format changes were involved in this slice.
- Other idlib containers, notably `Hierarchy.h`, `List.h`, `HashIndex.*`, `StrPool.h`, and `StaticList.h`, still need pointer-prefix passes.

### Verification Log For This Slice

- `C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\amd64\MSBuild.exe neo\idlib.vcxproj /p:Configuration=Debug /p:Platform=x64 /clp:ErrorsOnly`: passed.
- `.\build\x64\Debug\TypeInfo.exe`: passed and regenerated `neo/game/gamesys/GameTypeInfo.h`.
- `C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\amd64\MSBuild.exe neo\doom.sln /p:Configuration=Debug /p:Platform=x64 /clp:ErrorsOnly`: passed.
- `git diff --check`: passed.
- `rg -n "type \*\s*object\b|idLinkList \*\s*(head|next|prev)\b|type \*\s*owner\b|idLinkList<type>\s+\*node\b|->(head|next|prev|owner)\b|\.(head|next|prev|owner)\b|\b(head|next|prev|owner|node)\s*=" neo/idlib/containers/LinkList.h`: no matches.

## 2026-07-13 idlib Hierarchy Pointer Rename Slice

Files changed in this slice:

- `neo/idlib/containers/Hierarchy.h`

Corrections made:

- Renamed private hierarchy link fields from `parent`, `sibling`, and `child` to `p_parent`, `p_sibling`, and `p_child`.
- Renamed the private owner pointer field from `owner` to `p_owner`.
- Renamed the `SetOwner` pointer parameter from `object` to `p_object`.
- Renamed traversal locals `prev`, `parentNode`, `node`, and `prior` to `p_prev`, `p_parentNode`, `p_node`, and `p_prior`.
- Updated parent/child/sibling traversal, reparenting, removal, and owner-return paths to use the prefixed pointer fields.

### Struct Layout / ABI Notes

### `idHierarchy<type>`

Before this slice:

```c
idHierarchy *parent;
idHierarchy *sibling;
idHierarchy *child;
type *owner;
```

After this slice:

```c
idHierarchy *p_parent;
idHierarchy *p_sibling;
idHierarchy *p_child;
type *p_owner;
```

Layout impact: field names changed only; pointer order and native alignment are unchanged.

Binary compatibility: private template field-name source break only. Runtime layout is unchanged and no serialized format is affected.

### Additional Audit Findings

| File + line | Problem | Correct 64-bit fix | ABI / binary impact |
| --- | --- | --- | --- |
| `neo/idlib/containers/Hierarchy.h:47` | Pointer parameter used non-prefixed name `object`. | Renamed to `p_object`. | Function type unchanged; source name updated. |
| `neo/idlib/containers/Hierarchy.h:63-66` | Private parent/sibling/child/owner pointer fields did not use the required `p_` prefix. | Renamed to `p_parent`, `p_sibling`, `p_child`, and `p_owner`. | Private source/template field-name break; layout unchanged. |
| `neo/idlib/containers/Hierarchy.h:171-337` | Traversal and reparenting locals used non-prefixed pointer names. | Renamed locals to `p_prev`, `p_parentNode`, `p_node`, and `p_prior`; updated all field references. | Runtime behavior preserved; source/template names changed. |

### Pointer Renaming Summary For This Slice

- `idHierarchy::parent` -> `p_parent`.
- `idHierarchy::sibling` -> `p_sibling`.
- `idHierarchy::child` -> `p_child`.
- `idHierarchy::owner` -> `p_owner`.
- `SetOwner(type *object)` -> `SetOwner(type *p_object)`.
- Locals `prev`, `parentNode`, `node`, and `prior` -> `p_prev`, `p_parentNode`, `p_node`, and `p_prior`.

Known remaining:

- `idHierarchy` still returns raw pointers through public APIs; method names and return types were kept stable.
- No serialization, networking, or file-format changes were involved in this slice.
- Other idlib containers, notably `List.h`, `HashIndex.*`, `StrPool.h`, and `StaticList.h`, still need pointer-prefix passes.

### Verification Log For This Slice

- `C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\amd64\MSBuild.exe neo\idlib.vcxproj /p:Configuration=Debug /p:Platform=x64 /clp:ErrorsOnly`: passed.
- `.\build\x64\Debug\TypeInfo.exe`: passed and regenerated `neo/game/gamesys/GameTypeInfo.h`.
- First full-solution build attempt timed out in the tool after 244 seconds; no compile errors were returned.
- `C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\amd64\MSBuild.exe neo\doom.sln /p:Configuration=Debug /p:Platform=x64 /clp:ErrorsOnly`: passed on a fresh rerun.
- `git diff --check`: passed.
- `rg -n "type \*\s*object\b|idHierarchy \*\s*(parent|sibling|child)\b|type \*\s*owner\b|idHierarchy<type>\s+\*(prev|parentNode|node|prior)\b|const idHierarchy<type> \*node\b|->(parent|sibling|child|owner)\b|\.(parent|sibling|child|owner)\b|\b(parent|sibling|child|owner|prev|parentNode|node|prior)\s*=" neo/idlib/containers/Hierarchy.h`: no matches.

## 2026-07-13 idlib StrPool Pointer Rename Slice

Files changed in this slice:

- `neo/idlib/containers/StrPool.h`

Corrections made:

- Renamed `idPoolStr`'s back-pointer field from `pool` to `p_pool`.
- Renamed `AllocString` pointer parameter from `string` to `p_string`.
- Renamed `FreeString` and `CopyString` pointer parameters from `poolStr` to `p_poolStr`.
- Renamed the `AllocString` return local from `poolStr` to `p_poolStr`.
- Updated pool ownership asserts, allocation, copy, and free paths to use the prefixed pointer names.

### Struct Layout / ABI Notes

### `idPoolStr`

Before this slice:

```c
idStrPool *pool;
mutable int numUsers;
```

After this slice:

```c
idStrPool *p_pool;
mutable int numUsers;
```

Layout impact: field name changed only; pointer order and native alignment are unchanged.

Binary compatibility: private field-name source break only. Runtime layout is unchanged and no serialized format is affected.

### Additional Audit Findings

| File + line | Problem | Correct 64-bit fix | ABI / binary impact |
| --- | --- | --- | --- |
| `neo/idlib/containers/StrPool.h:57` | `idPoolStr` stored a pool back-pointer in a non-prefixed pointer field. | Renamed `pool` to `p_pool`. | Private field-name source break; layout unchanged. |
| `neo/idlib/containers/StrPool.h:73-75` | Pointer parameters used non-prefixed names. | Renamed `string` to `p_string` and `poolStr` to `p_poolStr`. | Function types unchanged; source names updated. |
| `neo/idlib/containers/StrPool.h:98-177` | Allocation/free/copy implementation used non-prefixed pointer locals and accesses. | Renamed `poolStr` local and all back-pointer accesses to `p_` names. | Runtime behavior preserved; source names changed. |

### Pointer Renaming Summary For This Slice

- `idPoolStr::pool` -> `p_pool`.
- `AllocString(const char *string)` -> `AllocString(const char *p_string)`.
- `FreeString(const idPoolStr *poolStr)` -> `FreeString(const idPoolStr *p_poolStr)`.
- `CopyString(const idPoolStr *poolStr)` -> `CopyString(const idPoolStr *p_poolStr)`.
- Local `poolStr` -> `p_poolStr`.

Known remaining:

- `idStrPool::pool` is an `idList<idPoolStr *>` container object, not a pointer variable, and was intentionally left unchanged in this slice.
- `idList` itself still needs a broader pointer-prefix pass for its pointer storage and parameters.
- No serialization, networking, or file-format changes were involved in this slice.

### Verification Log For This Slice

- `C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\amd64\MSBuild.exe neo\idlib.vcxproj /p:Configuration=Debug /p:Platform=x64 /clp:ErrorsOnly`: passed.
- `.\build\x64\Debug\TypeInfo.exe`: passed and regenerated `neo/game/gamesys/GameTypeInfo.h`.
- `C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\amd64\MSBuild.exe neo\doom.sln /p:Configuration=Debug /p:Platform=x64 /clp:ErrorsOnly`: passed.
- `git diff --check`: passed.
- `rg -n "const char \*string\b|const idPoolStr \*poolStr\b|idPoolStr \*poolStr\b|idStrPool \*\s*pool\b|->pool\b|\.pool\b|\bpoolStr\b" neo/idlib/containers/StrPool.h`: no matches.

## 2026-07-15 idlib StaticList Pointer Parameter Rename Slice

### Files Changed

- `neo/idlib/containers/StaticList.h`
- `Documentation/POINTER_64BIT_MIGRATION_REPORT.md`

### Classification And Compatibility Story

| Surface | Category | Resolution |
| --- | --- | --- |
| `idStaticList::IndexOf( const type *objptr )` | Legacy API interop / idlib container pointer parameter | Renamed the actual pointer parameter to `p_obj` in the declaration and inline definition. |

This slice does not widen savegame, network, demo, journal, renderer handle, model, or asset formats. The function type is unchanged because only the template parameter name changed; `IndexOf` still computes an integer index from a pointer into the internal fixed array. The internal `num` count and `list[size]` storage remain unchanged.

No binary-format compile-time assertion was added because `idStaticList` is a header-only in-memory container template and this slice does not introduce or modify any persisted structure size.

Known boundary: this slice only covers the actual pointer parameter in `idStaticList::IndexOf`. `idList` still has broader pointer storage, pointer parameters, and pointer locals that need a dedicated container slice.

### Verification Log For This Slice

- `rg -n "IndexOf\( const type \*p_obj \)|IndexOf\( type const \*p_obj \)|\bobjptr\b|p_obj - list" neo\idlib\containers\StaticList.h`: `p_obj` declaration/definition and pointer-difference use are present; stale `objptr` is gone.
- `cmake --build --preset ninja-gcc-release -j 8`: initial invocation exceeded the tool timeout while the header rebuild was still running; after the background build exited, rerun passed with no work remaining.
- `cmake --build --preset ninja-dedicated-release -j 8`: initial invocation exceeded the tool timeout while the header rebuild was still running; after the background build exited, rerun passed with no work remaining.
- `Doom3.exe +set fs_basepath F:\IdTech4-Remaster +set com_skipRenderer 1 +set s_noSound 1 +quit`: exit code 0.
- `DedServer.exe +set fs_basepath F:\IdTech4-Remaster +quit`: exit code 0.
- `rg --files | rg "(?i)\.(save|sav|savegame)$"`: no checked-in save corpus was found for a save/load compatibility smoke.
- `git diff --check`: passed.

## 2026-07-15 idlib List Pointer Storage Rename Slice

### Files Changed

- `neo/idlib/containers/List.h`
- `Documentation/POINTER_64BIT_MIGRATION_REPORT.md`

### Classification And Compatibility Story

| Surface | Category | Resolution |
| --- | --- | --- |
| `idList::list` private backing-array pointer | Legacy API interop / idlib container pointer storage | Renamed the private pointer field to `p_list` and updated all internal uses. |
| `idList::Resize` temporary backing-array pointers | Legacy API interop / idlib container pointer locals | Renamed `temp` pointer locals to `p_temp` in both resize overloads. |
| `idList::AssureSizeAlloc` allocator callback pointer | Legacy API interop / idlib container pointer parameter | Renamed the callback pointer parameter to `p_allocator`. |
| `idList::IndexOf` element pointer | Legacy API interop / idlib container pointer parameter | Renamed `objptr` to `p_obj` and kept the pointer-difference-to-index behavior unchanged. |
| `idList::Sort` / `SortSubSection` comparator pointers | Legacy API interop / idlib container pointer parameters | Renamed comparator function pointers and the `qsort` thunk local to `p_` names. |
| `idListSortCompare` comparator operands | Legacy API interop / idlib container pointer parameters | Renamed comparator pointer operands to `p_a` and `p_b`. |

This slice does not widen savegame, network, demo, journal, renderer handle, model, or asset formats. `idList` remains a header-only in-memory template; the private backing pointer stays in the same field order with the same native pointer width, and count/index fields remain `int`.

No binary-format compile-time assertion was added because this slice only renames source identifiers in a non-serialized template container and does not change the intended size of any persisted record.

Known boundary: this slice does not evaluate whether `idList` count/capacity fields should become `size_t`; those fields are counts and indexes rather than pointer storage and need a separate large-container compatibility review before widening.

### Verification Log For This Slice

- `rg -n "\btype \*\s*list\b|\blist\s*=|\blist\[|\*list\b|\bother\.list\b|\bobjptr\b|\bnew_t \*allocator\b|\btype\s*\*temp\b|\btemp\s*=|\bdelete\[\]\s*temp\b|\bcmp_t \*compare\b|\bvCompare\b|idListSortCompare\( const type \*(a|b)" neo\idlib\containers\List.h`: no stale code identifiers remained.
- `rg -n "p_list|p_temp|p_allocator|p_obj|p_compare|p_compareThunk|p_a|p_b" neo\idlib\containers\List.h`: renamed pointer storage, parameters, locals, and comparator operands are present.
- `cmake --build --preset ninja-gcc-release -j 8`: initial invocation exceeded the tool timeout while the header rebuild was still running; after the background build exited, rerun passed with no work remaining.
- `cmake --build --preset ninja-dedicated-release -j 8`: initial invocation exceeded the tool timeout while the header rebuild was still running; after the background build exited, rerun passed with no work remaining.
- `Doom3.exe +set fs_basepath F:\IdTech4-Remaster +set com_skipRenderer 1 +set s_noSound 1 +quit`: exit code 0.
- `DedServer.exe +set fs_basepath F:\IdTech4-Remaster +quit`: exit code 0.
- `rg --files | rg "(?i)\.(save|sav|savegame)$"`: no checked-in save corpus was found for a save/load compatibility smoke.
- `git diff --check`: passed.

## 2026-07-13 idlib HashIndex Pointer Rename Slice

Files changed in this slice:

- `neo/idlib/containers/HashIndex.h`
- `neo/idlib/containers/HashIndex.cpp`

Corrections made:

- Renamed private pointer fields `hash` and `indexChain` to `p_hash` and `p_indexChain`.
- Renamed the string key parameter in `GenerateKey` from `string` to `p_string`.
- Renamed resize and spread temporary pointer locals `oldIndexChain` and `numHashItems` to `p_oldIndexChain` and `p_numHashItems`.
- Updated allocation, free, copy assignment, add/remove, insert/remove-index, clear, resize, and spread code to use the prefixed pointer names.

### Struct Layout / ABI Notes

### `idHashIndex`

Before this slice:

```c
int hashSize;
int *hash;
int indexSize;
int *indexChain;
```

After this slice:

```c
int hashSize;
int *p_hash;
int indexSize;
int *p_indexChain;
```

Layout impact: field names changed only; pointer order and native alignment are unchanged.

Binary compatibility: private field-name source break only. Runtime layout is unchanged and no serialized format is affected.

### Additional Audit Findings

| File + line | Problem | Correct 64-bit fix | ABI / binary impact |
| --- | --- | --- | --- |
| `neo/idlib/containers/HashIndex.h:85` | Pointer parameter used non-prefixed name `string`. | Renamed to `p_string`. | Function type unchanged; source name updated. |
| `neo/idlib/containers/HashIndex.h:93,95` | Private pointer fields did not use the required `p_` prefix. | Renamed `hash` to `p_hash` and `indexChain` to `p_indexChain`. | Private field-name source break; layout unchanged. |
| `neo/idlib/containers/HashIndex.cpp:93,125` | Temporary pointer locals used non-prefixed names. | Renamed to `p_oldIndexChain` and `p_numHashItems`. | Runtime behavior preserved; source names changed. |

### Pointer Renaming Summary For This Slice

- `idHashIndex::hash` -> `p_hash`.
- `idHashIndex::indexChain` -> `p_indexChain`.
- `GenerateKey(const char *string)` -> `GenerateKey(const char *p_string)`.
- Local `oldIndexChain` -> `p_oldIndexChain`.
- Local `numHashItems` -> `p_numHashItems`.

Known remaining:

- `idHashIndex` stores integer hash/index chains in `int` arrays; these are index values rather than pointer-sized address storage and were not widened in this slice.
- `INVALID_INDEX` remains a static sentinel array, not a renamed pointer variable.
- No serialization, networking, or file-format changes were involved in this slice.

### Verification Log For This Slice

- `C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\amd64\MSBuild.exe neo\idlib.vcxproj /p:Configuration=Debug /p:Platform=x64 /clp:ErrorsOnly`: passed.
- `.\build\x64\Debug\TypeInfo.exe`: passed and regenerated `neo/game/gamesys/GameTypeInfo.h`.
- `C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\amd64\MSBuild.exe neo\doom.sln /p:Configuration=Debug /p:Platform=x64 /clp:ErrorsOnly`: passed.
- `git diff --check`: passed.
- `rg -n "\bint \*\s*(hash|indexChain|oldIndexChain|numHashItems)\b|\bconst char \*string\b|\bother\.(hash|indexChain)\b|\b(hash|indexChain)\s*=|[^_]\b(hash|indexChain)\[|\boldIndexChain\b|\bnumHashItems\b" neo/idlib/containers/HashIndex.h neo/idlib/containers/HashIndex.cpp`: no matches.

## 2026-07-14 SSE Joint Layout Assertion and Anchor Classification Slice

Files changed in this slice:

- `neo/idlib/math/Simd_SSE.cpp`
- `neo/idlib/math/Simd_SSE3.cpp`
- `neo/framework/CVarSystem.h`
- `neo/framework/CVarSystem.cpp`
- `neo/game/Game_local.cpp`
- `neo/d3xp/Game_local.cpp`
- `neo/MayaImport/maya_main.cpp`
- `Documentation/POINTER_64BIT_MIGRATION_REPORT.md`

Corrections made:

- Replaced repeated `idDrawVert` and `idJointQuat` layout checks that formed null-member addresses and narrowed them through `int`.
- Added file-scope `static_assert` checks for `idDrawVert` size and offsets, `idJointQuat` size, `idJointMat` size, `idVec4` joint-weight size, and the `idJointQuat::q` to `idJointQuat::t` offset relationship used by the SSE joint conversion code.
- Replaced the `idCVar::staticVars` magic pointer sentinel `(idCVar *)0xFFFFFFFF` with explicit `idCVar::staticVarsRegistered` boolean state in each module that owns a private static CVar list.
- Renamed the touched `SetInternalVar` pointer parameter to `p_cvar` and the `RegisterStaticVars` traversal local to `p_cvar`.
- Kept savegame, network, and renderer handle field widths unchanged; this slice only removed local pointer-width layout idioms.

### Classification Against Requested Categories

| Location | Category | Current disposition |
| --- | --- | --- |
| `neo/idlib/math/Simd_SSE.cpp` before this slice | Legacy API interop | Fixed locally. The x86 SSE implementation still relies on fixed draw-vertex and joint binary layouts, but the guards are now `static_assert` plus `offsetof` instead of pointer-to-`int` arithmetic. |
| `neo/idlib/math/Simd_SSE3.cpp` before this slice | Legacy API interop | Fixed locally. The SSE3 transform path now uses the same `static_assert`/`offsetof` layout guards instead of narrowing `idDrawVert` member addresses to `int`. |
| `neo/framework/CVarSystem.h:292,301,305` before this slice | Pointer storage | Fixed locally. The static CVar registration path no longer stores the sentinel value `0xFFFFFFFF` in an `idCVar *`; it now uses `staticVarsRegistered` and resets the pointer list to `NULL` after registration. |
| `neo/game/gamesys/SaveGame.cpp:678,1449` and `neo/d3xp/gamesys/SaveGame.cpp:683,1459` | Savegame field | `contactInfo_t::modelFeature` remains a 32-bit saved field. Earlier collision work changed the runtime value to a stable polygon ID, so this field must not be widened without a versioned save compatibility story. |
| `neo/game/Game_network.cpp:420,1434,1439` | Network field / renderer handle | Portal states serialize `qhandle_t` portal numbers through fixed bit widths. These are explicit renderer handles/indices, not pointer storage, and should stay narrow unless a protocol version is added. |
| `neo/renderer/tr_local.h:151,164` | Renderer handle | Existing comments identify the legacy `qhandle_t` render-light/entity API. Future work should move callers to typed render interfaces or explicit indices, not pointer-valued integers. |
| `neo/renderer/Model.h:297-301` | Renderer handle | `jointHandle_t` is an explicit model joint handle. No pointer widening is appropriate; any future migration should preserve it as a typed handle/index. |
| `neo/idlib/containers/HashIndex.*` from the starting inventory | Pointer hashing | Already classified as integer hash/index storage, not pointer hashing. No address-derived hash key was changed in this slice. |
| `neo/framework/*` journal/event items from the starting inventory | Serialization | Still open. Native `sysEvent_t` journaling remains a serialization compatibility issue and needs a versioned fixed-width journal record before any field widening. |

### Struct Layout / ABI Notes

#### `idDrawVert`, `idJointQuat`, `idJointMat`, and `idVec4`

The SSE code uses hard-coded vertex and joint sizes:

```c
#define DRAWVERT_SIZE   60
#define JOINTQUAT_SIZE (7*4)
#define JOINTMAT_SIZE  (4*3*4)
#define JOINTWEIGHT_SIZE (4*4)
```

This slice keeps those binary layout constants unchanged and promotes the checks to compile time:

```c
static_assert( sizeof( idDrawVert ) == DRAWVERT_SIZE, ... );
static_assert( offsetof( idDrawVert, xyz ) == DRAWVERT_XYZ_OFFSET, ... );
static_assert( offsetof( idDrawVert, st ) == DRAWVERT_ST_OFFSET, ... );
static_assert( offsetof( idDrawVert, normal ) == DRAWVERT_NORMAL_OFFSET, ... );
static_assert( offsetof( idDrawVert, tangents ) == DRAWVERT_TANGENT0_OFFSET, ... );
static_assert( offsetof( idDrawVert, color ) == DRAWVERT_COLOR_OFFSET, ... );
static_assert( sizeof( idJointQuat ) == JOINTQUAT_SIZE, ... );
static_assert( sizeof( idJointMat ) == JOINTMAT_SIZE, ... );
static_assert( sizeof( idVec4 ) == JOINTWEIGHT_SIZE, ... );
static_assert( offsetof( idJointQuat, t ) == offsetof( idJointQuat, q ) + sizeof( ( ( idJointQuat * )0 )->q ), ... );
```

Layout impact: none. This is a guard-strengthening change only.

Binary compatibility: no savegame, network, demo, or renderer handle width changed in this slice.

#### `idCVar` Static Registration State

Before this slice:

```c
static idCVar *staticVars;
...
if ( staticVars != (idCVar *)0xFFFFFFFF ) { ... }
...
staticVars = (idCVar *)0xFFFFFFFF;
```

After this slice:

```c
static idCVar *staticVars;
static bool staticVarsRegistered;
...
if ( !staticVarsRegistered ) { ... }
...
staticVars = NULL;
staticVarsRegistered = true;
```

Layout impact: `idCVar` gains one static boolean per module, but object layout is unchanged because this is static class state.

Binary compatibility: no file-format or network-format impact. The changed state is process-local CVar registration bookkeeping.

Known remaining:

- `sysEvent_t` journaling still serializes a native pointer-bearing structure and needs a versioned fixed-width format.
- `contactInfo_t::modelFeature` remains savegame/event serialized as `int`; this is acceptable only because it now carries a stable polygon ID rather than a pointer token.
- Renderer `qhandle_t` and `jointHandle_t` surfaces remain explicit handles/indices. They should be documented or wrapped with typed handles where API ownership changes, not widened as pointer storage.
- This slice did not perform a save/load smoke because it did not change save serialization and no existing save corpus is checked into this workspace.

### Verification Log For This Slice

- `rg -n "\(idCVar \*\)0xFFFFFFFF|staticVarsRegistered|staticVars|SetInternalVar" neo/framework/CVarSystem.h neo/framework/CVarSystem.cpp neo/game/Game_local.cpp neo/d3xp/Game_local.cpp neo/MayaImport/maya_main.cpp`: stale magic pointer sentinel is gone; the explicit `staticVarsRegistered` state is present.
- `rg -n "\(int\)&\(\(id(DrawVert|JointQuat) \*\)0\)->|static_assert\( sizeof\( idDrawVert|static_assert\( sizeof\( idJointQuat|offsetof\( idDrawVert|offsetof\( idJointQuat" neo/idlib/math/Simd_SSE.cpp neo/idlib/math/Simd_SSE3.cpp`: stale pointer-to-`int` member-offset patterns are gone; the new static assertions are present.
- `cmake --build --preset ninja-gcc-release -j 8`: passed; this rebuilt the touched CVar header users and emitted existing legacy warning noise, but no errors.
- `cmake --build --preset ninja-dedicated-release -j 8`: passed; this rebuilt the touched CVar header users and emitted existing legacy warning noise, but no errors.
- `rg -n "\(\s*idCVar\s*\*\s*\)\s*0xFFFFFFFF|\(int\)&\(\(id(DrawVert|JointQuat) \*\)0\)->" neo/framework neo/game neo/d3xp neo/MayaImport neo/idlib`: no matches.
- `rg --files | rg "(?i)\.(save|sav|savegame)$"`: no checked-in save corpus was found for a save/load compatibility smoke.
- `Doom3.exe +set fs_basepath F:\IdTech4-Remaster +set com_skipRenderer 1 +set s_noSound 1 +quit`: exit code 0.
- `DedServer.exe +set fs_basepath F:\IdTech4-Remaster +quit`: exit code 0.

## 2026-07-14 Fixed-Width Journal Event Serialization Slice

### Files Changed

- `neo/framework/EventLoop.cpp`
- `neo/framework/EventLoop.h`
- `Documentation/POINTER_64BIT_MIGRATION_REPORT.md`

### Classification And Compatibility Story

| Surface | Category | Resolution |
| --- | --- | --- |
| `neo/framework/EventLoop.cpp` native `sysEvent_t` journal reads/writes | Serialization | Replaced native `Read( &ev, sizeof( ev ) )` / `Write( &ev, sizeof( ev ) )` event serialization with explicit fixed-width integer fields. |
| `sysEvent_t::p_evPtr` in `journal.dat` | Pointer storage inside serialized data | New journal records no longer write the native pointer value. Event payload bytes remain serialized after the fixed record when `evPtrLength > 0`. |
| Headerless legacy journal files | Serialization compatibility | Playback falls back to a legacy 32-bit reader that reads five 32-bit fields and ignores the old serialized pointer token. |
| New `journal.dat` files | Serialization format | New recordings start with a `JRN2` magic and version `1`, then write a 16-byte event record plus optional payload bytes. |

The fixed journal event record is:

```c
typedef struct journalEventRecord_s {
	int evType;
	int evValue;
	int evValue2;
	int evPtrLength;
} journalEventRecord_t;
```

Compile-time guards now require `sizeof( int ) == 4` and `sizeof( journalEventRecord_t ) == 16`, keeping the on-disk event header independent from native pointer width and structure padding. `evPtrLength` remains a 32-bit `int`; no savegame or network field was widened in this slice.

Payload bytes continue to live in `journal.dat` immediately after the event record, matching the old event payload behavior. `journaldata.dat` remains reserved for the existing file-system/config journaling path and was not repurposed.

Compatibility boundary: this slice supports new versioned journals and old headerless 32-bit native journals. Headerless 64-bit native journals are not auto-detected because they have no reliable format marker; they should be regenerated with the new versioned writer.

### Verification Log For This Slice

- `rg -n "sizeof\s*\(\s*ev\s*\)|Write\s*\(\s*&ev|Read\s*\(\s*&ev|reinterpret_cast<int &>\( ev\.evType \)" neo/framework/EventLoop.cpp`: no matches.
- `cmake --build --preset ninja-gcc-release -j 8`: passed; existing legacy warnings remain, but no errors.
- `cmake --build --preset ninja-dedicated-release -j 8`: passed; existing legacy warnings remain, but no errors.
- `Doom3.exe +set fs_basepath F:\IdTech4-Remaster +set com_skipRenderer 1 +set s_noSound 1 +quit`: exit code 0.
- `DedServer.exe +set fs_basepath F:\IdTech4-Remaster +quit`: exit code 0.
- `rg --files | rg "(?i)\.(save|sav|savegame)$"`: no checked-in save corpus was found for a save/load compatibility smoke.
- `git diff --check`: passed.

## 2026-07-15 Token Special Float Bit Alias Cleanup Slice

### Files Changed

- `neo/idlib/Token.cpp`
- `Documentation/POINTER_64BIT_MIGRATION_REPORT.md`

### Classification And Compatibility Story

| Surface | Category | Resolution |
| --- | --- | --- |
| `idToken::NumberValue` special float parsing for `1.#INF`, `1.#IND`, and `1.#QNAN` | Legacy API interop / token parser float bit lane | Replaced `float *` aliasing of 32-bit integer literals with a fixed-width `memcpy` helper that materializes the same IEEE float bit patterns before widening to the existing `double` token value. |

This slice does not widen savegame, network, demo, journal, renderer handle, parser token, or asset formats. It only changes how the parser constructs transient special floating-point values for legacy token spellings. The 32-bit patterns remain `0x7f800000`, `0xffc00000`, and `0x7fc00000`.

Compile-time guards:

```c
static_assert( sizeof( dword ) == 4, "token special-float bit lanes must stay 32-bit" );
static_assert( sizeof( float ) == 4, "token special-float values require 32-bit IEEE float storage" );
```

### Verification Log For This Slice

- `rg -n "\*\s*\(\s*float\s*\*\s*\)\s*&|Token_FloatFromBits|token special-float|0x7f800000|0xffc00000|0x7fc00000" neo\idlib\Token.cpp`: stale float-pointer aliases are gone; fixed-width helper, guards, and exact legacy bit patterns are present.
- `cmake --build --preset ninja-gcc-release -j 8`: passed; rebuilt `neo/idlib/Token.cpp`, regenerated TypeInfo outputs, linked `Doom3.exe`, and emitted existing legacy warning noise but no errors.
- `cmake --build --preset ninja-dedicated-release -j 8`: passed; rebuilt `neo/idlib/Token.cpp`, regenerated TypeInfo outputs, and emitted existing legacy warning noise but no errors.
- `Doom3.exe +set fs_basepath F:\IdTech4-Remaster +set com_skipRenderer 1 +set s_noSound 1 +quit`: exit code 0.
- `DedServer.exe +set fs_basepath F:\IdTech4-Remaster +quit`: exit code 0.
- `rg --files | rg "(?i)\.(save|sav|savegame)$"`: no checked-in save corpus was found for a save/load compatibility smoke.
- `git diff --check`: passed.

## 2026-07-15 SSE2 Fixed-Width Mask Lane Cleanup Slice

### Files Changed

- `neo/idlib/math/Simd_SSE2.cpp`
- `Documentation/POINTER_64BIT_MIGRATION_REPORT.md`

### Classification And Compatibility Story

| Surface | Category | Resolution |
| --- | --- | --- |
| SSE2 128-bit mask constants | Legacy API interop / SIMD fixed-width mask lane | Replaced `unsigned long[4]` sign, absolute-value, and infinity masks with explicit `dword[4]` lanes so each aligned SIMD mask remains exactly 4 x 32-bit values on LP64 and LLP64 hosts. |

This slice does not widen savegame, network, demo, journal, renderer handle, model, or asset formats. The edited values are process-local SSE2 mask constants consumed by legacy SIMD code. The aligned 16-byte storage shape is preserved, but the lane type no longer depends on host `unsigned long` width.

Compile-time guard:

```c
static_assert( sizeof( dword ) == 4, "SSE2 bit-mask lanes must stay 32-bit" );
```

Known boundary: this slice only covers `neo/idlib/math/Simd_SSE2.cpp`; other legacy SIMD files still need their own fixed-width lane audits.

### Verification Log For This Slice

- `rg -n "unsigned long|SSE2 bit-mask|SIMD_SP_(singleSignBitMask|signBitMask|absMask|infinityMask)" neo\idlib\math\Simd_SSE2.cpp`: no `unsigned long` remains in the file; fixed-width masks and the guard are present.
- `MSBuild.exe neo\idlib.vcxproj /p:Configuration=Release /p:Platform=Win32 /clp:ErrorsOnly`: passed; this is the direct compiler coverage for `neo/idlib/math/Simd_SSE2.cpp`, which is intentionally excluded from x64 Visual Studio configs.
- `cmake --build --preset ninja-gcc-release -j 8`: passed; no work was needed, and the current runnable x64 client output remained current.
- `cmake --build --preset ninja-dedicated-release -j 8`: passed; no work was needed, and the current runnable x64 dedicated output remained current.
- `Doom3.exe +set fs_basepath F:\IdTech4-Remaster +set com_skipRenderer 1 +set s_noSound 1 +quit`: exit code 0.
- `DedServer.exe +set fs_basepath F:\IdTech4-Remaster +quit`: exit code 0.
- `rg --files | rg "(?i)\.(save|sav|savegame)$"`: no checked-in save corpus was found for a save/load compatibility smoke.
- `git diff --check`: passed.

## 2026-07-15 SSE Float Sign Bit And Mask Lane Cleanup Slice

### Files Changed

- `neo/idlib/math/Simd_SSE.cpp`
- `Documentation/POINTER_64BIT_MIGRATION_REPORT.md`

### Classification And Compatibility Story

| Surface | Category | Resolution |
| --- | --- | --- |
| SSE 128-bit mask constants | Legacy API interop / SIMD fixed-width mask lane | Replaced `unsigned long[4]` SIMD mask constants with `dword[4]` lanes so sign, absolute-value, infinity, shuffle, and bitwise-not masks remain explicitly 4 x 32-bit values on LP64 and LLP64 hosts. |
| `SSE_ATan` scalar fallback sign transfer | Legacy API interop / SIMD math float bit lane | Replaced `unsigned long *` float aliasing with fixed-width `memcpy` helpers that copy the sign bit from the input ratio into the half-pi fallback result. |
| `idSIMD_SSE::BlendJoints` quaternion sign handling | Legacy API interop / SIMD math float bit lane | Replaced `unsigned long *` float aliasing and scalar `unsigned long` sign storage with `dword` sign lanes plus fixed-width float-bit helpers. |
| `idSIMD_SSE::DeriveTangents` scalar fallback sign handling | Legacy API interop / SIMD math float bit lane | Replaced `unsigned long *` float aliasing in the non-asm tangent fallback paths with `dword` sign lanes plus fixed-width float-bit helpers. |

This slice does not widen savegame, network, demo, journal, renderer handle, model, or asset formats. The edited values are process-local SSE math masks and transient float sign-bit lanes. Existing inline-assembly paths still consume aligned 16-byte mask arrays; those arrays now use four explicit 32-bit `dword` lanes instead of host-width `unsigned long` lanes.

Compile-time guards:

```c
static_assert( sizeof( dword ) == 4, "SSE bit-mask lanes must stay 32-bit" );
static_assert( sizeof( float ) == 4, "SSE float bit helpers require 32-bit IEEE float storage" );
```

Related boundary: `neo/idlib/math/Simd_SSE2.cpp` declared related SIMD mask constants with `unsigned long`; that follow-up is tracked in the SSE2 fixed-width mask lane cleanup slice above.

### Verification Log For This Slice

- `rg -n "unsigned long|\*\s*\(\s*unsigned long\s*\*\s*\)\s*&|SimdSSE_|signBit" neo\idlib\math\Simd_SSE.cpp`: stale `unsigned long *` float aliases are gone; fixed-width helpers, `dword` masks, and sign-bit use are present.
- `cmake --build --preset ninja-gcc-release -j 8`: passed; the current runnable x64 client preset stayed green, with existing TypeInfo warning noise but no errors.
- `cmake --build --preset ninja-dedicated-release -j 8`: passed; no work was needed after the client preset, and the dedicated runnable output remained current.
- `MSBuild.exe neo\idlib.vcxproj /p:Configuration=Release /p:Platform=Win32 /clp:ErrorsOnly`: passed; this is the direct compiler coverage for `neo/idlib/math/Simd_SSE.cpp`, which is intentionally excluded from the x64 Visual Studio configs and not part of the current CMake Ninja target source list.
- `Doom3.exe +set fs_basepath F:\IdTech4-Remaster +set com_skipRenderer 1 +set s_noSound 1 +quit`: exit code 0.
- `DedServer.exe +set fs_basepath F:\IdTech4-Remaster +quit`: exit code 0.
- `rg --files | rg "(?i)\.(save|sav|savegame)$"`: no checked-in save corpus was found for a save/load compatibility smoke.
- `git diff --check`: passed.

## 2026-07-15 Generic SIMD Float Sign Bit Alias Cleanup Slice

### Files Changed

- `neo/idlib/math/Simd_Generic.cpp`
- `neo/idlib/math/Simd.cpp`
- `Documentation/POINTER_64BIT_MIGRATION_REPORT.md`

### Classification And Compatibility Story

| Surface | Category | Resolution |
| --- | --- | --- |
| `idSIMD_Generic::DeriveTangents` tangent sign-bit handling | Legacy API interop / SIMD math float bit lane | Replaced `unsigned long *` float aliasing with fixed-width `memcpy` helpers; the sign bit still XORs into the `idMath::RSqrt` result exactly as before. |
| `TestMath` `Fabs` benchmark bit path | Legacy API interop / SIMD test float bit lane | Replaced `int *` / `float *` aliasing with fixed-width `dword` bit copies, preserving the `0x7FFFFFFF` mask behavior. |

This slice does not widen savegame, network, demo, journal, renderer handle, or asset formats. Only in-memory SIMD math object-representation access changed. The legacy sign-bit behavior is now expressed through fixed-width 32-bit copies rather than pointer casts that depend on aliasing and integer-width assumptions.

Compile-time guards:

```c
static_assert( sizeof( dword ) == 4, "SIMD float bit helpers require a fixed 32-bit integer lane" );
static_assert( sizeof( float ) == 4, "SIMD float bit helpers require 32-bit IEEE float storage" );
static_assert( sizeof( dword ) == 4, "SIMD test float bit helpers require a fixed 32-bit integer lane" );
static_assert( sizeof( float ) == 4, "SIMD test float bit helpers require 32-bit IEEE float storage" );
```

Known boundary: broader `neo/idlib/math/Simd_SSE.cpp` float-bit aliasing remains and should be cleaned in a dedicated SSE slice.

### Verification Log For This Slice

- `rg -n "\*\s*\(\s*(unsigned long|int|float)\s*\*\s*\)\s*&|signBit|Simd(Generic|Test)_" neo\idlib\math\Simd_Generic.cpp neo\idlib\math\Simd.cpp`: stale alias expressions are gone; fixed-width helpers and `signBit` use are present.
- `cmake --build --preset ninja-gcc-release -j 8`: passed; rebuilt `neo/idlib/math/Simd.cpp`, `neo/idlib/math/Simd_Generic.cpp`, and linked `Doom3.exe`, with existing legacy warning noise but no errors.
- `cmake --build --preset ninja-dedicated-release -j 8`: passed when rerun sequentially; the first concurrent run hit a shared `libidLib.a` archive removal collision while the client build was also writing that archive.
- `Doom3.exe +set fs_basepath F:\IdTech4-Remaster +set com_skipRenderer 1 +set s_noSound 1 +quit`: exit code 0.
- `DedServer.exe +set fs_basepath F:\IdTech4-Remaster +quit`: exit code 0.
- `rg --files | rg "(?i)\.(save|sav|savegame)$"`: no checked-in save corpus was found for a save/load compatibility smoke.
- `git diff --check`: passed.

## 2026-07-15 AltiVec Alignment Macro Pointer-Width Slice

### Files Changed

- `neo/idlib/math/Simd_AltiVec.cpp`
- `Documentation/POINTER_64BIT_MIGRATION_REPORT.md`

### Classification And Compatibility Story

| Surface | Category | Resolution |
| --- | --- | --- |
| `IS_16BYTE_ALIGNED` and `NOT_16BYTE_ALIGNED` in the legacy AltiVec SIMD backend | Legacy API interop / runtime pointer arithmetic | Replaced address masking through `(unsigned long)&x` with `reinterpret_cast<uintptr_t>( &( x ) )`, preserving the 16-byte low-bit alignment test without assuming `unsigned long` is pointer-width. |

This slice does not widen savegame, network, demo, journal, renderer handle, or asset formats. The touched state is a compile-time macro used for process-local SIMD alignment decisions in the gated `MACOS_X && __ppc__` AltiVec implementation.

Known boundary: the Windows CMake presets do not compile the legacy PPC/Mac AltiVec implementation in this environment. The changed path was verified with source scans here and should still be compiled on a PPC/Mac-capable target if that build path is restored.

### Verification Log For This Slice

- `rg -n "\(unsigned long\)&|unsigned long\)&|reinterpret_cast\s*<\s*(int|unsigned int|long|unsigned long|dword)\s*>" neo\idlib\math\Simd_AltiVec.cpp`: no stale pointer-to-32-bit address casts remain in the touched file.
- `rg -n "IS_16BYTE_ALIGNED|NOT_16BYTE_ALIGNED|reinterpret_cast<uintptr_t>" neo\idlib\math\Simd_AltiVec.cpp`: the two alignment macros now use `uintptr_t`; call sites continue through the existing macros.
- `cmake --build --preset ninja-gcc-release -j 8`: passed; no Windows target work was required because the touched AltiVec path is gated out of this preset.
- `cmake --build --preset ninja-dedicated-release -j 8`: passed; no Windows target work was required because the touched AltiVec path is gated out of this preset.
- `Doom3.exe +set fs_basepath F:\IdTech4-Remaster +set com_skipRenderer 1 +set s_noSound 1 +quit`: exit code 0.
- `DedServer.exe +set fs_basepath F:\IdTech4-Remaster +quit`: exit code 0.
- `rg --files | rg "(?i)\.(save|sav|savegame)$"`: no checked-in save corpus was found for a save/load compatibility smoke.
- `git diff --check`: passed.

## 2026-07-15 Script VM And Callback Float Lane Alias Cleanup Slice

### Files Changed

- `neo/game/script/Script_Interpreter.cpp`
- `neo/d3xp/script/Script_Interpreter.cpp`
- `neo/game/gamesys/Class.cpp`
- `neo/d3xp/gamesys/Class.cpp`
- `neo/game/gamesys/Event.cpp`
- `neo/d3xp/gamesys/Event.cpp`
- `neo/game/gamesys/Callbacks.cpp`
- `neo/d3xp/gamesys/Callbacks.cpp`
- `Documentation/POINTER_64BIT_MIGRATION_REPORT.md`

### Classification And Compatibility Story

| Surface | Category | Resolution |
| --- | --- | --- |
| `idInterpreter::GetRegisterValue` object field debug reads in base game and d3xp | Legacy API interop / script VM value lane | Replaced `int *` and `float *` byte alias reads with `memcpy` helpers that decode the same 32-bit VM field lanes. |
| `idInterpreter::CallEvent` / `CallSysEvent` script-to-event scalar bridge | Legacy API interop / event argument bit field | Replaced writes through `float *` / `int *` into the transient `intptr_t p_data[]` callback array with explicit helpers and whole-lane integer assignment. |
| `OP_PUSH_BTOF` and `OP_PUSH_V` in base game and d3xp | Legacy API interop / script VM value lane | Replaced float-to-int bit aliasing with an explicit `memcpy` helper before pushing the 32-bit VM lane. |
| Generated `Callbacks.cpp` float argument reads and `CreateEventCallbackHandler` | Legacy API interop / generated callback float lane | Replaced generated `*( float * )&p_data[n]` callback reads with `idClass_FloatEventArg( p_data[n] )`, and updated the generator so future callback regeneration emits the helper path. |

This slice does not widen savegame fields, network fields, script VM stack lanes, or queued event buffers. Script VM int and float lanes remain four bytes, including the raw `localstack` bytes that are saved and restored by the interpreter. The transient event callback array remains `intptr_t`, but float callback arguments are copied into and decoded from the low 32-bit IEEE float lane explicitly; the helper zeroes the upper bytes before copying a float into an `intptr_t` slot.

Compile-time guards:

```c
static_assert( sizeof( int ) == 4, "script VM integer stack lanes must stay 32-bit" );
static_assert( sizeof( float ) == 4, "script VM float stack lanes must stay 32-bit" );
static_assert( sizeof( float ) == 4, "event callback float lanes must stay 32-bit" );
```

Known boundary: the broader VM `localstack` design still exposes typed pointer views through `varEval_t` and `idInterpreter::Push`; this slice removes the active float/int bit-pun expressions at the script/event callback boundary without changing VM layout or save format.

### Verification Log For This Slice

- `rg -n "\*reinterpret_cast<\s*(int|float)\s*\*>\s*\(\s*&obj->data|\*reinterpret_cast<\s*int\s*\*>\s*\(\s*&floatVal\s*\)|\*reinterpret_cast<\s*int\s*\*>\s*\(\s*&var_a\.vectorPtr|\*\(\s*(int|float)\s*\*\s*\)&p_data|\(\s*\*\(\s*float\s*\*\s*\)&p_data" neo\game\script\Script_Interpreter.cpp neo\d3xp\script\Script_Interpreter.cpp neo\game\gamesys\Callbacks.cpp neo\d3xp\gamesys\Callbacks.cpp neo\game\gamesys\Event.cpp neo\d3xp\gamesys\Event.cpp`: no matches.
- `rg -n "ScriptVM_(ReadIntLane|ReadFloatLane|FloatToIntLane|WriteFloatEventArg)|idClass_FloatEventArg|script VM (integer|float) stack lanes|event callback float lanes" neo\game\script\Script_Interpreter.cpp neo\d3xp\script\Script_Interpreter.cpp neo\game\gamesys\Class.cpp neo\d3xp\gamesys\Class.cpp neo\game\gamesys\Event.cpp neo\d3xp\gamesys\Event.cpp neo\game\gamesys\Callbacks.cpp neo\d3xp\gamesys\Callbacks.cpp`: helper definitions, use sites, generator output, and guards are present.
- `cmake --build --preset ninja-gcc-release -j 8`: passed; rebuilt the touched game and d3xp modules, regenerated TypeInfo, and emitted existing legacy warning noise but no errors.
- `cmake --build --preset ninja-dedicated-release -j 8`: passed; rebuilt the touched game and d3xp modules under the dedicated preset, with existing legacy warning noise but no errors.
- `Doom3.exe +set fs_basepath F:\IdTech4-Remaster +set com_skipRenderer 1 +set s_noSound 1 +quit`: exit code 0.
- `DedServer.exe +set fs_basepath F:\IdTech4-Remaster +quit`: exit code 0.
- `rg --files | rg "(?i)\.(save|sav|savegame)$"`: no checked-in save corpus was found for a save/load compatibility smoke.
- `git diff --check`: passed.

## 2026-07-14 DrawVert Color And InvSqrt Bit Alias Cleanup Slice

### Files Changed

- `neo/idlib/geometry/DrawVert.h`
- `neo/idlib/math/Math.h`
- `neo/idlib/math/Math.cpp`
- `Documentation/POINTER_64BIT_MIGRATION_REPORT.md`

### Classification And Compatibility Story

| Surface | Category | Resolution |
| --- | --- | --- |
| `idDrawVert::SetColor` / `idDrawVert::GetColor` packed color helpers | Renderer geometry helper / legacy API interop | Replaced `dword *` aliasing of the `byte color[4]` member with `memcpy`, preserving the exact 32-bit packed color payload. |
| `idMath::InvSqrt16`, `idMath::InvSqrt`, and `idMath::InvSqrt64` seed bit paths | Legacy API interop / math helper bit fields | Replaced `_flint` union pointer aliasing with the fixed-width `idMath_FloatBits` and `idMath_BitsToFloat` helpers. |
| `idMath::Init` inverse-square-root lookup table generation | Legacy API interop / math helper bit fields | Builds lookup-table input/output bit patterns through the same fixed-width helpers instead of `_flint` union aliasing. |

This slice does not widen any savegame or network field. `idDrawVert::color` remains four bytes, and `SetColor` / `GetColor` still copy exactly one `dword`. The inverse-square-root paths keep the same 32-bit IEEE seed-bit math and lookup table values; only the object-representation access mechanism changed.

Compile-time guards:

```c
static_assert( sizeof( dword ) == 4, "math float bit helpers require a fixed 32-bit integer lane" );
static_assert( sizeof( float ) == 4, "math float bit helpers require 32-bit IEEE float storage" );
static_assert( sizeof( this->color ) == sizeof( dword ), "idDrawVert color storage must stay 32-bit" );
```

### Verification Log For This Slice

- `rg -n "reinterpret_cast<\s*(const\s+)?dword\s*\*>\s*\(this->color\)|\(union _flint\*\)|union _flint|seed\.f|seed\.i" neo\idlib\geometry\DrawVert.h neo\idlib\math\Math.h neo\idlib\math\Math.cpp`: no matches.
- `rg -n "idMath_FloatBits|idMath_BitsToFloat|SetColor|GetColor|idDrawVert color storage" neo\idlib\geometry\DrawVert.h neo\idlib\math\Math.h neo\idlib\math\Math.cpp`: fixed-width helpers and color storage guards are present.
- `cmake --build --preset ninja-gcc-release -j 8`: passed; rebuilt the broad idLib/header dependency surface and linked runtime outputs, with existing legacy warning noise but no errors.
- `cmake --build --preset ninja-dedicated-release -j 8`: passed; rebuilt the broad idLib/header dependency surface and linked `DedServer.exe`, with existing legacy warning noise but no errors.
- `Doom3.exe +set fs_basepath F:\IdTech4-Remaster +set com_skipRenderer 1 +set s_noSound 1 +quit`: exit code 0.
- `DedServer.exe +set fs_basepath F:\IdTech4-Remaster +quit`: exit code 0.
- `rg --files | rg "(?i)\.(save|sav|savegame)$"`: no checked-in save corpus was found for a save/load compatibility smoke.
- `git diff --check`: passed.

## 2026-07-14 Math And Random Float Bit Alias Cleanup Slice

### Files Changed

- `neo/idlib/math/Math.h`
- `neo/idlib/math/Math.cpp`
- `neo/idlib/math/Random.h`
- `Documentation/POINTER_64BIT_MIGRATION_REPORT.md`

### Classification And Compatibility Story

| Surface | Category | Resolution |
| --- | --- | --- |
| `FLOATSIGNBIT*`, `FLOATNOTZERO`, and `FLOAT_IS_*` macros | Legacy API interop / math helper bit fields | Replaced direct `unsigned long *` float aliasing with a local `memcpy`-based 32-bit float bit helper. |
| `idMath::RSqrt`, `Exp16`, `Log16`, `ILog2`, `MaskForFloatSign`, and `Fabs` | Legacy API interop / math helper bit fields | Replaced active `reinterpret_cast<int *>` / `reinterpret_cast<long *>` float bit punning with the same fixed-width helper path. |
| `idMath::FloatToBits` / `BitsToFloat` | Network field helper | Kept the compressed-float bit layout unchanged while replacing strict-aliasing conversions with fixed-width bit copies. |
| `idRandom2::RandomFloat` / `CRandomFloat` | Runtime math helper | Replaced `*(float *)&i` with the fixed-width helper while preserving the existing generated mantissa bits. |

This slice does not widen savegame or network fields. It keeps the existing 32-bit IEEE float assumptions explicit and removes undefined strict-aliasing reads from the active math and random helper paths. The public `idRandom2` seed type is unchanged; only the local generated IEEE float bit lane now uses `dword`.

Compile-time guards:

```c
static_assert( sizeof( dword ) == 4, "math float bit helpers require a fixed 32-bit integer lane" );
static_assert( sizeof( float ) == 4, "math float bit helpers require 32-bit IEEE float storage" );
```

Known boundary: this does not address non-aliasing math warnings such as deprecated implicit copies, class `memcpy` warnings in vector/matrix storage, or unrelated renderer/game warnings.

### Verification Log For This Slice

- `rg -n "\*reinterpret_cast<\s*(int|long|float)\s*\*>\s*\(&|\*\(float \*\)&|\*\(float \*\)|\*\(const unsigned long \*\)" neo\idlib\math\Math.h neo\idlib\math\Math.cpp neo\idlib\math\Random.h`: no matches.
- `rg -n "FLOATSIGNBITSET\(|FLOAT_IS_NAN\(|idMath_FloatBits|idMath_BitsToFloat|Random2::RandomFloat|Random2::CRandomFloat" neo\idlib\math\Math.h neo\idlib\math\Math.cpp neo\idlib\math\Random.h`: fixed-width helper definitions and use sites are present.
- `cmake --build --preset ninja-gcc-release -j 8`: passed; rebuilt the broad idLib/header dependency surface, linked runtime outputs, and emitted existing non-aliasing legacy warning noise but no errors.
- `cmake --build --preset ninja-dedicated-release -j 8`: passed; rebuilt the broad idLib/header dependency surface for the dedicated preset and emitted existing non-aliasing legacy warning noise but no errors.
- `Doom3.exe +set fs_basepath F:\IdTech4-Remaster +set com_skipRenderer 1 +set s_noSound 1 +quit`: exit code 0.
- `DedServer.exe +set fs_basepath F:\IdTech4-Remaster +quit`: exit code 0.
- `rg --files | rg "(?i)\.(save|sav|savegame)$"`: no checked-in save corpus was found for a save/load compatibility smoke.
- `git diff --check`: passed.

## 2026-07-15 Math SinCos x87 Pointer-Write Cleanup Slice

### Files Changed

- `neo/idlib/math/Math.h`
- `Documentation/POINTER_64BIT_MIGRATION_REPORT.md`

### Classification And Compatibility Story

| Surface | Category | Resolution |
| --- | --- | --- |
| `idMath::SinCos` MSVC x86 `fsincos` path | Legacy API interop / math helper pointer-write assembly | Removed the inline x87 assembly that moved reference addresses into 32-bit registers and wrote through them; all builds now use the existing portable `sinf`/`cosf` path. |
| `idMath::SinCos64` MSVC x86 `fsincos` path | Legacy API interop / math helper pointer-write assembly | Removed the matching double-precision x87 pointer-write path; all builds now use the existing portable `sin`/`cos` path. |

This slice does not widen savegame, network, demo, journal, renderer handle, model, or asset formats. The public function signatures are unchanged, and the output references are still written as normal C++ references. The behavioral boundary is runtime math implementation only: legacy MSVC x86 builds no longer use the old x87 `fsincos` instruction path for these helpers.

No binary-format compile-time assertion was added because this slice removes architecture-specific inline assembly and does not define or modify any persisted record layout.

Known boundary: `FtoiFast` and `FtolFast` still contain architecture-gated conversion assembly/fallback code. Those conversions are separate numeric rounding helpers and were intentionally left out of this `SinCos` pointer-write slice.

### Verification Log For This Slice

- `rg -n "SinCos\(|SinCos64\(|fsincos|mov\s+ecx, c|mov\s+edx, s|qword ptr \[ecx\]|dword ptr \[ecx\]" neo\idlib\math\Math.h`: only the `SinCos` declarations/definitions remain; stale x87 pointer-write assembly is gone.
- `cmake --build --preset ninja-gcc-release -j 8`: initial invocation exceeded the tool timeout while the header rebuild was still running; after the background build exited, rerun passed with no work remaining.
- `cmake --build --preset ninja-dedicated-release -j 8`: initial invocation exceeded the tool timeout while the header rebuild was still running; after the background build exited, rerun passed with no work remaining.
- `Doom3.exe +set fs_basepath F:\IdTech4-Remaster +set com_skipRenderer 1 +set s_noSound 1 +quit`: exit code 0.
- `DedServer.exe +set fs_basepath F:\IdTech4-Remaster +quit`: exit code 0.
- `rg --files | rg "(?i)\.(save|sav|savegame)$"`: no checked-in save corpus was found for a save/load compatibility smoke.
- `git diff --check`: passed.

## 2026-07-15 Math FloatHash Fixed-Width Lane Cleanup Slice

### Files Changed

- `neo/idlib/math/Math.h`
- `Documentation/POINTER_64BIT_MIGRATION_REPORT.md`

### Classification And Compatibility Story

| Surface | Category | Resolution |
| --- | --- | --- |
| `idMath::FloatHash` float-array hashing | Pointer hashing / math helper fixed-width float lane | Replaced `reinterpret_cast<const int *>( array )` hashing with per-float `idMath_FloatBits` reads, preserving the old 32-bit XOR hash semantics without reading a `float *` through an `int *`. |

This slice does not widen savegame, network, demo, journal, renderer handle, model, or asset formats. `FloatHash` still returns an `int`, and each input float still contributes exactly its 32-bit IEEE object representation to the XOR hash. Only the in-memory object-representation access path changed.

Compile-time guards:

```c
static_assert( sizeof( dword ) == 4, "math float bit helpers require a fixed 32-bit integer lane" );
static_assert( sizeof( float ) == 4, "math float bit helpers require 32-bit IEEE float storage" );
static_assert( sizeof( int ) == 4, "math integer bit lanes must stay 32-bit" );
```

Known boundary: `FloatHash` is used by base game and d3xp clip-model hashes. This slice preserves the existing hash type and bit pattern; any future change to a wider or endian-stable hash would need an explicit asset/network compatibility review.

### Verification Log For This Slice

- `rg -n "FloatHash\(|reinterpret_cast<\s*const\s*int\s*\*>\s*\(\s*array\s*\)|math integer bit lanes|idMath_FloatBits\( array\[i\] \)" neo\idlib\math\Math.h`: stale `reinterpret_cast<const int *>` array hashing is gone; fixed-width helper use and guards are present.
- `cmake --build --preset ninja-gcc-release -j 8`: initial invocation exceeded the tool timeout while the header rebuild was still running; after the background build exited, rerun passed with no work remaining.
- `cmake --build --preset ninja-dedicated-release -j 8`: initial invocation exceeded the tool timeout while the header rebuild was still running; after the background build exited, rerun passed with no work remaining.
- `Doom3.exe +set fs_basepath F:\IdTech4-Remaster +set com_skipRenderer 1 +set s_noSound 1 +quit`: exit code 0.
- `DedServer.exe +set fs_basepath F:\IdTech4-Remaster +quit`: exit code 0.
- `rg --files | rg "(?i)\.(save|sav|savegame)$"`: no checked-in save corpus was found for a save/load compatibility smoke.
- `git diff --check`: passed.

## 2026-07-14 BitMsg Float Bit Serialization Alias Cleanup Slice

### Files Changed

- `neo/idlib/BitMsg.h`
- `Documentation/POINTER_64BIT_MIGRATION_REPORT.md`

### Classification And Compatibility Story

| Surface | Category | Resolution |
| --- | --- | --- |
| `idBitMsg::WriteFloat` / `ReadFloat` | Network field / serialization | Replaced strict-aliasing `reinterpret_cast<int *>` float bit punning with local `memcpy` helpers that preserve the same 32-bit IEEE payload. |
| `idBitMsg::WriteDeltaFloat` / `ReadDeltaFloat` | Network field / serialization | Replaced old/new float aliasing casts with the same fixed-width helper path before feeding the existing 32-bit delta codec. |
| `idBitMsgDelta` float and delta-float helpers | Network field / serialization | Applied the same conversion helpers to the delta message wrapper, keeping base and delta message float encoding identical. |

This slice does not widen any network message field. Full-precision float messages still write exactly 32 bits, and compressed float messages still use the existing `idMath::FloatToBits` / `BitsToFloat` path. The change only removes undefined aliasing behavior in the in-memory conversion between `float` and the 32-bit integer lane used by `WriteBits`, `ReadBits`, and `ReadDelta`.

Compile-time guards:

```c
static_assert( sizeof( int ) == 4, "bit-message float fields must stay 32-bit" );
static_assert( sizeof( float ) == 4, "bit-message float fields must stay 32-bit" );
```

Known boundary: this does not address the remaining strict-aliasing warnings in `neo/idlib/math/Math.h` and `neo/idlib/math/Random.h`; those are separate math helper slices and should preserve their existing bit-level semantics when converted.

### Verification Log For This Slice

- `rg -n "reinterpret_cast<\s*int \*>\(&[A-Za-z_][A-Za-z0-9_]*\)|\*reinterpret_cast<int \*>\(&" neo\idlib\BitMsg.h`: no matches.
- `rg -n "BitMsg_FloatToRawBits|BitMsg_RawBitsToFloat|static_assert\( sizeof\( (int|float) \) == 4" neo\idlib\BitMsg.h`: fixed-width guards and helper use sites are present.
- `cmake --build --preset ninja-gcc-release -j 8`: initial invocation exceeded the tool timeout while still running; after the background build exited, rerun passed. A final post-report run also passed, reran TypeInfo, linked `gamex64.dll`, `d3xp/gamex64.dll`, and `Doom3.exe`, and emitted existing TypeInfo warnings but no errors.
- `cmake --build --preset ninja-dedicated-release -j 8`: passed; rebuilt the broad idLib/header dependency surface and emitted existing legacy warning noise, but no errors. A final post-report run also passed with no work remaining.
- `Doom3.exe +set fs_basepath F:\IdTech4-Remaster +set com_skipRenderer 1 +set s_noSound 1 +quit`: exit code 0.
- `DedServer.exe +set fs_basepath F:\IdTech4-Remaster +quit`: exit code 0.
- `rg --files | rg "(?i)\.(save|sav|savegame)$"`: no checked-in save corpus was found for a save/load compatibility smoke.
- `git diff --check`: passed.

## 2026-07-14 Fixed-Width JournalData Config Length Slice

### Files Changed

- `neo/framework/FileSystem.cpp`
- `Documentation/POINTER_64BIT_MIGRATION_REPORT.md`

### Classification And Compatibility Story

| Surface | Category | Resolution |
| --- | --- | --- |
| `journaldata.dat` `.cfg` payload length in `idFileSystemLocal::ReadFile` | Serialization | Replaced raw native `Read( &len, sizeof( len ) )` with `ReadInt( len )`, matching the fixed 32-bit little-endian helper used elsewhere in file serialization. |
| `journaldata.dat` `.cfg` payload length in `idFileSystemLocal::ReadFile` recording path | Serialization | Replaced raw native `Write( &len, sizeof( len ) )` with `WriteInt( len )`. |
| Journal playback length validation | Serialization robustness | Added a negative-length guard before allocating the journaled config payload buffer. |

This slice does not widen the journaldata length field. The on-disk length remains a 32-bit signed integer, but the byte order and width are now explicit through `idFile::ReadInt` / `WriteInt` rather than implicit native memory layout. The `.cfg` payload bytes remain unchanged after the length field.

Compile-time guard:

```c
static_assert( sizeof( int ) == 4, "journal data length fields must stay fixed-width" );
```

Compatibility boundary: old same-endian 32-bit/64-bit journaldata files remain readable because the field width is unchanged. Cross-endian journaldata files now follow the existing `ReadInt` little-endian conversion behavior.

### Verification Log For This Slice

- `rg -n "com_journalDataFile->(Read|Write)\s*\(\s*&len|com_journalDataFile->ReadInt\s*\(\s*len\s*\)|com_journalDataFile->WriteInt\s*\(\s*len\s*\)|Invalid journalDataFile length|journal data length" neo/framework/FileSystem.cpp`: stale raw `&len` reads/writes are gone; fixed-width helpers and the negative-length guard are present.
- `cmake --build --preset ninja-gcc-release -j 8`: passed; rebuilt `FileSystem.cpp` and linked `Doom3.exe`, with existing legacy warning noise but no errors.
- `cmake --build --preset ninja-dedicated-release -j 8`: passed; rebuilt `FileSystem.cpp` and linked `DedServer.exe`, with existing legacy warning noise but no errors.
- `Doom3.exe +set fs_basepath F:\IdTech4-Remaster +set com_skipRenderer 1 +set s_noSound 1 +quit`: exit code 0.
- `DedServer.exe +set fs_basepath F:\IdTech4-Remaster +quit`: exit code 0.
- `rg --files | rg "(?i)\.(save|sav|savegame)$"`: no checked-in save corpus was found for a save/load compatibility smoke.
- `git diff --check`: passed.

## 2026-07-14 Trace Event Save Marker And D3XP Fast Event Serialization Slice

### Files Changed

- `neo/game/gamesys/Event.cpp`
- `neo/d3xp/gamesys/Event.cpp`
- `Documentation/POINTER_64BIT_MIGRATION_REPORT.md`

### Classification And Compatibility Story

| Surface | Category | Resolution |
| --- | --- | --- |
| `idEvent::SaveTrace` / `idEvent::RestoreTrace` material field in base game and d3xp | Savegame field / serialization | Replaced `WriteInt( (int&)trace.c.material )` and `ReadInt( (int&)trace.c.material )` with an explicit 32-bit material presence marker. The material name string that already follows the trace payload remains the authoritative restore data. |
| `trace_t::c.material` inside queued event buffers | Pointer storage in transient event data | Runtime event buffers still carry a native material pointer while the event is live, but saves now persist only the 32-bit marker plus the material name string. On restore, a non-null marker is used only to trigger reading the following material name; `ServiceEvents` resolves the real pointer through `declManager->FindMaterial`. |
| Saved event argument size for `D_EVENT_TRACE` | Savegame compatibility | Restore accepts both the current 64-bit event argument size and the legacy 32-bit trace-event argument size (`108 + MAX_STRING_LEN + sizeof(bool)` for each trace argument). It allocates the current event buffer size before reading typed fields, so legacy arg-size validation does not under-allocate on x64. |
| d3xp `_D3XP` fast event queue | Serialization | Replaced raw `savefile->Write( event->data, event->eventdef->GetArgSize() )` and raw restore with the same typed per-argument serialization path used by the normal event queue. This avoids writing native pointer-bearing `trace_t` bytes directly. |
| Queued string event arguments | Serialization | Added explicit `D_EVENT_STRING` read/write handling in typed event serialization, preserving the fixed `MAX_STRING_LEN` payload instead of relying on raw event-data dumps. |

This slice does not widen any savegame field. The trace material slot stays a 32-bit marker in the save stream. Existing old saves that wrote a truncated nonzero material pointer still restore as "material present" because any nonzero marker causes the following material name to be read and later resolved to a real material pointer.

The fixed-width assumptions used by the compatibility path are guarded with compile-time assertions for the trace event scalar/vector/matrix field widths:

```c
static_assert( sizeof( contactType_t ) == sizeof( int ), ... );
static_assert( sizeof( idVec3 ) == 12, ... );
static_assert( sizeof( idMat3 ) == 36, ... );
```

Known boundary: the compatibility path validates legacy 32-bit queued trace-event argument sizes, but this workspace still has no checked-in save corpus to prove a full old-save load through a real saved event queue.

### Verification Log For This Slice

- `rg -n "\(int&\)trace\.c\.material|\(int &\)trace\.c\.material|Write\s*\(\s*event->data\s*,\s*event->eventdef->GetArgSize\s*\(\s*\)\s*\)|Read\s*\(\s*event->data\s*,\s*argsize\s*\)" neo/game/gamesys/Event.cpp neo/d3xp/gamesys/Event.cpp`: no matches.
- `cmake --build --preset ninja-gcc-release -j 8`: passed; rebuilt `Game` and `Game-d3xp`, regenerated TypeInfo, and emitted existing legacy warning noise, but no errors.
- `cmake --build --preset ninja-dedicated-release -j 8`: passed; rebuilt the same touched game modules under the dedicated preset, with existing legacy warning noise but no errors.
- `Doom3.exe +set fs_basepath F:\IdTech4-Remaster +set com_skipRenderer 1 +set s_noSound 1 +quit`: exit code 0.
- `DedServer.exe +set fs_basepath F:\IdTech4-Remaster +quit`: exit code 0.
- `rg --files | rg "(?i)\.(save|sav|savegame)$"`: no checked-in save corpus was found for a save/load compatibility smoke.
- `git diff --check`: passed.

## 2026-07-14 Event Scalar Lane Alias Cleanup Slice

### Files Changed

- `neo/game/gamesys/Class.h`
- `neo/d3xp/gamesys/Class.h`
- `neo/game/gamesys/Event.cpp`
- `neo/d3xp/gamesys/Event.cpp`
- `Documentation/POINTER_64BIT_MIGRATION_REPORT.md`

### Classification And Compatibility Story

| Surface | Category | Resolution |
| --- | --- | --- |
| `idEventArg( float )` in base game and d3xp | Legacy API interop / event argument bit field | Replaced `float` through `int *` aliasing with a `memcpy` helper that stores the same 32-bit IEEE payload in the transient `intptr_t` event argument lane. |
| Queued `D_EVENT_FLOAT` / `D_EVENT_INTEGER` storage in base game and d3xp | Serialization / savegame field helper | Added explicit 4-byte scalar-lane read/write helpers for queued event buffers instead of reading and writing through `int *` / `float *` aliases. |
| d3xp fast-event scalar dispatch and save/restore | Serialization / savegame field helper | Applied the same helpers to the `_D3XP` fast-event queue so normal and fast event queues preserve identical scalar lane semantics. |

This slice does not widen queued event buffers, savegame fields, or network fields. `D_EVENT_FLOAT` and `D_EVENT_INTEGER` still occupy exactly four bytes in queued event storage and saved event streams. The already pointer-sized transient callback array remains `intptr_t`; scalar event values are copied into or out of the low 32-bit lane explicitly.

Compile-time guards:

```c
static_assert( sizeof( int ) == 4, "event argument integer lanes must stay 32-bit" );
static_assert( sizeof( float ) == 4, "event argument float lanes must stay 32-bit" );
```

Known boundary: script interpreter stack pushes still contain separate float/vector bit-punning sites. Those are script VM stack lanes rather than queued event lanes and should be cleaned in a dedicated script-VM slice.

### Verification Log For This Slice

- `rg -n "idEventArg\( float data \)|\*reinterpret_cast<\s*(int|float)\s*\*>\s*\(\s*&data|\*reinterpret_cast<\s*(int|float)\s*\*>\s*\(\s*dataPtr\s*\)|\*reinterpret_cast<\s*(int|float)\s*\*>\s*\(\s*&data\[\s*offset\s*\]\s*\)" neo\game\gamesys\Class.h neo\d3xp\gamesys\Class.h neo\game\gamesys\Event.cpp neo\d3xp\gamesys\Event.cpp`: no matches.
- `rg -n "Event_Write(Int|Float)Lane|Event_Read(Int|Float)Lane|idEventArg_FloatToBits|event argument (integer|float) lanes" neo\game\gamesys\Class.h neo\d3xp\gamesys\Class.h neo\game\gamesys\Event.cpp neo\d3xp\gamesys\Event.cpp`: helper definitions, use sites, and guards are present.
- `cmake --build --preset ninja-gcc-release -j 8`: passed; rebuilt the game and d3xp modules, regenerated/linking outputs as needed, and emitted existing legacy warning noise but no errors.
- `cmake --build --preset ninja-dedicated-release -j 8`: passed; rebuilt the touched game modules under the dedicated preset, with existing legacy warning noise but no errors.
- `Doom3.exe +set fs_basepath F:\IdTech4-Remaster +set com_skipRenderer 1 +set s_noSound 1 +quit`: exit code 0.
- `DedServer.exe +set fs_basepath F:\IdTech4-Remaster +quit`: exit code 0.
- `rg --files | rg "(?i)\.(save|sav|savegame)$"`: no checked-in save corpus was found for a save/load compatibility smoke.
- `git diff --check`: passed.

## 2026-07-15 Script VM Local Stack Push Lane Cleanup Slice

### Files Changed

- `neo/game/script/Script_Interpreter.h`
- `neo/d3xp/script/Script_Interpreter.h`
- `Documentation/POINTER_64BIT_MIGRATION_REPORT.md`

### Classification And Compatibility Story

| Surface | Category | Resolution |
| --- | --- | --- |
| Base game `idInterpreter::Push( int )` local stack write | Serialization / script VM local stack scalar lane | Replaced `int *` aliasing of the byte `localstack` with `memcpy`, preserving the existing four-byte VM scalar lane used by integer/entity/bool and float-bit pushes. |
| d3xp `idInterpreter::Push( int )` local stack write | Serialization / script VM local stack scalar lane | Applied the same fixed-width write to the d3xp interpreter stack. |

This slice does not widen savegame, network, demo, journal, script bytecode, or VM stack fields. `localstack` remains a byte array, and each scalar push still advances by `sizeof( int )`. Existing script float/vector push sites that convert floats to 32-bit integer lanes continue to produce the same stack bytes; only the object-representation write into the byte stack changed.

Compile-time guard:

```c
static_assert( sizeof( int ) == 4, "script VM local stack scalar lanes must stay 32-bit" );
```

Known boundary: `varEval_t` and several interpreter read/write paths still expose typed `int *`, `float *`, and `idVec3 *` views into `localstack`. Those are broader VM layout surfaces and should be audited in a dedicated VM stack-view slice before changing stack layout or save compatibility.

### Verification Log For This Slice

- `rg -n "\*\s*\(\s*int\s*\*\s*\)\s*&localstack\[ localstackUsed \]|script VM local stack scalar lanes|memcpy\( &localstack\[ localstackUsed \]" neo\game\script\Script_Interpreter.h neo\d3xp\script\Script_Interpreter.h`: stale push write aliases are gone; fixed-width guard and `memcpy` writes are present.
- `cmake --build --preset ninja-gcc-release -j 8`: passed; rebuilt broad game/d3xp script-dependent surfaces, regenerated TypeInfo, linked `Doom3.exe`, and emitted existing legacy warning noise but no errors.
- `cmake --build --preset ninja-dedicated-release -j 8`: passed; rebuilt broad game/d3xp script-dependent surfaces for the dedicated preset and emitted existing legacy warning noise but no errors. A final incremental run reported no work remaining.
- `Doom3.exe +set fs_basepath F:\IdTech4-Remaster +set com_skipRenderer 1 +set s_noSound 1 +quit`: exit code 0.
- `DedServer.exe +set fs_basepath F:\IdTech4-Remaster +quit`: exit code 0.
- `rg --files | rg "(?i)\.(save|sav|savegame)$"`: no checked-in save corpus was found for a save/load compatibility smoke.
- `git diff --check`: passed.

## 2026-07-15 Sys Sound And Legacy Stack Patch Pointer Arithmetic Slice

### Files Changed

- `neo/sys/linux/sound.cpp`
- `neo/sys/linux/sound_alsa.cpp`
- `neo/sys/win32/win_main.cpp`
- `Documentation/POINTER_64BIT_MIGRATION_REPORT.md`

### Classification And Compatibility Story

| Surface | Category | Resolution |
| --- | --- | --- |
| OSS `idAudioHardwareOSS::Write` mix-buffer write pointer | Legacy API interop / runtime pointer arithmetic | Replaced `(int)m_buffer + byteOffset` with typed `byte *` pointer arithmetic and a `p_writeBuffer` pointer local before calling `write`. |
| ALSA `idAudioHardwareALSA::Write` mix-buffer write pointer | Legacy API interop / runtime pointer arithmetic | Replaced `(int)m_buffer + byteOffset` with typed `byte *` pointer arithmetic and passed the resulting pointer to `snd_pcm_writei`. |
| Win32 legacy `_chkstk` jump patch | Legacy API interop / 32-bit instruction patch | Removed `(int)_chkstk` / `(int)clrstk` address math. The patch now uses a byte pointer for the write target and `intptr_t` for the address difference before narrowing to the required 32-bit relative jump displacement. |

This slice does not widen savegame, network, demo, journal, renderer handle, or asset formats. The Linux audio changes are process-local buffer pointer arithmetic only. The Win32 `_chkstk` patch remains compiled only for the legacy non-x64 path and still writes the same 5-byte relative jump encoding.

Compile-time guard:

```c
static_assert( sizeof( int ) == 4, "relative jump patch displacement must stay 32-bit" );
```

Known boundary: the Windows CMake presets compile and link `neo/sys/win32/win_main.cpp`; they do not compile the Linux OSS/ALSA backend files in this environment. The Linux backend changes were verified with source scans here and should still be compiled on a Linux target when that build path is available.

### Verification Log For This Slice

- `rg -n "\(int\)m_buffer|int pos = \(int\)|\(int\)_chkstk|\(int\)clrstk|\*\(int \*\)\(\(int\)" neo\sys\linux\sound.cpp neo\sys\linux\sound_alsa.cpp neo\sys\win32\win_main.cpp`: no matches.
- `rg -n "byteOffset|p_writeBuffer|p_chkstkBytes|relative jump patch displacement|reinterpret_cast<intptr_t>\( (_chkstk|clrstk) \)" neo\sys\linux\sound.cpp neo\sys\linux\sound_alsa.cpp neo\sys\win32\win_main.cpp`: helper locals and guard are present.
- `cmake --build --preset ninja-gcc-release -j 8`: passed; rebuilt `neo/sys/win32/win_main.cpp` and linked `Doom3.exe`, with existing legacy warning noise but no errors.
- `cmake --build --preset ninja-dedicated-release -j 8`: passed; rebuilt `neo/sys/win32/win_main.cpp` and linked `DedServer.exe`, with existing legacy warning noise but no errors.
- `Doom3.exe +set fs_basepath F:\IdTech4-Remaster +set com_skipRenderer 1 +set s_noSound 1 +quit`: exit code 0.
- `DedServer.exe +set fs_basepath F:\IdTech4-Remaster +quit`: exit code 0.
- `rg --files | rg "(?i)\.(save|sav|savegame)$"`: no checked-in save corpus was found for a save/load compatibility smoke.
- `git diff --check`: passed.

## 2026-07-15 Map CRC And MegaTexture 32-Bit Lane Alias Cleanup Slice

### Files Changed

- `neo/idlib/MapFile.cpp`
- `neo/renderer/MegaTexture.cpp`
- `Documentation/POINTER_64BIT_MIGRATION_REPORT.md`

### Classification And Compatibility Story

| Surface | Category | Resolution |
| --- | --- | --- |
| `FloatCRC` in `neo/idlib/MapFile.cpp` | Legacy API interop / map geometry CRC bit lane | Replaced `*( unsigned int * )&f` with a `memcpy` copy into a 32-bit integer lane, preserving the exact IEEE float bits used by map geometry CRC generation. |
| MegaTexture debug label RGBA stamp in `idTextureLevel::UpdateTile` | Renderer texture helper / fixed 32-bit color lane | Replaced `*( int * )` color/data aliasing with a four-byte `memcpy` from the RGBA label color into the tile buffer. |

This slice does not widen savegame, network, demo, journal, renderer handle, map, or asset formats. Map geometry CRCs still use the same 32-bit float object representation, and MegaTexture debug labels still stamp exactly four RGBA bytes per pixel. The change only removes strict-aliasing access to those fixed-width lanes.

Compile-time guards:

```c
static_assert( sizeof( unsigned int ) == 4, "map float CRC integer lane must stay 32-bit" );
static_assert( sizeof( float ) == 4, "map float CRC float lane must stay 32-bit" );
static_assert( sizeof( color ) == 4, "megaTexture label color storage must stay 32-bit" );
```

### Verification Log For This Slice

- `rg -n "\*\(unsigned int \*\)&f|\*\(int \*\)&data|\*\(int \*\)color|map float CRC|megaTexture label color|memcpy\( &bits|memcpy\( &data" neo\idlib\MapFile.cpp neo\renderer\MegaTexture.cpp`: stale alias expressions are gone; fixed-width guards and `memcpy` paths are present.
- `cmake --build --preset ninja-gcc-release -j 8`: passed; rebuilt `neo/idlib/MapFile.cpp`, `neo/renderer/MegaTexture.cpp`, and linked `Doom3.exe`, with existing legacy warning noise but no errors.
- `cmake --build --preset ninja-dedicated-release -j 8`: passed; rebuilt the same touched source files and linked `DedServer.exe`, with existing legacy warning noise but no errors.
- `Doom3.exe +set fs_basepath F:\IdTech4-Remaster +set com_skipRenderer 1 +set s_noSound 1 +quit`: exit code 0.
- `DedServer.exe +set fs_basepath F:\IdTech4-Remaster +quit`: exit code 0.
- `rg --files | rg "(?i)\.(save|sav|savegame)$"`: no checked-in save corpus was found for a save/load compatibility smoke.
- `git diff --check`: passed.

## 2026-07-15 MMX Memcpy Alignment Peel Cleanup Slice

### Files Changed

- `neo/idlib/math/Simd_MMX.cpp`
- `Documentation/POINTER_64BIT_MIGRATION_REPORT.md`

### Classification And Compatibility Story

| Surface | Category | Resolution |
| --- | --- | --- |
| `idSIMD_MMX::Memcpy` destination pre-alignment peel | Legacy API interop / runtime pointer alignment arithmetic | Kept destination alignment testing in `uintptr_t`, then computed the bounded byte count needed to reach the next 8-byte boundary before entering the MMX copy loops. |

This slice does not widen savegame, network, demo, journal, renderer handle, model, or asset formats. The changed state is process-local MMX copy alignment arithmetic only. `idSIMD_MMX::Memcpy` keeps the same signature and still uses `int` byte counts for the existing ABI; the pointer-derived value is reduced to a 0-7 byte peel count before narrowing.

Known boundary: `neo/idlib/math/Simd_MMX.cpp` is excluded from the current x64 CMake target source list, so the direct compiler coverage for this source is the Win32 MSBuild `idLib` target. The x64 CMake runtime builds still passed after the source/report update.

### Verification Log For This Slice

- `rg -n "static_cast<int>\( reinterpret_cast<uintptr_t>\( p_dest \) & 7 \)|reinterpret_cast<uintptr_t>\( p_dest \) & 7|destAlignment|8 byte aligned boundary" neo\idlib\math\Simd_MMX.cpp`: stale direct pointer-mask-to-int expression is gone; the `destAlignment` low-bit lane and existing `Memset` byte-peel guard are present.
- `MSBuild.exe neo\idlib.vcxproj /p:Configuration=Release /p:Platform=Win32 /clp:ErrorsOnly`: passed; this is direct compiler coverage for `neo/idlib/math/Simd_MMX.cpp`.
- `cmake --build --preset ninja-gcc-release -j 8`: passed; x64 CMake runtime target graph reported no work remaining.
- `cmake --build --preset ninja-dedicated-release -j 8`: passed; x64 dedicated target graph reported no work remaining.
- `Doom3.exe +set fs_basepath F:\IdTech4-Remaster +set com_skipRenderer 1 +set s_noSound 1 +quit`: exit code 0.
- `DedServer.exe +set fs_basepath F:\IdTech4-Remaster +quit`: exit code 0.
- `rg --files | rg "(?i)\.(save|sav|savegame)$"`: no checked-in save corpus was found for a save/load compatibility smoke.
- `git diff --check`: passed.

## 2026-07-15 SSE2 CmpLT Destination Lane Alias Cleanup Slice

### Files Changed

- `neo/idlib/math/Simd_SSE2.cpp`
- `Documentation/POINTER_64BIT_MIGRATION_REPORT.md`

### Classification And Compatibility Story

| Surface | Category | Resolution |
| --- | --- | --- |
| Legacy Mac SSE2 `idSIMD_SSE2::CmpLT` destination mask lane | Legacy API interop / SIMD fixed 32-bit lane | Replaced `*((int *) p_dstBytes)` reads and writes with explicit `dword` lane helpers that copy exactly four bytes with `memcpy`. |

This slice does not widen savegame, network, demo, journal, renderer handle, model, or asset formats. The destination comparison mask still updates one four-byte lane per four source floats, and the lane type is now explicitly `dword` to match the existing SSE2 fixed-width mask guard.

Known boundary: this code is inside the legacy `MACOS_X && __i386__` SSE2 path. It is not part of the current x64 CMake runtime source list, so direct compiler coverage here is the Win32 MSBuild `idLib` target. The active x64 CMake runtime target graph still passed after the source/report update.

### Verification Log For This Slice

- `rg -n "\*\s*\(\s*int\s*\*\s*\)\s*p_dstBytes|\(\s*int\s*\*\s*\)\s*p_dstBytes|SSE2_ReadDwordLane|SSE2_WriteDwordLane|dword mask_l|dword dst_l|0x01010101u" neo\idlib\math\Simd_SSE2.cpp`: stale destination `int *` lane accesses are gone; fixed-width helpers and `dword` lanes are present.
- `MSBuild.exe neo\idlib.vcxproj /p:Configuration=Release /p:Platform=Win32 /clp:ErrorsOnly`: passed; this is direct compiler coverage for `neo/idlib/math/Simd_SSE2.cpp`.
- `cmake --build --preset ninja-gcc-release -j 8`: passed; x64 CMake runtime target graph reported no work remaining.
- `cmake --build --preset ninja-dedicated-release -j 8`: passed; x64 dedicated target graph reported no work remaining.
- `Doom3.exe +set fs_basepath F:\IdTech4-Remaster +set com_skipRenderer 1 +set s_noSound 1 +quit`: exit code 0.
- `DedServer.exe +set fs_basepath F:\IdTech4-Remaster +quit`: exit code 0.
- `rg --files | rg "(?i)\.(save|sav|savegame)$"`: no checked-in save corpus was found for a save/load compatibility smoke.
- `git diff --check`: passed.

## 2026-07-15 Script VM Local Stack Typed-View Helper Slice

### Files Changed

- `neo/game/script/Script_Interpreter.h`
- `neo/game/script/Script_Interpreter.cpp`
- `neo/d3xp/script/Script_Interpreter.h`
- `neo/d3xp/script/Script_Interpreter.cpp`
- `Documentation/POINTER_64BIT_MIGRATION_REPORT.md`

### Classification And Compatibility Story

| Surface | Category | Resolution |
| --- | --- | --- |
| Base and d3xp `idInterpreter::GetVariable` stack-variable view | Serialization / script VM local stack typed view | Replaced the ad-hoc `( int * )&localstack[...]` stack view construction with `LocalStackValue`, which creates the same `varEval_t` byte-addressed view through one helper. |
| Base and d3xp `CallEvent` / `CallSysEvent` argument extraction | Legacy API interop / script-to-event stack view | Replaced repeated event bridge casts from `localstack` bytes to `int *` with `LocalStackValue`; string arguments now use `LocalStackBytes`. |
| Script VM local stack lane assumptions | Serialization / savegame field guard | Added compile-time guards for 32-bit integer lanes, 32-bit float lanes, and 3-float vector lanes used by raw `localstack` save/restore bytes. |

This slice does not widen savegame, network, demo, journal, script bytecode, or VM stack fields. `localstack` remains a byte array and is still saved/restored as raw bytes with `localstackUsed` and `localstackBase` 32-bit counters. The helper centralizes typed views in the touched event/get-variable paths without changing the underlying serialized byte layout.

Compile-time guards:

```c
static_assert( sizeof( int ) == 4, "script VM local stack scalar lanes must stay 32-bit" );
static_assert( sizeof( float ) == 4, "script VM local stack float lanes must stay 32-bit" );
static_assert( sizeof( idVec3 ) == 3 * sizeof( float ), "script VM local stack vector lanes must stay 3 floats" );
```

Known boundary: the broader `varEval_t` design still exposes typed pointer members into constants, globals, and other stack paths. This slice intentionally does not replace interpreter arithmetic/evaluation with `memcpy` load/store helpers, because that would be a larger VM layout compatibility change.

### Verification Log For This Slice

- `rg -n "LocalStackBytes|LocalStackValue|script VM local stack (float|vector)|\( int \* \)&localstack\[ localstackBase \+ def->value\.stackOffset \]|\( int \* \)&localstack\[ start( \+ pos)? \]|&localstack\[ start \+ pos \]" neo\game\script\Script_Interpreter.h neo\d3xp\script\Script_Interpreter.h neo\game\script\Script_Interpreter.cpp neo\d3xp\script\Script_Interpreter.cpp`: helper functions, guards, and touched use sites are present; stale direct casts in the touched `GetVariable`, `CallEvent`, and `CallSysEvent` paths are gone.
- `cmake --build --preset ninja-gcc-release -j 8`: passed; rebuilt broad game/d3xp script-dependent surfaces, with existing legacy warning noise but no errors.
- `cmake --build --preset ninja-dedicated-release -j 8`: passed; rebuilt broad game/d3xp script-dependent surfaces for the dedicated preset, with existing legacy warning noise but no errors.
- `Doom3.exe +set fs_basepath F:\IdTech4-Remaster +set com_skipRenderer 1 +set s_noSound 1 +quit`: exit code 0.
- `DedServer.exe +set fs_basepath F:\IdTech4-Remaster +quit`: exit code 0.
- `rg --files | rg "(?i)\.(save|sav|savegame)$"`: no checked-in save corpus was found for a save/load compatibility smoke.
- `git diff --check`: passed.

## 2026-07-15 SSE MinMax Index Lane Alias Cleanup Slice

### Files Changed

- `neo/idlib/math/Simd_SSE.cpp`
- `Documentation/POINTER_64BIT_MIGRATION_REPORT.md`

### Classification And Compatibility Story

| Surface | Category | Resolution |
| --- | --- | --- |
| Legacy Mac/i386 SSE `idSIMD_SSE::MinMax` index reads | Legacy API interop / SIMD fixed 32-bit index lane | Replaced direct `int *` alias reads from `indexes` byte offsets with `SimdSSE_ReadIndexLane`, which copies exactly four bytes into an `int` lane. |
| `indexes_p` byte cursor | Legacy API interop / pointer const-correctness | Kept the byte cursor as `const char *`, matching the public `const int *indexes` input and avoiding a mutable view of index storage. |

This slice does not widen savegame, network, demo, journal, renderer handle, model, asset, or index formats. The public SIMD API still consumes `const int *indexes`, and each index lane remains an explicitly guarded 32-bit value before being multiplied by the fixed `idDrawVert` byte stride.

Compile-time guard:

```c
static_assert( sizeof( int ) == 4, "SSE index lanes must stay 32-bit" );
```

Known boundary: this code is inside the legacy `MACOS_X && __i386__` SSE path. It is not part of the current x64 CMake runtime source list, so direct compiler coverage here is the Win32 MSBuild `idLib` target. The active x64 CMake runtime target graph still passed after the source/report update.

### Verification Log For This Slice

- `rg -n "\*\s*\(\s*int\s*\*\s*\)\s*\(indexes_p\+|SimdSSE_ReadIndexLane|SSE index lanes|const char \*indexes_p" neo\idlib\math\Simd_SSE.cpp`: stale direct `int *` index-lane reads are gone; the fixed-width helper, guard, and const byte cursor are present.
- `MSBuild.exe neo\idlib.vcxproj /p:Configuration=Release /p:Platform=Win32 /clp:ErrorsOnly`: passed; this is direct compiler coverage for `neo/idlib/math/Simd_SSE.cpp`.
- `cmake --build --preset ninja-gcc-release -j 8`: passed; x64 CMake runtime target graph reported no work remaining.
- `cmake --build --preset ninja-dedicated-release -j 8`: passed; x64 dedicated target graph reported no work remaining.
- `Doom3.exe +set fs_basepath F:\IdTech4-Remaster +set com_skipRenderer 1 +set s_noSound 1 +quit`: exit code 0.
- `DedServer.exe +set fs_basepath F:\IdTech4-Remaster +quit`: exit code 0.
- `rg --files | rg "(?i)\.(save|sav|savegame)$"`: no checked-in save corpus was found for a save/load compatibility smoke.
- `git diff --check`: passed.
