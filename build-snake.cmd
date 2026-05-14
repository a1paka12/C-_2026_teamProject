@echo off
setlocal EnableExtensions

rem MSYS2 설치 경로 (다르면 여기만 수정)
set "MSYS2_ROOT=C:\msys64"

set "PROJ=%~dp0"
if "%PROJ:~-1%"=="\" set "PROJ=%PROJ:~0,-1%"

cd /d "%PROJ%"
echo Current directory:
cd
echo.
echo Directory listing:
dir
echo.

if not exist "%MSYS2_ROOT%\ucrt64\bin\g++.exe" (
  echo [ERROR] g++ not found at "%MSYS2_ROOT%\ucrt64\bin\g++.exe"
  echo Edit MSYS2_ROOT at the top of this script to your MSYS2 folder.
  exit /b 1
)

echo Installing ncurses if needed (MSYS2 pacman)...
"%MSYS2_ROOT%\usr\bin\bash.exe" -lc "pacman -S --needed --noconfirm mingw-w64-ucrt-x86_64-ncurses"
if errorlevel 1 (
  echo [WARN] pacman failed. If ncurses is already installed, you can ignore this and try build anyway.
)
echo.

echo Building snake.exe...
"%MSYS2_ROOT%\ucrt64\bin\g++.exe" -std=c++17 -Wall -g -Iinclude -I"%MSYS2_ROOT%\ucrt64\include\ncursesw" ^
  src\main.cpp src\Map.cpp src\RedWallProjectile.cpp src\Gate.cpp src\Snake.cpp src\ItemSnake.cpp ^
  src\Item.cpp src\ScoreBoard.cpp ^
  -L"%MSYS2_ROOT%\ucrt64\lib" -lncursesw -o snake.exe
if errorlevel 1 (
  echo [ERROR] Build failed.
  exit /b 1
)

echo.
echo Running snake.exe...
"%PROJ%\snake.exe"
exit /b %errorlevel%
