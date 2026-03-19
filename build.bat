@echo off
REM Usage: build [main] [build_type] [target_arch]
REM 
REM main, build_type and target_arch can appear in any order:
REM
REM - main: the benchmark/test to compile. A program entry point (main) is picked based on its value
REM   - Valid values:  1 of: benchmark_encode benchmark_decode test_encode test_decode
REM   - Default value: benchmark_encode
REM
REM - build_type: the type of build to perform
REM   - Valid values:  1 of: debug, dev, release
REM   - Default value: release
REM
REM - target_arch: the target CPU architecture to build for
REM   - Valid values:  1 of: x64, arm64
REM   - Default value: the architecture of the CPU in use
REM
REM "build" is the same as "build benchmark_decode release x64"

setlocal EnableDelayedExpansion
pushd %~dp0

REM ==== Files & directories ====
REM If known, the path to VsDevCmd.bat can be directly set to skip calling vswhere.exe
set vsdevcmd="%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat"
set vswhere_exe="%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

set build_dir=build
set obj_dir_name=obj

REM Logs sources
set logs_sources=logs\*.c

REM Project sources
set main=benchmark_encode.c
set sources=..\nucleotide_2bit_codec\*.c

REM Compilation options
set msvc_base_comp_flags=/arch:AVX2 /c /DLOGS_ENABLED=1 /DNOMINMAX /DWIN32_LEAN_AND_MEAN /GS- /Gs2147483647 /nologo /std:c11 /W4 /WX
set msvc_debug_comp_flags=/Od /Zi
set msvc_dev_comp_flags=/O2 /Zi
set msvc_release_comp_flags=/O2

REM Link options
set link_flags=/entry:program_start /nodefaultlib /nologo /subsystem:console
set debug_link_flags=/debug /profile
set dev_link_flags=/debug /profile
set release_link_flags=/emittoolversioninfo:no /fixed /NOCOFFGRPINFO /incremental:no /opt:ref

set libs=Kernel32.lib


REM ==== Command line processing ====
REM Turn command line arguments into variables
for %%a in (%*) do set %%a=1

REM Determine build type
set config_msg=
if [%debug%] equ [1] (
  set config_msg=Compiling a debug build
  set build_dir=%build_dir%\debug
  set comp_flags=%msvc_base_comp_flags% %msvc_debug_comp_flags%
  set link_flags=%link_flags% %debug_link_flags%
) else if [%dev%] equ [1] (
  set config_msg=Compiling a development build
  set build_dir=%build_dir%\dev
  set comp_flags=%msvc_base_comp_flags% %msvc_dev_comp_flags%
  set link_flags=%link_flags% %dev_link_flags%
) else (
  set config_msg=Compiling a release build
  set build_dir=%build_dir%\release
  set comp_flags=%msvc_base_comp_flags% %msvc_release_comp_flags%
  set link_flags=%link_flags% %release_link_flags%
)

set obj_dir=!build_dir!\%obj_dir_name%

REM Pick what to build:
for /f "delims=" %%i in ('dir /b *.c') do (
  set main_c=%%i
  set main_name=!main_c:.c=!
  call set arg_name=%%!main_name!%%

  REM The name of one of the tools can be received as an argument through the command line, which
  REM makes it a variable with the same name as its .c file
  if [!arg_name!] equ [1] (
    set main=%%i
    goto :main_found
  )
)

:main_found

set exe_name=!main:.c=!
set config_msg=!config_msg! of !exe_name!

REM Determine host & target architecture
if [%PROCESSOR_ARCHITECTURE%] equ [AMD64] (
  set host_arch=x64
) else if [%PROCESSOR_ARCHITECTURE%] equ [ARM64] (
  set host_arch=arm64
) else (
  REM 32-bit ARM or x86, Itanium, etc.
  echo Error: the "%PROCESSOR_ARCHITECTURE%" architecture isn't supported. Exiting
  exit /b 1
)

if [!x64!] neq [] (
  set target_arch=x64
) else if [!arm64!] neq [] (
  set target_arch=arm64
) else if [!host_arch!] neq [] (
  set target_arch=!host_arch!
) else (
  echo No supported target architecture was passed ^(x64 or arm64^), and the host architecture ^(%PROCESSOR_ARCHITECTURE%^) cannot be used as a fallback because it isn't supported
)

set config_msg=!config_msg! for !target_arch!
echo !config_msg!


REM ==== Compile ====
REM Setup compilation environment
if [%VSCMD_VER%] equ [] (
  call :setup_env
) else if [%VSCMD_ARG_TGT_ARCH%] neq [!target_arch!] (
  call :setup_env
)

REM Create both the build and OBJ directories
if not exist !obj_dir! mkdir !obj_dir!

REM Compile logs C sources
if not exist !obj_dir!\logs_compiled (
  echo( && echo ^> Compiling logs C sources
  call cl.exe !comp_flags! /Fo!obj_dir!\ %logs_sources% || exit /b 1
  echo 1 > !obj_dir!\logs_compiled
)

REM Compile project C sources
echo( && echo ^> Compiling project C sources
call cl.exe !comp_flags! /Fo!obj_dir!\ %sources% || exit /b 1

REM The source file containing the program entry point is compiled separetely into a "main" OBJ file
REM to prevent collisions with other program entry points that have already been compiled
call cl.exe !comp_flags! /Fo!obj_dir!\main.obj !main! || exit /b 1

REM Link all compiled sources together
echo( && echo ^> Linking compiled sources together
set exe_file=!exe_name!.exe
call link.exe !link_flags! /out:%build_dir%\%exe_file% !obj_dir!\*.obj %libs% || exit /b 1
echo Executable successfully created: %build_dir%\%exe_file%

taskkill /F /T /IM vctip.exe > nul
exit /b 0


REM ===== Functions =====
REM Finds and calls VsDevCmd.bat
:setup_env
  if not exist %vsdevcmd% (
    if not exist %vswhere_exe% (
      echo Error: could not find vswhere.exe
      exit /b 1
    )

    REM Find the latest version of Visual Studio Build Tools
    set find_vs_install_dir_cmd=%vswhere_exe% -latest                                             ^
                                              -products Microsoft.VisualStudio.Product.BuildTools ^
                                              -property installationPath

    for /f "tokens=*" %%i in ('!find_vs_install_dir_cmd!') do set vs_install_dir=%%i

    if not exist !vs_install_dir! (
      REM Find the latest version of Visual Studio Native Desktop workload instead
      set find_vs_install_dir_cmd=%vswhere_exe% -latest                                                 ^
                                                -requires Microsoft.VisualStudio.Workload.NativeDesktop ^
                                                -property installationPath

      for /f "tokens=*" %%i in ('!find_vs_install_dir_cmd!') do set vs_install_dir=%%i

      if not exist !vs_install_dir! (
        echo Error: could not find Visual Studio Build Tools nor the Visual Studio Native Desktop workload
        exit /b 1
      )
    )

    set vsdevcmd="!vs_install_dir!\Common7\Tools\VsDevCmd.bat"
    if not exist !vsdevcmd! (
      echo Error: could not find VsDevCmd.bat
      exit /b 1
    )
  )

  set VSCMD_SKIP_SENDTELEMETRY=1
  set __VSCMD_ARG_NO_LOGO=1
  set __VSCMD_ARG_TGT_ARCH=!target_arch!
  set __VSCMD_ARG_host_arch=!host_arch!
  call !vsdevcmd!

  goto :eof