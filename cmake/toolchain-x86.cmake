# Cross-compilation toolchain for 32-bit x86 on a 64-bit Linux host
#
# 32-bit library path varies by distro:
#   Gentoo multilib: /usr/lib (32-bit), /usr/lib64 (64-bit)
#   Debian/Ubuntu:   /usr/lib/i386-linux-gnu or /usr/lib32
#   Fedora/Arch:     /usr/lib32
#
# Set PKG_CONFIG_PATH_32 to override the default (/usr/lib/pkgconfig).

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR i686)

set(CMAKE_C_FLAGS_INIT "-m32")
set(CMAKE_CXX_FLAGS_INIT "-m32")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-m32")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-m32")

# 32-bit pkg-config path (default: /usr/lib/pkgconfig for Gentoo)
if(DEFINED ENV{PKG_CONFIG_PATH_32})
  set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH_32}")
  set(ENV{PKG_CONFIG_LIBDIR} "$ENV{PKG_CONFIG_PATH_32}")
else()
  set(ENV{PKG_CONFIG_PATH} "/usr/lib/pkgconfig")
  set(ENV{PKG_CONFIG_LIBDIR} "/usr/lib/pkgconfig")
endif()

# Help CMake find 32-bit libraries
set(CMAKE_LIBRARY_PATH /usr/lib)
set(CMAKE_FIND_ROOT_PATH /usr)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Ensure protoc runs the native (64-bit) binary
find_program(Protobuf_PROTOC_EXECUTABLE protoc)
