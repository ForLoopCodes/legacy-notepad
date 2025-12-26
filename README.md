# Legacy Notepad

A lightweight, 50x fast, Windows notpad alternative built with C++ and Win32 API which I made because microsoft wont stop adding AI bloatware to notepad.exe.

<img width="752" height="242" alt="image" src="https://github.com/user-attachments/assets/bca18796-b088-4488-a445-649a549ddace" />
<img width="738" height="243" alt="image" src="https://github.com/user-attachments/assets/e9ba9361-d1ea-41ee-a18d-f38fb9bdb546" />


## Features

- **Fast & Lightweight** - 50x less memory consumption than real notepad.
- **Multiple Encodings** - UTF-8, UTF-8 BOM, UTF-16 LE, UTF-16 BE, ANSI
- **Customization**
  - Font selection and size adjustment
  - Window transparency
  - Background images with multiple positioning modes
  - Maximize button support
- **Drag & Drop** - Open files by dragging into the window

## Building

### Requirements

- Visual Studio 2022 (or later)
- CMake 3.16+
- Windows SDK 10.0+

### Compile

```bash
cd legacy-notepad
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

### Run

```bash
.\Release\legacy-notepad.exe
```

## Usage

**File Operations**

- `Ctrl+N` - New file
- `Ctrl+O` - Open file
- `Ctrl+S` - Save file
- `Ctrl+Shift+S` - Save as
- `Ctrl+P` - Print

**Editing**

- `Ctrl+Z` - Undo
- `Ctrl+Y` - Redo
- `Ctrl+X` - Cut
- `Ctrl+C` - Copy
- `Ctrl+V` - Paste
- `Ctrl+A` - Select all
- `Ctrl+Backspace` - Delete word backward
- `Ctrl+Delete` - Delete word forward

**Finding**

- `Ctrl+F` - Find
- `F3` - Find next
- `Shift+F3` - Find previous
- `Ctrl+H` - Replace

**Navigation**

- `Ctrl+G` - Go to line

**View**

- `Ctrl++` - Zoom in
- `Ctrl+-` - Zoom out
- `Ctrl+0` - Reset zoom

**Other**

- `F5` - Insert date/time
- `Alt+F4` - Exit

## Architecture

- **Core**: Win32 API windowing and message handling
- **Graphics**: GDI+ for background image rendering
- **Text Rendering**: Native Windows Edit Control with RichEdit enhancements
- **Resources**: RC file for menu, accelerators, and icons
- **Encoding**: Full support for multiple text encodings and line endings

## License

MIT License - see [LICENSE](LICENSE) file for details

## Contributing

Feel free to fork, modify, and improve. Submit issues or pull requests with enhancements.

## Notes

- **Windows Only** - Uses Win32 API, not compatible with Linux/Mac without major refactoring
- **Performance** - Optimized for fast startup and low memory usage
- **Stability** - Handles large files efficiently with streaming where applicable

## Queries

[x.com/forloopcodes](https://x.com/forloopcodes)
