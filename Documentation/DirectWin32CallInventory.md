# Direct Win32 Call Inventory

Date: 2026-07-13

This inventory records direct Windows API and Winsock call surfaces in the current tree. It focuses on engine-owned calls in `neo/sys/win32`, game/framework calls that pierce the platform abstraction, renderer dynamic-library calls, and editor/tool code that talks directly to Win32 or MFC-hosted Win32 controls.

The scan used a curated API-name list for function-call expressions. It intentionally does not count every Win32 type (`HWND`, `HDC`, `HANDLE`, etc.) or resource macro because this file is about call sites.

## Main Platform Layer

| File | Surface | Direct dependencies |
| --- | --- | --- |
| `neo/sys/win32/win_main.cpp` | Windows process entry, message pump, window lifecycle, startup/shutdown | `RegisterClassEx`, `CreateWindowEx`, `PeekMessage`, `GetMessage`, `TranslateMessage`, `DispatchMessage`, `DefWindowProc`, `GetModuleFileName`, `Sleep`, `MessageBox`. |
| `neo/sys/win32/win_wndproc.cpp` | Main window procedure and input dispatch | `DefWindowProc`, cursor/window message handling, Win32 message constants. |
| `neo/sys/win32/win_glimp.cpp` | GL window/context/display mode | `ChoosePixelFormat`, `SetPixelFormat`, `SwapBuffers`, WGL context calls, `ChangeDisplaySettings`, `GetDC`, `ReleaseDC`, `SetWindowPos`, `ShowWindow`. |
| `neo/sys/win32/win_qgl.cpp` | OpenGL dynamic binding | `LoadLibrary`, `FreeLibrary`, `GetProcAddress`, `wglGetProcAddress`. |
| `neo/sys/win32/win_net.cpp` | Winsock platform backend | `WSAStartup`, `WSACleanup`, `WSAGetLastError`, `socket`, `bind`, `sendto`, `recvfrom`, `ioctlsocket`, `closesocket`, `gethostname`, `gethostbyname`. |
| `neo/sys/win32/win_shared.cpp` | Filesystem, timing, clipboard/process helpers | File/time/process APIs such as current-directory, file attributes, and system timing helpers. |
| `neo/sys/win32/win_syscon.cpp` | Dedicated/server console window | Window/control creation, `SendMessage`, `SetWindowText`, `ShowWindow`, message loop, console text update. |
| `neo/sys/win32/win_input.cpp` | Keyboard/mouse/DirectInput input | DirectInput creation/acquisition, cursor clipping/visibility, focus/capture APIs. |
| `neo/sys/win32/win_snd.cpp` | Legacy Win32 sound bootstrap | Windows multimedia/direct sound integration points. |
| `neo/sys/win32/win_gamma.cpp` | Display gamma | Win32 device-context and gamma-ramp calls. |
| `neo/sys/win32/win_cpu.cpp` | CPU/timer/platform probing | Win32 timing and CPU feature helper calls. |

## Non-Platform Runtime Piercing Calls

| File | Calls | Notes |
| --- | ---: | --- |
| `neo/renderer/cg_explicit.cpp` | 123 | Dynamic Cg library binding through `LoadLibrary`/`GetProcAddress` style calls. This is renderer-owned but platform-specific. |
| `neo/framework/async/AsyncClient.cpp` | 28 | Network serialization path uses direct socket-ish and message helper names in addition to the platform network layer. |
| `neo/framework/FileSystem.cpp` | 24 | Contains direct Windows file API calls in shared framework code. |
| `neo/framework/Session.cpp` | 10 | Uses Windows-facing UI/file helpers from outside `neo/sys/win32`. |
| `neo/framework/Session_menu.cpp` | 13 | Save-game UI/file menu handling with direct Windows-style helper calls. |

## Tool And Editor Win32 Surface

| Area | Representative files | Notes |
| --- | --- | --- |
| Debugger | `neo/tools/debugger/DebuggerWindow.cpp`, `DebuggerQuickWatchDlg.cpp`, `DebuggerServer.cpp`, `DebuggerClient.cpp`, `debugger.cpp` | Heaviest direct Win32/MFC UI surface; message sending, window text, focus, process/debug message plumbing. |
| Radiant editor | `EntityDlg.cpp`, `MainFrm.cpp`, `XYWnd.cpp`, `PropertyList.cpp`, `EditViewDlg.cpp`, `CamWnd.cpp`, `PreviewDlg.cpp`, `NewTexWnd.cpp`, `ZWnd.cpp`, `ConsoleDlg.cpp`, `Radiant.cpp`, `WIN_DLG.CPP`, `WIN_QE3.CPP`, `GLWidget.cpp`, `TabsDlg.cpp`, `DialogTextures.cpp` | Editor windows, panes, dialogs, OpenGL view windows, message dispatch, focus/capture, and file dialogs. |
| GUI editor | `GEApp.cpp`, `GEItemPropsDlg.cpp`, `GEViewer.cpp`, `GEWorkspace.cpp`, `GEItemScriptsDlg.cpp`, `GETransformer.cpp`, `GENavigator.cpp`, `GEWorkspaceFile.cpp`, `GEProperties.cpp` | Win32/MFC-hosted GUI editing surfaces and canvas/view windows. |
| Shared tool widgets | `neo/tools/common/RollupPanel.cpp`, `PropertyGrid.cpp`, `PropTree/*`, `OpenFileDialog.cpp`, `AlphaPopup.cpp` | Reusable Win32 control wrappers. |
| Decl/material/AF/script/sound/particle tools | `neo/tools/decl/*`, `neo/tools/materialeditor/*`, `neo/tools/af/*`, `neo/tools/script/DialogScriptEditor.cpp`, `neo/tools/sound/DialogSound.cpp`, `neo/tools/particle/DialogParticleEditor.cpp` | MFC dialogs and custom controls. |

## Highest-Volume Direct API Calls

| API | Count |
| --- | ---: |
| `SendMessage` | 281 |
| `SetWindowText` | 180 |
| `ShowWindow` | 177 |
| `MessageBox` | 146 |
| `GetProcAddress` | 124 |
| `GetClientRect` | 119 |
| `SetWindowPos` | 94 |
| `GetWindowRect` | 87 |
| `SetFocus` | 84 |
| `GetWindowText` | 83 |
| `MoveWindow` | 82 |
| `ReadFile` | 59 |
| `SetCursor` | 45 |
| `GetDC` | 41 |
| `DestroyWindow` | 33 |
| `ReleaseDC` | 30 |
| `UpdateWindow` | 29 |
| `SetCapture` | 25 |
| `ReleaseCapture` | 23 |
| `DefWindowProc` | 22 |
| `InvalidateRect` | 22 |
| `CreateWindow` | 21 |
| `CopyFile` | 20 |
| `OutputDebugString` | 16 |
| `WriteFile` | 16 |
| `PeekMessage` | 15 |
| `PostMessage` | 15 |
| `WSAGetLastError` | 14 |
| `CreateWindowEx` | 13 |
| `SetTimer` | 13 |
| `LoadLibrary` | 11 |
| `GetLastError` | 11 |
| `RegisterClassEx` | 9 |
| `DeleteFile` | 9 |
| `GetModuleFileName` | 9 |
| `WaitForSingleObject` | 9 |
| `Sleep` | 8 |
| `ChoosePixelFormat` | 6 |
| `SetPixelFormat` | 6 |
| `wglMakeCurrent` | 5 |
| `timeGetTime` | 4 |
| `socket` | 4 |
| `CreateEvent` | 4 |

## Highest-Volume Files In The Scan

| File | Curated call count |
| --- | ---: |
| `neo/tools/debugger/DebuggerWindow.cpp` | 187 |
| `neo/renderer/cg_explicit.cpp` | 123 |
| `neo/tools/radiant/EntityDlg.cpp` | 113 |
| `neo/sys/win32/win_glimp.cpp` | 78 |
| `neo/sys/win32/win_net.cpp` | 63 |
| `neo/sys/win32/win_syscon.cpp` | 57 |
| `neo/tools/radiant/MainFrm.cpp` | 51 |
| `neo/tools/common/RollupPanel.cpp` | 47 |
| `neo/tools/guied/GEApp.cpp` | 47 |
| `neo/tools/guied/GEItemPropsDlg.cpp` | 45 |
| `neo/tools/radiant/XYWnd.cpp` | 45 |
| `neo/sys/win32/win_main.cpp` | 44 |
| `neo/tools/common/PropertyGrid.cpp` | 44 |
| `neo/tools/decl/DialogEntityDefEditor.cpp` | 41 |
| `neo/tools/decl/DialogDeclBrowser.cpp` | 39 |
| `neo/tools/radiant/PropertyList.cpp` | 38 |
| `neo/tools/guied/GEViewer.cpp` | 36 |
| `neo/tools/guied/GEWorkspace.cpp` | 35 |

## Scan Pattern

```powershell
$apis = @(
  'LoadLibrary','FreeLibrary','GetProcAddress','GetModuleHandle','GetModuleFileName',
  'MessageBox','CreateWindow','CreateWindowEx','DefWindowProc','PeekMessage',
  'GetMessage','TranslateMessage','DispatchMessage','SendMessage','PostMessage',
  'ShowWindow','SetWindowText','GetWindowText','GetClientRect','GetWindowRect',
  'GetDC','ReleaseDC','ChoosePixelFormat','SetPixelFormat','SwapBuffers',
  'wglMakeCurrent','wglGetProcAddress','CreateFile','ReadFile','WriteFile',
  'CloseHandle','CreateThread','WaitForSingleObject','CreateEvent','SetEvent',
  'Sleep','timeGetTime','WSAStartup','WSACleanup','WSAGetLastError','socket',
  'bind','send','sendto','recv','recvfrom','ioctlsocket','closesocket'
)
$rx = '\b(' + (($apis | ForEach-Object {[regex]::Escape($_)}) -join '|') + ')\s*\('
Get-ChildItem neo\sys\win32,neo\framework,neo\renderer,neo\tools -Recurse -Include *.c,*.cpp,*.h |
  Select-String -Pattern $rx
```

## Modernization Notes

- A platform abstraction pass should start with `neo/sys/win32`; most runtime OS behavior already belongs there.
- `neo/renderer/cg_explicit.cpp` is a separate dynamic-library abstraction problem, not a general OS-layer problem.
- Tool/editor code is deeply Win32/MFC-shaped and should be tracked separately from runtime portability.
