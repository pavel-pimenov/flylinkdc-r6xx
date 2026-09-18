#!/bin/bash
set -euo pipefail

source_dir="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
build_dir="$source_dir/build-win"

cmake -S "$source_dir" -B "$build_dir" \
    -DCMAKE_TOOLCHAIN_FILE="$source_dir/cmake/Toolchain-mingw64.cmake" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SHARED_LIBS=OFF
cmake --build "$build_dir" --parallel "${JOBS:-$(nproc)}"
