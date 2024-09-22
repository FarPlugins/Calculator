@echo off

mkdir bin

if exist "%VS170COMNTOOLS%..\..\VC\Auxiliary\Build\vcvarsall.bat" (
  call "%VS170COMNTOOLS%..\..\VC\Auxiliary\Build\vcvarsall.bat" x86
) else (
  call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x86
)

mkdir build86
cd build86
cmake .. -DCMAKE_INSTALL_PREFIX=../bin/x86 -DCMAKE_BUILD_TYPE=Release -DCALC_BUILD_ARCH=x86 -G "NMake Makefiles"
cmake --build .
cpack
copy *.zip ..\bin

if errorlevel 1 goto end

cd ..
if exist "%VS170COMNTOOLS%..\..\VC\Auxiliary\Build\vcvarsall.bat" (
  call "%VS170COMNTOOLS%..\..\VC\Auxiliary\Build\vcvarsall.bat" x64
) else (
  call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
)
mkdir build64
cd build64
cmake .. -DCMAKE_INSTALL_PREFIX=../bin/x64 -DCMAKE_BUILD_TYPE=Release -DCALC_BUILD_ARCH=x64 -G "NMake Makefiles"
cmake --build .
cpack
copy *.zip ..\bin


cd ..
if exist "%VS170COMNTOOLS%..\..\VC\Auxiliary\Build\vcvarsall.bat" (
  call "%VS170COMNTOOLS%..\..\VC\Auxiliary\Build\vcvarsall.bat" amd64_arm
) else (
  call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64_arm
)
mkdir build_arm64
cd build_arm64
cmake .. -DCMAKE_INSTALL_PREFIX=../bin/arm64 -DCMAKE_BUILD_TYPE=Release -DCALC_BUILD_ARCH=arm64 -G "NMake Makefiles"
cmake --build .
cpack
copy *.zip ..\bin

:end
echo See result in directory "bin"
