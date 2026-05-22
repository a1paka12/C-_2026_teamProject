#!/usr/bin/env bash
# MSYS2 "UCRT64" 터미널에서:  bash build-snake.sh   또는   ./build-snake.sh
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")"

if [[ -x /ucrt64/bin/g++ ]]; then
  GXX=/ucrt64/bin/g++
  INC=/ucrt64/include/ncursesw
  LIB=/ucrt64/lib
elif [[ -n "${MSYSTEM_PREFIX:-}" && -x "${MSYSTEM_PREFIX}/bin/g++" ]]; then
  GXX="${MSYSTEM_PREFIX}/bin/g++"
  INC="${MSYSTEM_PREFIX}/include/ncursesw"
  LIB="${MSYSTEM_PREFIX}/lib"
else
  echo "이 스크립트는 MSYS2 UCRT64 터미널에서 실행하세요." >&2
  exit 1
fi

echo "== $(pwd) =="
ls
echo

pacman -S --needed --noconfirm mingw-w64-ucrt-x86_64-ncurses || true

echo "Building snake.exe..."
"$GXX" -std=c++17 -Wall -g -Iinclude -I"$INC" \
  src/main.cpp src/Map.cpp src/RedWallProjectile.cpp src/Gate.cpp src/Snake.cpp src/ItemSnake.cpp \
  src/Item.cpp src/ScoreBoard.cpp src/Zone.cpp \
  -L"$LIB" -lncursesw -o snake.exe

echo "Running ./snake.exe ..."
exec ./snake.exe
