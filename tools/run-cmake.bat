mkdir src\build
cd src\build
mkdir x86
cd x86
cmake -A Win32 -DVCPKG_TARGET_TRIPLET=x86-windows-static -DCMAKE_TOOLCHAIN_FILE="../../../vcpkg/scripts/buildsystems/vcpkg.cmake"  ..\..
cd ..
mkdir x64
cd x64
cmake -A x64 -DCMAKE_CUDA_FLAGS="-arch=sm_52" ..\..
cd ..\..\..
