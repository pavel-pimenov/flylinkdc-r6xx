#!/bin/bash
set -e

echo "=== Установка необходимых пакетов ==="
# sudo apt update
# sudo apt install -y cmake mingw-w64

echo "=== Создание директории сборки ==="
mkdir -p build-win
cd build-win

echo "=== Запуск CMake с тулчейном MinGW ==="
cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/Toolchain-mingw64.cmake ..

echo "=== Сборка проекта ==="
make -j$(nproc)

echo "=== Готово ==="
echo "Исполняемый файл: $(pwd)/flylinkdc.exe"