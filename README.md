# Noire Suite

Modding tools for L.A. Noire.

## Quick Start

Prerequisites:

- Windows 10
- Visual Studio 2019
- Git
- CMake 3.12.4 or newer

1. Enter the [Visual Studio development environment](https://docs.microsoft.com/en-us/cpp/build/building-on-the-command-line?view=vs-2019) to get access to the CMake version included with Visual Studio.

1. Run CMake:

    ```console
    > mkdir src\build
    > cd src\build
    > cmake -A Win32 ..
    ```

1. Open `src\build\noire.sln` in Visual Studio or build directly with CMake:

    ```console
    > cmake --build . --config Release
    ```

1. Compiled binaries will be in `src\build\bin\` and libraries in `src\build\lib\`.
