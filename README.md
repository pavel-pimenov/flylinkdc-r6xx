FlylinkDC++ r6xx
==============

* Special thanks https://pvs-studio.com
* http://www.softpedia.com/get/Internet/File-Sharing/FlylinkDC.shtml

## Быстрый старт (кросс-компиляция под Windows из Ubuntu)

```bash
# Установите компилятор и CMake
sudo apt update
sudo apt install cmake mingw-w64

# Клонируйте репозиторий (если ещё не)
git clone https://github.com/flylinkdc/flylinkdc-r6xx.git
cd flylinkdc-r6xx

# Создайте директорию сборки
mkdir build-win && cd build-win

# Запустите CMake с тулчейном для MinGW
cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/Toolchain-mingw64.cmake ..

# Соберите проект
make -j$(nproc)

# Готовый exe-файл
ls -la flylinkdc.exe
```

Если возникают ошибки из-за отсутствующих зависимостей, см. раздел «Сборка» ниже.

## Сборка

### Сборка с Visual Studio (Windows)

### Сборка с использованием MXE (рекомендуется для кросс-компиляции)

[MXE](https://mxe.cc/) — это набор скриптов для сборки кросс-компилятора и библиотек под Windows из-под Linux. После установки MXE сборка FlylinkDC++ становится простой.

1. Установите MXE согласно [официальной инструкции](https://mxe.cc/#tutorial).
2. Убедитесь, что переменная окружения `MXE_DIR` указывает на каталог MXE (обычно `/opt/mxe`).
3. Запустите скрипт:

```bash
chmod +x build-with-mxe.sh
./build-with-mxe.sh
```

Либо вручную:

```bash
export MXE_DIR=/opt/mxe
export PATH=$MXE_DIR/usr/bin:$PATH
mkdir build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=$MXE_DIR/usr/x86_64-w64-mingw32.static/share/cmake/mxe-conf.cmake ..
make
```

Скрипт автоматически использует статическую компоновку, все зависимости будут включены в исполняемый файл.

Откройте решение `FlylinkDC_2022.sln` в Visual Studio 2022 и соберите проект.

### Сборка с CMake (кросс-платформенная)

Добавлена поддержка сборки через CMake, что позволяет компилировать проект под Windows с использованием MinGW-w64 (кросс-компиляция из-под Linux) или нативно под Windows с помощью MSVC.

#### Требования

- CMake 3.12 или выше
- Компилятор: MSVC, MinGW-w64 или совместимый
- Зависимости:
  - Boost (header-only)
  - OpenSSL
  - libtorrent
  - zlib
  - bzip2
  - leveldb
  - lua
  - jsoncpp
  - maxminddb
  - MediaInfoLib
  - ZenLib

Зависимости могут быть собраны из исходников, находящихся в дереве проекта, или установлены системно.

#### Инструкция для кросс-компиляции под Windows из Ubuntu

1. Установите компилятор mingw-w64 и утилиты:

```bash
sudo apt update
sudo apt install mingw-w64 cmake make g++-mingw-w64-x86-64
```

Пакет `g++-mingw-w64-x86-64` предоставляет компилятор `x86_64-w64-mingw32-g++`. Также могут потребоваться библиотеки для кросс-компиляции зависимостей. Установите их:

```bash
sudo apt install libssl-dev libboost-all-dev libtorrent-rasterbar-dev zlib1g-dev libbz2-dev libleveldb-dev liblua5.3-dev libjsoncpp-dev libmaxminddb-dev libmediainfo-dev libzen-dev
```

Обратите внимание, что эти пакеты устанавливают библиотеки для хост-системы (Ubuntu). Для кросс-компиляции под Windows нужны скомпилированные под Windows версии. Альтернативно можно собрать зависимости из исходников, используя тот же тулчейн.

2. Создайте директорию для сборки и запустите CMake с тулчейном:

```bash
mkdir build-win && cd build-win
cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/Toolchain-mingw64.cmake ..
make -j$(nproc)
```

Если зависимости не найдены, возможно, потребуется указать пути к ним через переменные CMake (например, `-DOPENSSL_ROOT_DIR=/usr/x86_64-w64-mingw32`).

3. Полученный исполняемый файл `flylinkdc.exe` будет находиться в поддиректории `bin` (или в корне сборки).

#### Инструкция для сборки под Windows с MSVC

```bash
mkdir build && cd build
cmake -G "Visual Studio 17 2022" -A x64 ..
cmake --build . --config Release
```

#### Файл тулчейна для MinGW-w64

Создайте файл `cmake/Toolchain-mingw64.cmake` со следующим содержимым:

```cmake
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)

set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
```

### Примечания

- Проект использует предкомпилированные заголовки (stdinc.h). CMake автоматически генерирует их.
- Для сборки требуется поддержка C++17.
- Некоторые зависимости (например, WTL) являются заголовочными библиотеками и должны быть расположены в соответствующих директориях.

## История изменений

См. [HISTORY.md](HISTORY.md)

## Лицензия

GNU General Public License v2
