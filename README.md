# NIR Material Editor

Desktop editor for creating and editing 2D materials with inhomogeneities.

This repository uses Qt 6 (Widgets) and CMake. Stage 01 provides the basic
application skeleton:

- Main window with central editor view (QGraphicsView)
- Left dock with object tree (QTreeView)
- Top toolbar with placeholder tool actions

## Requirements

- CMake >= 3.16
- C++17 compiler (GCC, Clang, or MSVC)
- Qt 6 with Widgets module

## Configure and Build

### Linux

Install Qt 6 (example for Ubuntu/Debian):

```bash
sudo apt update
sudo apt install qt6-base-dev
```

Configure and build:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

Run:

```bash
./build/NIRMaterialEditor
```

### macOS

With Homebrew:

```bash
brew install qt
cmake -S . -B build -DCMAKE_PREFIX_PATH="$(brew --prefix qt)" -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
open build/NIRMaterialEditor.app
```

### Windows

1. Install Qt 6 using the Qt Online Installer (choose the toolchain matching your compiler, e.g. MSVC or MinGW).
2. Configure with CMake, pointing CMAKE_PREFIX_PATH to your Qt installation:

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:/Qt/6.x.x/msvcXXXX_64" -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Executable will be in `build/Release/` (MSVC) or `build/` (MinGW).

## Notes

- High-DPI scaling is handled by Qt 6 by default.
- Icons are embedded via Qt Resource System (`resources/app.qrc`).
- Next stages will add the substrate, tools, object tree model, and JSON I/O.


