@echo off
rem Builds version.dll (ASI loader) and the per-game trainer .asi files into .\build
setlocal
cd /d "%~dp0"
rem Find any Visual Studio / Build Tools install that has the C++ x64 compiler
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (echo Visual Studio Build Tools not found. Install them with the C++ workload. & exit /b 1)
for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSDIR=%%i"
if not defined VSDIR (echo No Visual Studio install with the C++ x64 tools was found. & exit /b 1)
call "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" >nul || exit /b 1
if not exist third_party\imgui\imgui.cpp (echo third_party is empty: clone with --recursive or run "git submodule update --init". & exit /b 1)
if not exist build mkdir build
if not exist build\obj mkdir build\obj

set IMGUI=third_party\imgui
set MH=third_party\minhook
set CFLAGS=/nologo /O2 /MT /EHsc /std:c++17 /utf-8 /W3 /DNOMINMAX /DUNICODE /D_UNICODE /I%IMGUI% /I%IMGUI%\backends /I%MH%\include
set LIBS=user32.lib gdi32.lib d3d11.lib d3d12.lib dxgi.lib d3dcompiler.lib

rem ---- ASI loader (version.dll) ----
ml64 /nologo /c /Fobuild\obj\version_stubs.obj src\loader\version_stubs.asm || exit /b 1
cl %CFLAGS% /c /Fobuild\obj\version_proxy.obj src\loader\version_proxy.cpp || exit /b 1
link /nologo /DLL /DEF:src\loader\version.def /OUT:build\version.dll build\obj\version_proxy.obj build\obj\version_stubs.obj kernel32.lib || exit /b 1

rem ---- shared code (ImGui, MinHook, core) ----
set SHARED=%IMGUI%\imgui.cpp %IMGUI%\imgui_draw.cpp %IMGUI%\imgui_tables.cpp %IMGUI%\imgui_widgets.cpp ^
 %IMGUI%\backends\imgui_impl_win32.cpp %IMGUI%\backends\imgui_impl_dx11.cpp %IMGUI%\backends\imgui_impl_dx12.cpp ^
 src\core\log.cpp src\core\gamethread.cpp src\core\menu.cpp src\core\render.cpp src\core\common.cpp
cl %CFLAGS% /c /Fobuild\obj\ %SHARED% || exit /b 1
cl /nologo /O2 /MT /W0 /c /Fobuild\obj\ %MH%\src\buffer.c %MH%\src\hook.c %MH%\src\trampoline.c %MH%\src\hde\hde64.c || exit /b 1
set SHARED_OBJ=build\obj\imgui.obj build\obj\imgui_draw.obj build\obj\imgui_tables.obj build\obj\imgui_widgets.obj ^
 build\obj\imgui_impl_win32.obj build\obj\imgui_impl_dx11.obj build\obj\imgui_impl_dx12.obj ^
 build\obj\log.obj build\obj\gamethread.obj build\obj\menu.obj build\obj\render.obj build\obj\common.obj ^
 build\obj\buffer.obj build\obj\hook.obj build\obj\trampoline.obj build\obj\hde64.obj

rem ---- San Andreas ----
cl %CFLAGS% /c /Fobuild\obj\sa_trainer.obj src\sa\sa_trainer.cpp || exit /b 1
link /nologo /DLL /OUT:build\TrilogyTrainer.SA.asi build\obj\sa_trainer.obj %SHARED_OBJ% %LIBS% || exit /b 1

rem ---- Vice City and GTA III (same source, different game define) ----
cl %CFLAGS% /DGAME_VC /c /Fobuild\obj\vc_trainer.obj src\classic\classic_trainer.cpp || exit /b 1
link /nologo /DLL /OUT:build\TrilogyTrainer.VC.asi build\obj\vc_trainer.obj %SHARED_OBJ% %LIBS% || exit /b 1
cl %CFLAGS% /DGAME_III /c /Fobuild\obj\iii_trainer.obj src\classic\classic_trainer.cpp || exit /b 1
link /nologo /DLL /OUT:build\TrilogyTrainer.III.asi build\obj\iii_trainer.obj %SHARED_OBJ% %LIBS% || exit /b 1

echo Build OK
