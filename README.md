# Noire Suite

[![CI](https://github.com/alexguirre/noire-suite/workflows/CI/badge.svg)](https://github.com/alexguirre/noire-suite/actions?workflow=CI)

Modding tools for L.A. Noire.

## Quick Start

Prerequisites:

- Windows 10
- Visual Studio 2019
- Git
- CMake 3.12.4 or newer

1. Enter the [Visual Studio development environment](https://docs.microsoft.com/en-us/cpp/build/building-on-the-command-line?view=vs-2019) to get access to the CMake version included with Visual Studio.

1. Install [vcpkg](https://github.com/Microsoft/vcpkg):

    ```console
    > git clone https://github.com/microsoft/vcpkg.git
    > cd vcpkg
    > git checkout 2019.09
    > .\bootstrap-vcpkg.bat
    ```

1. Install the dependencies:

    ```console
    > .\vcpkg --triplet x86-windows-static install ms-gsl doctest wxwidgets
    ```

1. Run CMake:

    ```console
    > mkdir src\build
    > cd src\build
    > cmake -A Win32 -DVCPKG_TARGET_TRIPLET=x86-windows-static -DCMAKE_TOOLCHAIN_FILE="../../vcpkg/scripts/buildsystems/vcpkg.cmake" ..
    ```

1. Open `src\build\noire.sln` in Visual Studio or build directly with CMake:

    ```console
    > cmake --build . --config Release
    ```

1. Compiled binaries will be in `src\build\bin\` and libraries in `src\build\lib\`.
