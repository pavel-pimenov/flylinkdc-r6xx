#!/bin/bash
set -e

echo "=== Кросс-компиляция FlylinkDC++ с использованием MXE ==="

# Проверка наличия MXE
if [ -z "$MXE_DIR" ]; then
    MXE_DIR="/opt/mxe"
    if [ ! -d "$MXE_DIR" ]; then
        echo "MXE не найден в $MXE_DIR. Установите MXE или задайте переменную MXE_DIR."
        echo "Инструкция по установке MXE: https://mxe.cc/"
        exit 1
    fi
fi

# Путь к компилятору
export PATH="$MXE_DIR/usr/bin:$PATH"
export CMAKE_TOOLCHAIN_FILE="$MXE_DIR/usr/x86_64-w64-mingw32.static/share/cmake/mxe-conf.cmake"

echo "Используется MXE из $MXE_DIR"
echo "Компилятор: $(which x86_64-w64-mingw32.static-g++)"

mkdir -p build-mxe
cd build-mxe

cmake -DCMAKE_TOOLCHAIN_FILE="$CMAKE_TOOLCHAIN_FILE" ..
make -j$(nproc)

echo "=== Сборка завершена ==="
echo "Исполняемый файл: $(pwd)/flylinkdc.exe"