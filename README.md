# Legacy Notepad

A lightweight, 25x fast, Windows notepad alternative built with C++17 and Win32 API which I made because microsoft wont stop adding AI bloatware to notepad.exe.

<img width="752" height="242" alt="image" src="https://github.com/user-attachments/assets/bca18796-b088-4488-a445-649a549ddace" />
<img width="738" height="243" alt="image" src="https://github.com/user-attachments/assets/e9ba9361-d1ea-41ee-a18d-f38fb9bdb546" />

## Features

- **Multi-encoding text**: UTF-8, UTF-8 BOM, UTF-16 LE/BE, ANSI with line-ending selection.
- **Rich editing**: word wrap toggle, font selection, zoom, time/date stamp, find/replace/goto.
- **Backgrounds**: optional image with tile/stretch/fit/fill/anchor modes and opacity control. (known issues)
- **Printing**: print and page setup dialogs.

## Requirements

- Windows 10/11 with Win32 and GDI+ (desktop apps)
- CMake 3.16+
- A C++17 toolchain: MSVC (Visual Studio 2022+) or MinGW-w64 (tested with GCC 13)

## Build & Run

```bash
cd legacy-notepad
mkdir build && cd build
cmake ..
mingw32-make -j4   # or: cmake --build . --config Release
.\legacy-notepad.exe
```

## Architecture (concise)

- **Entry**: `src/main.cpp` — window class, message loop, wiring modules.
- **Core**: `src/core` — shared types and globals.
- **Modules** (`src/modules`): `theme` (dark mode), `editor` (RichEdit handling), `file` (I/O & encodings), `ui` (title/status/layout), `background` (GDI+), `dialog` (find/replace/font/transparency), `commands` (menu actions).
- **Resources**: `src/notepad.rc`, `src/resource.h`, icons/menus/accelerators.

## Repository tree

```
src/
  core/           # types, globals
  modules/        # theme, editor, file, ui, background, dialog, commands
  main.cpp
  notepad.rc
CMakeLists.txt
```

## File quicklook

| File/Folder | Purpose |
| --- | --- |
| `src/main.cpp` | Win32 entry point, WndProc, module wiring |
| `src/core/types.h` | Enums, structs, app constants |
| `src/core/globals.*` | Shared handles/state definitions |
| `src/modules/editor.*` | RichEdit setup, word wrap, zoom |
| `src/modules/file.*` | Load/save, encoding + line endings, recent list |
| `src/modules/ui.*` | Title/status updates, layout sizing |
| `src/modules/theme.*` | Dark mode title/menu/status, theming |
| `src/modules/background.*` | GDI+ background image/opacity/position |
| `src/modules/dialog.*` | Find/replace/goto, font, transparency dialogs |
| `src/modules/commands.*` | Menu command handlers |
| `src/notepad.rc`, `src/resource.h` | Menus, accelerators, icons |

## License

MIT License — see [LICENSE](LICENSE).

## Notes

- Windows-only (Win32 API + GDI+). Use Wine/Proton at your own risk.
- Background image modes may vary slightly across DPI/scaling settings.

## Queries

[x.com/forloopcodes](https://x.com/forloopcodes)
