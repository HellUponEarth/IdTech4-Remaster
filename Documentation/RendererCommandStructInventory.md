# Renderer Command Struct Inventory

Date: 2026-07-13

This inventory records the renderer backend command queue structs and their producers/consumers. The command queue is defined in `neo/renderer/tr_local.h` and allocated from per-frame renderer memory by `R_GetCommandBuffer`.

## Command Tags And Payload Structs

| Command/tag | Struct | Definition | Payload | Producer | Consumer |
| --- | --- | --- | --- | --- | --- |
| `RC_NOP` | `emptyCommand_t` | `neo/renderer/tr_local.h:473`, `neo/renderer/tr_local.h:481` | `commandId`, `next` | `R_ClearCommandChain` initializes the command head at `neo/renderer/RenderSystem.cpp:182-184`. | `RB_ExecuteBackEndCommands` skips it at `neo/renderer/tr_backend.cpp:618`. |
| `RC_DRAW_VIEW` | `drawSurfsCommand_t` | `neo/renderer/tr_local.h:474`, `neo/renderer/tr_local.h:491` | `viewDef_t *viewDef` | `R_AddDrawViewCmd` at `neo/renderer/RenderSystem.cpp:209-218`; `R_LockSurfaceScene` reuses `tr.lockSurfacesCmd`. | `RB_ExecuteBackEndCommands` dispatches to `RB_DrawView` at `neo/renderer/tr_backend.cpp:620-627`; `RB_DrawView` decodes it in `neo/renderer/tr_render.cpp:850-852`. |
| `RC_SET_BUFFER` | `setBufferCommand_t` | `neo/renderer/tr_local.h:475`, `neo/renderer/tr_local.h:485` | `GLenum buffer`, `int frameCount` | `idRenderSystemLocal::SetBuffer` queues it at `neo/renderer/RenderSystem.cpp:628-681`. | `RB_SetBuffer` decodes it at `neo/renderer/tr_backend.cpp:454-458`; backend dispatch at `neo/renderer/tr_backend.cpp:629-632`. |
| `RC_COPY_RENDER` | `copyRenderCommand_t` | `neo/renderer/tr_local.h:476`, `neo/renderer/tr_local.h:496` | `x`, `y`, `imageWidth`, `imageHeight`, `idImage *image`, `cubeFace` | `idRenderSystemLocal::CaptureRenderToImage` queues it at `neo/renderer/RenderSystem.cpp:935-936`. | `RB_CopyRender` decodes it at `neo/renderer/tr_backend.cpp:576-578`; backend dispatch at `neo/renderer/tr_backend.cpp:637-640`. |
| `RC_SWAP_BUFFERS` | `emptyCommand_t` | `neo/renderer/tr_local.h:477`, `neo/renderer/tr_local.h:481` | `commandId`, `next` only | `idRenderSystemLocal::SwapCommandBuffers` queues it at `neo/renderer/RenderSystem.cpp:709-738`. | `RB_SwapBuffers` from backend dispatch at `neo/renderer/tr_backend.cpp:633-636`. |

## Queue Ownership And Lifetime

| Surface | Location | Notes |
| --- | --- | --- |
| `R_GetCommandBuffer` | `neo/renderer/RenderSystem.cpp:161-169` | Allocates command payload bytes with `R_FrameAlloc`, links the new command through `frameData->cmdTail->next`, and advances `cmdTail`. |
| `R_ClearCommandChain` | `neo/renderer/RenderSystem.cpp:182-185` | Resets the list to a single `RC_NOP` head. |
| `R_IssueRenderCommands` | `neo/renderer/RenderSystem.cpp:130-151` | Submits the current command chain to `RB_ExecuteBackEndCommands` unless skipped, then clears the chain. |
| `RB_ExecuteBackEndCommands` | `neo/renderer/tr_backend.cpp:600-641` | Iterates the intrusive linked list by casting `next` back to `emptyCommand_t *` and switching on `commandId`. |
| `frameData_t::cmdHead/cmdTail` | `neo/renderer/tr_local.h:542` | Queue head/tail storage in per-frame renderer data. |
| `tr.lockSurfacesCmd` | `neo/renderer/tr_local.h:790` | Cached `drawSurfsCommand_t` used by `r_lockSurfaces` debugging. |

## Struct Shape

```cpp
typedef enum {
    RC_NOP,
    RC_DRAW_VIEW,
    RC_SET_BUFFER,
    RC_COPY_RENDER,
    RC_SWAP_BUFFERS
} renderCommand_t;

typedef struct {
    renderCommand_t commandId, *next;
} emptyCommand_t;

typedef struct {
    renderCommand_t commandId, *next;
    GLenum buffer;
    int frameCount;
} setBufferCommand_t;

typedef struct {
    renderCommand_t commandId, *next;
    viewDef_t *viewDef;
} drawSurfsCommand_t;

typedef struct {
    renderCommand_t commandId, *next;
    int x, y, imageWidth, imageHeight;
    idImage *image;
    int cubeFace;
} copyRenderCommand_t;
```

## Modernization Notes

- The `next` field is typed as `renderCommand_t *`, not `emptyCommand_t *`, so the queue relies on `commandId` being the first field of every payload struct.
- Every payload struct begins with `commandId, *next`; any new backend command must preserve that prefix or change all queue traversal/casts.
- `RC_SWAP_BUFFERS` reuses `emptyCommand_t`; there is no dedicated swap payload today.
- `copyRenderCommand_t::cubeFace` is present in the payload but the current `RB_CopyRender` implementation only copies through `cmd->image->CopyFramebuffer(...)`.
