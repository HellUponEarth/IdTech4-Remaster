# Direct OpenGL Call Inventory

Date: 2026-07-13

This inventory records current direct OpenGL, WGL, and QGL-wrapper call sites in the engine, platform layer, and in-tree tools. It treats `qgl*` and `qwgl*` calls as direct renderer-backend dependencies because they bind straight through the engine's OpenGL dispatch table. Raw `gl*` and `wgl*` calls are called out separately where they bypass the QGL naming layer.

Excluded from the table: generated extension declaration headers (`glext.h`, `wglext.h`) and third-party/vendor trees. They contain API declarations or unrelated compatibility code, not engine-owned call sites.

## Scan Pattern

```powershell
$rx = '\b(qgl[A-Z][A-Za-z0-9_]*|qwgl[A-Z][A-Za-z0-9_]*|(?<!q)gl[A-Z][A-Za-z0-9_]*|(?<!q)wgl[A-Z][A-Za-z0-9_]*)\s*\('
Get-ChildItem neo\renderer,neo\sys,neo\tools -Recurse -Include *.c,*.cpp,*.h,*.mm,*.m |
  Where-Object { $_.FullName -notmatch '\\jpeg-6\\|\\curl\\|\\openal\\|glext\.h$|wglext\.h$' } |
  Select-String -Pattern $rx
```

## Primary Runtime Renderer Surface

| Area | Files | Calls | Notes |
| --- | --- | ---: | --- |
| Backend command execution and GL state | `neo/renderer/tr_backend.cpp`, `neo/renderer/tr_render.cpp` | 152 | Backend draw-buffer, clear, copy-render, swap, draw-view, and GL state transitions. |
| Material interaction paths | `neo/renderer/draw_common.cpp`, `draw_arb*.cpp`, `draw_exp.cpp`, `draw_nv*.cpp`, `draw_r200.cpp` | 1,086 | Fixed-function, ARB, NV, ATI/R200, and experimental path rendering calls. `draw_exp.cpp` also reaches raw WGL pbuffer APIs. |
| Texture and image upload/copy | `neo/renderer/Image_load.cpp`, `Image_init.cpp`, `Image.h`, `MegaTexture.cpp` | 118 | Texture creation, upload, compression, readback, and background-load completion surfaces. |
| Debug and editor renderer helpers | `neo/renderer/tr_rendertools.cpp`, `tr_trace.cpp`, `RenderWorld_portals.cpp` | 348 | Debug line/text/polygon drawing, trace visualization, and portal visualization. |
| Render-system setup | `neo/renderer/RenderSystem_init.cpp`, `RenderSystem.cpp`, `VertexCache.cpp`, `GuiModel.cpp`, `tr_local.h` | 39 | Startup capability probes, command setup, vertex-cache buffer calls, GUI model upload references. |

## Platform GL/WGL Binding Surface

| File | Calls | Kinds | Notes |
| --- | ---: | --- | --- |
| `neo/sys/win32/win_glimp.cpp` | 22 | `qgl/qwgl=13`, `raw wgl=9` | Win32 GL context creation, pixel format, fullscreen/display-mode, WGL extension probing, and buffer/context lifecycle. |
| `neo/sys/win32/win_qgl.cpp` | 1 | `raw gl=1` | Win32 QGL binding helper surface. |
| `neo/sys/linux/glimp.cpp` | 9 | `qgl/qwgl=9` | Linux GL context/system binding path. |
| `neo/sys/osx/macosx_glimp.mm` | 39 | `qgl/qwgl=17`, `raw gl=21`, `raw wgl=1` | macOS GL setup and platform-specific GL handling. |
| `neo/sys/stub/stub_gl.cpp` | 336 | `raw gl=336` | Stubbed GL exports for non-rendering or test builds. |

## Tool And Editor GL Surface

| Area | Files | Calls | Notes |
| --- | --- | ---: | --- |
| Radiant 2D/3D views | `CamWnd.cpp`, `XYWnd.cpp`, `Z.CPP`, `ZWnd.cpp`, `ZClip.cpp`, `GLWidget.cpp`, `PointFile.cpp`, `PMESH.CPP`, `EditorBrush.cpp`, `NewTexWnd.cpp`, `splines.cpp/.h`, `Radiant.cpp`, `QE3.CPP`, `WIN_DLG.CPP`, `WIN_QE3.CPP` | 1,078 | Editor viewport drawing, brush/wireframe drawing, clip planes, texture browser, point-file and spline rendering. |
| GUI editor | `GEWorkspace.cpp`, `GESelectionMgr.cpp`, `GEViewer.cpp` | 134 | GUI editor canvas, selection handles, and preview rendering. |
| Material editor | `MaterialPreviewView.cpp` | 7 | Material preview viewport. |
| Offline compilers/debug visualizers | `tools/compilers/dmap/gldraw.cpp`, `dmap/optimize.cpp`, `renderbump/renderbump.cpp` | 210 | DMAP debug drawing and renderbump capture/generation. |

## Complete Engine-Owned Call-Bearing Files

| File | Calls | Kinds |
| --- | ---: | --- |
| `neo/renderer/draw_arb.cpp` | 98 | `qgl/qwgl=98` |
| `neo/renderer/draw_arb2.cpp` | 66 | `qgl/qwgl=65`, `raw gl=1` |
| `neo/renderer/draw_common.cpp` | 254 | `qgl/qwgl=254` |
| `neo/renderer/draw_exp.cpp` | 319 | `qgl/qwgl=298`, `raw gl=3`, `raw wgl=18` |
| `neo/renderer/draw_nv10.cpp` | 100 | `qgl/qwgl=100` |
| `neo/renderer/draw_nv20.cpp` | 161 | `qgl/qwgl=160`, `raw gl=1` |
| `neo/renderer/draw_r200.cpp` | 88 | `qgl/qwgl=88` |
| `neo/renderer/GuiModel.cpp` | 1 | `qgl/qwgl=1` |
| `neo/renderer/Image.h` | 2 | `qgl/qwgl=2` |
| `neo/renderer/Image_init.cpp` | 10 | `qgl/qwgl=10` |
| `neo/renderer/Image_load.cpp` | 101 | `qgl/qwgl=101` |
| `neo/renderer/MegaTexture.cpp` | 5 | `qgl/qwgl=5` |
| `neo/renderer/RenderSystem.cpp` | 2 | `qgl/qwgl=2` |
| `neo/renderer/RenderSystem_init.cpp` | 20 | `qgl/qwgl=17`, `raw gl=3` |
| `neo/renderer/RenderWorld_portals.cpp` | 5 | `qgl/qwgl=5` |
| `neo/renderer/tr_backend.cpp` | 83 | `qgl/qwgl=82`, `raw gl=1` |
| `neo/renderer/tr_local.h` | 1 | `raw gl=1` |
| `neo/renderer/tr_render.cpp` | 69 | `qgl/qwgl=69` |
| `neo/renderer/tr_rendertools.cpp` | 330 | `qgl/qwgl=330` |
| `neo/renderer/tr_trace.cpp` | 13 | `qgl/qwgl=13` |
| `neo/renderer/VertexCache.cpp` | 15 | `qgl/qwgl=15` |
| `neo/sys/linux/glimp.cpp` | 9 | `qgl/qwgl=9` |
| `neo/sys/osx/macosx_glimp.mm` | 39 | `qgl/qwgl=17`, `raw gl=21`, `raw wgl=1` |
| `neo/sys/stub/stub_gl.cpp` | 336 | `raw gl=336` |
| `neo/sys/win32/win_glimp.cpp` | 22 | `qgl/qwgl=13`, `raw wgl=9` |
| `neo/sys/win32/win_qgl.cpp` | 1 | `raw gl=1` |
| `neo/tools/compilers/dmap/gldraw.cpp` | 58 | `raw gl=58` |
| `neo/tools/compilers/dmap/optimize.cpp` | 99 | `qgl/qwgl=99` |
| `neo/tools/compilers/renderbump/renderbump.cpp` | 53 | `qgl/qwgl=53` |
| `neo/tools/guied/GESelectionMgr.cpp` | 68 | `qgl/qwgl=68` |
| `neo/tools/guied/GEViewer.cpp` | 15 | `qgl/qwgl=11`, `raw gl=4` |
| `neo/tools/guied/GEWorkspace.cpp` | 51 | `qgl/qwgl=43`, `raw gl=8` |
| `neo/tools/materialeditor/MaterialPreviewView.cpp` | 7 | `qgl/qwgl=7` |
| `neo/tools/radiant/CamWnd.cpp` | 121 | `qgl/qwgl=115`, `raw gl=5`, `raw wgl=1` |
| `neo/tools/radiant/EditorBrush.cpp` | 219 | `qgl/qwgl=211`, `raw gl=8` |
| `neo/tools/radiant/GLWidget.cpp` | 62 | `qgl/qwgl=61`, `raw gl=1` |
| `neo/tools/radiant/NewTexWnd.cpp` | 44 | `qgl/qwgl=44` |
| `neo/tools/radiant/PMESH.CPP` | 111 | `qgl/qwgl=111` |
| `neo/tools/radiant/PointFile.cpp` | 20 | `qgl/qwgl=20` |
| `neo/tools/radiant/QE3.CPP` | 1 | `qgl/qwgl=1` |
| `neo/tools/radiant/Radiant.cpp` | 9 | `qgl/qwgl=9` |
| `neo/tools/radiant/splines.cpp` | 58 | `qgl/qwgl=50`, `raw gl=8` |
| `neo/tools/radiant/splines.h` | 3 | `raw gl=3` |
| `neo/tools/radiant/WIN_DLG.CPP` | 4 | `qgl/qwgl=4` |
| `neo/tools/radiant/WIN_QE3.CPP` | 2 | `qgl/qwgl=2` |
| `neo/tools/radiant/XYWnd.cpp` | 323 | `qgl/qwgl=322`, `raw gl=1` |
| `neo/tools/radiant/Z.CPP` | 68 | `qgl/qwgl=68` |
| `neo/tools/radiant/ZClip.cpp` | 30 | `qgl/qwgl=30` |
| `neo/tools/radiant/ZClip.h` | 1 | `raw gl=1` |
| `neo/tools/radiant/ZWnd.cpp` | 5 | `qgl/qwgl=4`, `raw wgl=1` |

## Modernization Notes

- The runtime renderer is not cleanly abstracted behind a backend-neutral interface; `qgl*` calls are spread across draw paths, image upload, debug draw, and backend command execution.
- Win32 WGL setup is mostly localized to `neo/sys/win32/win_glimp.cpp`, but experimental draw code also uses raw WGL pbuffer extension calls.
- Tools are still immediate-mode OpenGL heavy. A renderer backend replacement can isolate game runtime first, then treat Radiant/GUI/material tools as a separate migration.
