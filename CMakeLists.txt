cmake_minimum_required(VERSION 3.12)
project(FlylinkDC LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

# Detect target system
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(IS_WINDOWS TRUE)
else()
    set(IS_WINDOWS FALSE)
endif()

# Cross-compilation detection
if(CMAKE_CROSSCOMPILING)
    message(STATUS "Cross-compiling for ${CMAKE_SYSTEM_NAME}")
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/boost
    ${CMAKE_CURRENT_SOURCE_DIR}/openssl/include
    ${CMAKE_CURRENT_SOURCE_DIR}/wtl
    ${CMAKE_CURRENT_SOURCE_DIR}/zlib-ng
    ${CMAKE_CURRENT_SOURCE_DIR}/windows
    ${CMAKE_CURRENT_SOURCE_DIR}/bzip2
    ${CMAKE_CURRENT_SOURCE_DIR}/leveldb/include
    ${CMAKE_CURRENT_SOURCE_DIR}/client
    ${CMAKE_CURRENT_SOURCE_DIR}/libtorrent/include
    ${CMAKE_CURRENT_SOURCE_DIR}/selene/include
    ${CMAKE_CURRENT_SOURCE_DIR}/lua
    ${CMAKE_CURRENT_SOURCE_DIR}/jsoncpp/include
    ${CMAKE_CURRENT_SOURCE_DIR}/maxminddb
    ${CMAKE_CURRENT_SOURCE_DIR}/mediainfolib/Source
    ${CMAKE_CURRENT_SOURCE_DIR}/Zenlib/Source
)

# Preprocessor definitions
add_definitions(
    -DUNICODE
    -D_UNICODE
    -DBOOST_USE_WINDOWS_H
    -DTORRENT_NO_DEPRECATE
    -DWEBRTC_WIN
    -DZMQ_STATIC
    -DOS_WIN
    -DLEVELDB_PLATFORM_WINDOWS
    -DOPENSSL_SYSNAME_WIN32
    -DHAVE_NATUPNP_H
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-D_DEBUG)
else()
    add_definitions(-DNDEBUG)
endif()

# Compiler-specific flags
if(MSVC)
    add_compile_options(/W4 /wd4996)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wno-deprecated-declarations)
endif()

# Find libraries (for cross-compilation, we assume they are installed in the sysroot)
find_library(ZLIB_LIBRARY NAMES zlib1 zlib PATHS /usr/x86_64-w64-mingw32/lib)
find_library(BZIP2_LIBRARY NAMES bz2 PATHS /usr/x86_64-w64-mingw32/lib)
find_library(OPENSSL_SSL_LIBRARY NAMES ssl PATHS /usr/x86_64-w64-mingw32/lib)
find_library(OPENSSL_CRYPTO_LIBRARY NAMES crypto PATHS /usr/x86_64-w64-mingw32/lib)
find_library(LIBTORRENT_LIBRARY NAMES torrent PATHS /usr/x86_64-w64-mingw32/lib)
find_library(LEVELDB_LIBRARY NAMES leveldb PATHS /usr/x86_64-w64-mingw32/lib)
find_library(LUA_LIBRARY NAMES lua5.3 lua PATHS /usr/x86_64-w64-mingw32/lib)
find_library(JSONCPP_LIBRARY NAMES jsoncpp PATHS /usr/x86_64-w64-mingw32/lib)
find_library(MAXMINDDB_LIBRARY NAMES maxminddb PATHS /usr/x86_64-w64-mingw32/lib)
find_library(MEDIAINFO_LIBRARY NAMES mediainfo PATHS /usr/x86_64-w64-mingw32/lib)
find_library(ZEN_LIBRARY NAMES zen PATHS /usr/x86_64-w64-mingw32/lib)

# Client library
file(GLOB_RECURSE CLIENT_SOURCES
    client/*.cpp
    client/*.c
    client/webrtc/rtc_base/synchronization/*.cc
    client/webrtc/talk/base/*.cc
    client/XMLParser/*.cpp
    client/sqlite/*.c
    client/sqlite/*.cpp
    client/idna/*.cpp
    client/FlyFeatures/*.cpp
)

# Exclude StringDefs.cpp because it's generated
list(FILTER CLIENT_SOURCES EXCLUDE REGEX ".*StringDefs\\.cpp$")

add_library(client STATIC ${CLIENT_SOURCES})
target_link_libraries(client
    ${ZLIB_LIBRARY}
    ${BZIP2_LIBRARY}
    ${OPENSSL_SSL_LIBRARY}
    ${OPENSSL_CRYPTO_LIBRARY}
    ${LIBTORRENT_LIBRARY}
    ${LEVELDB_LIBRARY}
    ${LUA_LIBRARY}
    ${JSONCPP_LIBRARY}
    ${MAXMINDDB_LIBRARY}
    ${MEDIAINFO_LIBRARY}
    ${ZEN_LIBRARY}
)

# Windows GUI executable
file(GLOB_RECURSE WINDOWS_SOURCES
    windows/*.cpp
    windows/*.c
)

# Resource files
if(IS_WINDOWS)
    set(RESOURCE_FILES
        FlylinkDC.rc
        FlylinkDC_version.rc
    )
    list(APPEND WINDOWS_SOURCES ${RESOURCE_FILES})
endif()

add_executable(flylinkdc ${WINDOWS_SOURCES})
target_link_libraries(flylinkdc
    client
    # Windows libraries
    ws2_32 winmm powrprof gdiplus delayimp msimg32 wininet shell32 comdlg32 uxtheme
    imagehlp shfolder
)

# Install target
install(TARGETS flylinkdc DESTINATION bin)