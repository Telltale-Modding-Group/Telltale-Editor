# Telltale Editor

![Telltale Editor](https://github.com/Telltale-Modding-Group/Telltale-Editor/actions/workflows/cmake-multi-platform.yml/badge.svg)

This project is an all in one modding application for all games made in the Telltale Tool by Telltale Games. For information about how it works, what you can do with it and more, see the [wiki](https://github.com/Telltale-Modding-Group/Telltale-Editor/wiki).
This can be built for Windows, MacOS and Linux.

## How to build

This project uses CMake and to build everything you can use the build scripts. Simply run them passing in 'release' as the argument on your machine to build.

### Prerequisites
- [Git](https://git-scm.com/downloads)
- [CMake](https://cmake.org/download/)

#### For windows
- [Visual Studio 2019 or 2022](https://visualstudio.microsoft.com/downloads/) with:
  - Desktop development with C++ workload
  - MSVC v143 - VS 2022 C++ x64/x86 build tools
  - Windows 11 SDK (10.0.26100.4654)
  - CMake tools for Visual Studio

### Steps
1. Clone Repository
```
git clone https://github.com/Telltale-Modding-Group/Telltale-Editor.git
```
2. Update Submodules
```
cd Telltale-Editor
```
```
git submodule update --init --recursive
```
3. Build Using Script
```
cd Scripts/Builders
```
```
build_windows.bat
```

## Authors

This project was made possible by lots of work done by various people. 

All C++ and C implementation, and Lua Classes:
#### [Lucas Saragosa](https://github.com/LucasSaragosa)

Lua classes and CMake build system as well as UI development:
#### [Ivan ('DarkShadow')](https://github.com/iMrShadow)

Initial testing help and github workflows
#### [Asil ('Proton')](https://github.com/asilz)

Support and future help
#### [David M.](https://github.com/frostbone25)
