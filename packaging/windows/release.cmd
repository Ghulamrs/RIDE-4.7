@echo off
rem The release build, Windows: every program compiled from a fresh git checkout, the installer last.
rem git is the source of record, so nothing of a working tree reaches a release; one run, one set of build times.
rem
rem   release.cmd [version]   -> %RELEASE_DIR%\<stamp>\RIDE-<ver>\dist\RIDE-<ver>-setup.exe and RELEASE.txt beside it
rem
rem What is final is each repository's default branch on GitHub (main or master), never a
rem side branch: VM6747's Compiler-Cppi is taken at the head of its own default branch, not at the commit pinned.
rem RELEASE_DIR (default %USERPROFILE%\ride-release).
setlocal enabledelayedexpansion
set "VER=%~1"
if "%VER%"=="" set "VER=4.7"
if "%RELEASE_DIR%"=="" set "RELEASE_DIR=%USERPROFILE%\ride-release"
for /f %%t in ('powershell -NoProfile -Command "Get-Date -Format yyyyMMdd-HHmmss"') do set "STAMP=%%t"
set "W=%RELEASE_DIR%\%STAMP%"
rem The submodule is named by an ssh URL; https needs no key, and every repository is public.
set "GIT=git -c url.https://github.com/.insteadOf=git@github.com: -c advice.detachedHead=false -c core.autocrlf=false"

mkdir "%W%" || exit /b 1
echo RIDE %VER% release in %W%
echo [1/3] Fresh checkouts from github.com/Ghulamrs
call :clone RIDE-%VER% RIDE-4.7 || exit /b 1
call :clone VM6747 VM6747 || exit /b 1
call :clone ASM6x ASM6x || exit /b 1
call :clone LNK6x LNK6X || exit /b 1
call :clone LINK LINK || exit /b 1
call :clone MASM MASM || exit /b 1
call :clone Converter-C2S Converter-C2S || exit /b 1
rem The submodule at the head of its default branch, which .gitmodules names, with the pin recorded beside it.
for /f "tokens=3" %%h in ('git -C "%W%\VM6747" ls-tree HEAD Compiler-Cppi') do set "PIN=%%h"
%GIT% -C "%W%\VM6747" submodule -q update --init --remote Compiler-Cppi || exit /b 1
for /f %%h in ('git -C "%W%\VM6747\Compiler-Cppi" rev-parse HEAD') do (echo   VM6747/Compiler-Cppi  %%h  default branch, VM6747 pins !PIN!& echo VM6747/Compiler-Cppi  %%h  default branch of Compiler-Cpp-Optimize; VM6747 pins !PIN!>> "%W%\RELEASE.txt.repos")
(echo RIDE %VER%, built %DATE% %TIME% on %COMPUTERNAME% ^(Windows^) from fresh checkouts& type "%W%\RELEASE.txt.repos") > "%W%\RELEASE.txt"
del "%W%\RELEASE.txt.repos"

rem The sources as sealed, before a line is compiled: a release of code that does not match its seal is refused (T1).
echo [1b] The seals - verify_seals.py, MASTER.SEAL down to every project
python -c "print(1)" >nul 2>&1
if errorlevel 1 (
  echo   WARNING: no Python on this machine, so the seals were NOT checked here - release.sh checks the same commits
) else (
  python "%W%\RIDE-%VER%\verify_seals.py" > "%W%\SEALS.txt" 2>&1
  if errorlevel 1 (type "%W%\SEALS.txt" & echo release.cmd: the sources do not match their seals - reseal, commit, push, run again & exit /b 1)
)

echo [2/3] Every program, then the installer - RIDE.sln, whose Installer project comes last
set "R=%W%\RIDE-%VER%"
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
rem Through a file, as build.bat does: for /f cannot run a quoted path that has a space in it.
"%VSWHERE%" -latest -products * -version "[17.0,18.0)" -property installationPath > "%TEMP%\ride-release-vspath.txt"
set "VSPATH="
set /p VSPATH=<"%TEMP%\ride-release-vspath.txt"
del "%TEMP%\ride-release-vspath.txt"
if "%VSPATH%"=="" (echo could not find Visual Studio 2022 & exit /b 1)
call "%VSPATH%\VC\Auxiliary\Build\vcvars64.bat" >nul
set "RIDE_NO_INSTALLER="
pushd "%R%"
msbuild RIDE.sln /p:Configuration=Release /p:Platform=x64 /p:OutDir=%R%\bin\ /v:minimal /m
set "RC=%errorlevel%"
popd
if not "%RC%"=="0" (echo BUILD FAILED & exit /b 1)
set "PRODUCT=%R%\dist\RIDE-%VER%-setup.exe"
if not exist "%PRODUCT%" (echo release.cmd: no %PRODUCT% & exit /b 1)

echo [3/3] Done
echo   installer : %PRODUCT%
echo   record    : %W%\RELEASE.txt
for %%f in ("%PRODUCT%") do echo installer %%~f  %%~tf>> "%W%\RELEASE.txt"
exit /b 0

:clone
rem dir  repository - cloned at its default branch, the one GitHub calls HEAD
%GIT% clone -q "https://github.com/Ghulamrs/%2.git" "%W%\%1" || exit /b 1
for /f %%b in ('git -C "%W%\%1" rev-parse --abbrev-ref HEAD') do set "BR=%%b"
for /f %%h in ('git -C "%W%\%1" rev-parse HEAD') do (echo   %1  %%h  !BR!& echo %1  %%h  !BR!  github.com/Ghulamrs/%2>> "%W%\RELEASE.txt.repos")
exit /b 0
