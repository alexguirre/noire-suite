mkdir src\build
cd src\build
cmake -A Win32 -DVCPKG_TARGET_TRIPLET=x86-windows-static -DCMAKE_TOOLCHAIN_FILE="../../vcpkg/scripts/buildsystems/vcpkg.cmake" ..
cd ..\..
