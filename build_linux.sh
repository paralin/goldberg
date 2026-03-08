#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"

build_threads="$(( $(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2) * 70 / 100 ))"
[[ $build_threads -lt 2 ]] && build_threads=2

BUILD_X64=1
BUILD_X32=1
BUILD_TYPE=Release

for arg in "$@"; do
  case "$arg" in
    --x64-only) BUILD_X32=0 ;;
    --x32-only) BUILD_X64=0 ;;
    --debug) BUILD_TYPE=Debug ;;
    --help)
      echo "Usage: $0 [--x64-only] [--x32-only] [--debug]"
      exit 0
      ;;
    *)
      echo "Unknown argument: $arg" >&2
      exit 1
      ;;
  esac
done

if [[ $BUILD_X64 -eq 1 ]]; then
  echo "=== Building x64 (${BUILD_TYPE}) ==="
  cmake -B build/x64 -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" .
  cmake --build build/x64 -j "$build_threads"
  echo "x64 build complete:"
  ls -la build/x64/steamclient.so build/x64/libsteam_api.so 2>/dev/null || true
fi

if [[ $BUILD_X32 -eq 1 ]]; then
  echo "=== Building x32 (${BUILD_TYPE}) ==="
  cmake -B build/x32 \
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-x86.cmake \
    .
  cmake --build build/x32 -j "$build_threads"
  echo "x32 build complete:"
  ls -la build/x32/steamclient.so build/x32/libsteam_api.so 2>/dev/null || true
fi

echo "=== Done ==="
