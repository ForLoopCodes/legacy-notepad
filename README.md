# Legacy Notepad

A lightweight, 25x fast, Windows notepad alternative built with C++17 and Win32 API which I made because microsoft wont stop adding AI bloatware to notepad.exe.

<img width="752" height="242" alt="image" src="https://github.com/user-attachments/assets/bca18796-b088-4488-a445-649a549ddace" />
<img width="738" height="243" alt="image" src="https://github.com/user-attachments/assets/e9ba9361-d1ea-41ee-a18d-f38fb9bdb546" />

## Screenshot:

<img width="986" height="735" alt="image" src="https://github.com/user-attachments/assets/939cbb8b-a770-449c-9d9d-e93e076787d3" />

## Features

- **Multi-language support**: English and Japanese translations with runtime switching.
- **Multi-encoding text**: UTF-8, UTF-8 BOM, UTF-16 LE/BE, ANSI with line-ending selection.
- **Rich editing**: word wrap toggle, font selection, zoom, time/date stamp, find/replace/goto.
- **Backgrounds**: optional image with tile/stretch/fit/fill/anchor modes and opacity control. (known issues)
- **Always on top**: window pinning support.
- **Printing**: print and page setup dialogs.
- **Customizable icon**: change the application icon to any .ico file, including classic Notepad icons.

## Requisites

Building Legacy Notepad requires:

- **Operating System**: Windows 7 SP1 or newer (Windows 10/11 recommended for full UI feature support).
- **Build System**: [CMake](https://cmake.org/download/) 3.16 or higher.
- **Compiler**: A C++17 compatible toolchain:
  - **MinGW-w64**: Tested with GCC 13.2.0 (standard for lightweight builds).
  - **MSVC**: Visual Studio 2022 or higher.
- **SDKs**: Windows SDK (included with Visual Studio or available via MSYS2 for MinGW).
- **Dependencies**: The app uses standard Windows components: `GDI+`, `RichEdit 4.1` (`Msftedit.dll`), `Common Controls`, and `DWM`.

## Build & Installation

### Using MinGW (Recommended for GCC)

Ensure your MinGW `bin` folder is in your system `PATH`.

```powershell
# 1. Clone the repository
git clone https://github.com/ForLoopCodes/legacy-notepad.git
cd legacy-notepad

# 2. Create and enter build directory
mkdir build
cd build

# 3. Configure and build
cmake -G "MinGW Makefiles" ..
mingw32-make -j$(nproc)

# 4. Run the application
.\legacy-notepad.exe
```

### Using Visual Studio (MSVC)

1. Open the folder in Visual Studio 2022.
2. Visual Studio will automatically detect `CMakeLists.txt` and configure the project.
3. Select `legacy-notepad.exe` in the startup item dropdown.
4. Press `F5` to build and run.

Alternatively, via command line:

```powershell
cmake -B build -S .
cmake --build build --config Release
```

## Architecture

- **Entry**: `src/main.cpp` - window class, message loop, wiring modules.
- **Core**: `src/core` - shared types and globals.
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

| File/Folder                        | Purpose                                         |
| ---------------------------------- | ----------------------------------------------- |
| `src/main.cpp`                     | Win32 entry point, WndProc, module wiring       |
| `src/core/types.h`                 | Enums, structs, app constants                   |
| `src/core/globals.*`               | Shared handles/state definitions                |
| `src/modules/editor.*`             | RichEdit setup, word wrap, zoom                 |
| `src/modules/file.*`               | Load/save, encoding + line endings, recent list |
| `src/modules/ui.*`                 | Title/status updates, layout sizing             |
| `src/modules/theme.*`              | Dark mode title/menu/status, theming            |
| `src/modules/background.*`         | GDI+ background image/opacity/position          |
| `src/modules/dialog.*`             | Find/replace/goto, font, transparency dialogs   |
| `src/modules/commands.*`           | Menu command handlers                           |
| `src/notepad.rc`, `src/resource.h` | Menus, accelerators, icons                      |

## License

MIT License - see [LICENSE](LICENSE).

## Notes

- Windows-only (Win32 API + GDI+). Use Wine/Proton at your own risk.
- Background image modes may vary slightly across DPI/scaling settings.

## Queries

[x.com/forloopcodes](https://x.com/forloopcodes)
