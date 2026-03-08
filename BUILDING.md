# Building from Source (Linux)

## Dependencies

- CMake 3.23+
- C17 / C++17 compiler (GCC or Clang)
- zlib
- curl
- protobuf (with abseil-cpp)
- opus
- portaudio
- mbedtls 3.x

## Submodules

Initialize the libssq submodule before building:

```bash
git submodule update --init --recursive
```

## Building

```bash
bash build_linux.sh
```

Options:

- `--x64-only` - build only 64-bit
- `--x32-only` - build only 32-bit
- `--debug` - build with debug symbols

Outputs are in `build/x64/` and `build/x32/`:

- `steamclient.so` - Steam client emulator
- `libsteam_api.so` - Steam API library
- `libsteamnetworkingsockets.so` - networking sockets library
- `tool_generate_interfaces` - interface generator
- `tool_lobby_connect` - lobby connection tool

### 32-bit Cross-compilation

The 32-bit build uses `cmake/toolchain-x86.cmake`. By default it looks for
32-bit pkg-config files in `/usr/lib/pkgconfig` (Gentoo convention). Override
with `PKG_CONFIG_PATH_32` for other distros:

```bash
# Debian/Ubuntu
PKG_CONFIG_PATH_32=/usr/lib/i386-linux-gnu/pkgconfig bash build_linux.sh --x32-only

# Arch/Fedora
PKG_CONFIG_PATH_32=/usr/lib32/pkgconfig bash build_linux.sh --x32-only
```

## Distro-specific Notes

### Ubuntu / Debian

```bash
apt install cmake build-essential libz-dev libcurl4-openssl-dev \
  libprotobuf-dev protobuf-compiler libabsl-dev libopus-dev \
  libportaudio2 libportaudiocpp0 portaudio19-dev libmbedtls-dev
```

For 32-bit: install the `:i386` variants of the library packages.

### Fedora

```bash
dnf install cmake gcc-c++ zlib-devel libcurl-devel protobuf-devel \
  abseil-cpp-devel opus-devel portaudio-devel mbedtls-devel
```

### Arch Linux

```bash
pacman -S cmake zlib curl protobuf abseil-cpp opus portaudio mbedtls
```

For 32-bit: install `lib32-` variants from the multilib repository.

### Gentoo

```bash
emerge -av dev-build/cmake sys-libs/zlib net-misc/curl dev-libs/protobuf \
  dev-libs/abseil-cpp media-libs/opus media-libs/portaudio net-libs/mbedtls
```

For 32-bit, enable `abi_x86_32` USE flags on the library packages.

**mbedtls note:** On Gentoo with slotted mbedtls, the 3.x pkg-config names
are `mbedtls-3`, `mbedcrypto-3`, `mbedx509-3`. The CMakeLists.txt handles
this automatically by trying both versioned and unversioned names.
