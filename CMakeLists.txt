cmake_minimum_required(VERSION 3.23)
project(gbe LANGUAGES C CXX)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# GNU extensions for gamepad.c (strdup, MAX_PATH, etc.)
set(CMAKE_C_EXTENSIONS ON)

# EMU_BUILD_STRING default
if(NOT DEFINED EMU_BUILD_STRING)
  string(TIMESTAMP EMU_BUILD_STRING "%Y_%m_%d-%H_%M_%S")
endif()

# Find system dependencies
find_package(ZLIB REQUIRED)
find_package(CURL REQUIRED)
find_package(absl REQUIRED)
find_package(Protobuf REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(OPUS REQUIRED opus)
pkg_check_modules(PORTAUDIO REQUIRED portaudio-2.0)

# mbedtls 3.x for auth ticket signing
# Try -3 suffix first (Gentoo slotted installs), fall back to unversioned names
pkg_check_modules(MBEDTLS mbedtls-3)
if(NOT MBEDTLS_FOUND)
  pkg_check_modules(MBEDTLS REQUIRED mbedtls)
endif()
pkg_check_modules(MBEDCRYPTO mbedcrypto-3)
if(NOT MBEDCRYPTO_FOUND)
  pkg_check_modules(MBEDCRYPTO REQUIRED mbedcrypto)
endif()
pkg_check_modules(MBEDX509 mbedx509-3)
if(NOT MBEDX509_FOUND)
  pkg_check_modules(MBEDX509 REQUIRED mbedx509)
endif()

# libssq submodule
add_subdirectory(third-party/libssq EXCLUDE_FROM_ALL)

# Generate protobuf files
set(PROTO_SRC "${CMAKE_CURRENT_SOURCE_DIR}/dll/net.proto")
set(PROTO_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/proto_gen")
file(MAKE_DIRECTORY "${PROTO_OUT_DIR}")

add_custom_command(
  OUTPUT "${PROTO_OUT_DIR}/net.pb.cc" "${PROTO_OUT_DIR}/net.pb.h"
  COMMAND protobuf::protoc
    --cpp_out="${PROTO_OUT_DIR}"
    -I "${CMAKE_CURRENT_SOURCE_DIR}/dll"
    "${PROTO_SRC}"
  DEPENDS "${PROTO_SRC}"
  COMMENT "Generating protobuf files from net.proto"
)
add_custom_target(proto_gen DEPENDS "${PROTO_OUT_DIR}/net.pb.cc" "${PROTO_OUT_DIR}/net.pb.h")

# Collect common source files
file(GLOB DLL_SOURCES "dll/*.cpp")
set(HELPER_SOURCES
  helpers/common_helpers.cpp
  helpers/dbg_log.cpp
)
set(LIB_SOURCES
  libs/gamepad/gamepad.c
  libs/simpleini/ConvertUTF.c
)
set(CRASH_PRINTER_SOURCES crash_printer/linux.cpp)
set(PROTO_SOURCES "${PROTO_OUT_DIR}/net.pb.cc")

# Common include directories
set(COMMON_INCLUDES
  "${CMAKE_CURRENT_SOURCE_DIR}/dll"
  "${CMAKE_CURRENT_SOURCE_DIR}/libs"
  "${CMAKE_CURRENT_SOURCE_DIR}/libs/utfcpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/helpers"
  "${CMAKE_CURRENT_SOURCE_DIR}/crash_printer"
  "${CMAKE_CURRENT_SOURCE_DIR}/sdk"
  "${CMAKE_CURRENT_SOURCE_DIR}/overlay_experimental"
  "${PROTO_OUT_DIR}"
)

# Common compile definitions
set(COMMON_DEFINES
  UTF_CPP_CPLUSPLUS=201703L
  CONTROLLER_SUPPORT
  EMU_BUILD_STRING=${EMU_BUILD_STRING}
  GNUC
)

# Common compile options
set(COMMON_C_FLAGS -fvisibility=hidden -fno-jump-tables -Wno-switch)
set(COMMON_CXX_FLAGS -fvisibility=hidden -fno-jump-tables -Wno-switch -fno-char8_t)
set(COMMON_LINK_FLAGS -Wl,--exclude-libs,ALL)

# Common link libraries
set(COMMON_LIBS
  ssq
  ZLIB::ZLIB
  CURL::libcurl
  protobuf::libprotobuf-lite
  ${OPUS_LIBRARIES}
  ${PORTAUDIO_LIBRARIES}
  ${MBEDTLS_LIBRARIES}
  ${MBEDCRYPTO_LIBRARIES}
  ${MBEDX509_LIBRARIES}
  pthread
  dl
)

# Helper function for setting up a goldberg shared library target
function(gbe_shared_lib TARGET_NAME OUTPUT_NAME DEFINE_STEAMCLIENT EXCLUDE_FLAT)
  # Determine sources
  set(SOURCES ${DLL_SOURCES} ${HELPER_SOURCES} ${LIB_SOURCES} ${CRASH_PRINTER_SOURCES} ${PROTO_SOURCES})

  if(EXCLUDE_FLAT)
    list(FILTER SOURCES EXCLUDE REGEX "dll/flat\\.cpp$")
  endif()

  add_library(${TARGET_NAME} SHARED ${SOURCES})
  add_dependencies(${TARGET_NAME} proto_gen)

  set_target_properties(${TARGET_NAME} PROPERTIES
    OUTPUT_NAME "${OUTPUT_NAME}"
    PREFIX ""
  )

  target_include_directories(${TARGET_NAME} PRIVATE
    ${COMMON_INCLUDES}
    ${OPUS_INCLUDE_DIRS}
    ${PORTAUDIO_INCLUDE_DIRS}
    ${MBEDTLS_INCLUDE_DIRS}
    ${MBEDCRYPTO_INCLUDE_DIRS}
  )

  set(TARGET_DEFINES ${COMMON_DEFINES})
  if(DEFINE_STEAMCLIENT)
    list(APPEND TARGET_DEFINES STEAMCLIENT_DLL)
  endif()
  target_compile_definitions(${TARGET_NAME} PRIVATE ${TARGET_DEFINES})

  target_compile_options(${TARGET_NAME} PRIVATE
    $<$<COMPILE_LANGUAGE:C>:${COMMON_C_FLAGS}>
    $<$<COMPILE_LANGUAGE:CXX>:${COMMON_CXX_FLAGS}>
  )

  target_link_options(${TARGET_NAME} PRIVATE ${COMMON_LINK_FLAGS})
  target_link_libraries(${TARGET_NAME} PRIVATE ${COMMON_LIBS})
  target_link_directories(${TARGET_NAME} PRIVATE ${OPUS_LIBRARY_DIRS} ${PORTAUDIO_LIBRARY_DIRS} ${MBEDCRYPTO_LIBRARY_DIRS})
endfunction()

# steamclient_regular (steamclient.so)
gbe_shared_lib(steamclient_regular "steamclient" ON ON)

# api_regular (libsteam_api.so)
gbe_shared_lib(api_regular "libsteam_api" OFF OFF)

# lib_steamnetworkingsockets
add_library(lib_steamnetworkingsockets SHARED
  networking_sockets_lib/steamnetworkingsockets.cpp
  helpers/common_helpers.cpp
  helpers/dbg_log.cpp
)
set_target_properties(lib_steamnetworkingsockets PROPERTIES
  OUTPUT_NAME "steamnetworkingsockets"
  PREFIX "lib"
)
target_include_directories(lib_steamnetworkingsockets PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/dll"
  "${CMAKE_CURRENT_SOURCE_DIR}/libs"
  "${CMAKE_CURRENT_SOURCE_DIR}/libs/utfcpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/helpers"
  "${CMAKE_CURRENT_SOURCE_DIR}/crash_printer"
  "${CMAKE_CURRENT_SOURCE_DIR}/sdk"
  "${CMAKE_CURRENT_SOURCE_DIR}/overlay_experimental"
)
target_compile_options(lib_steamnetworkingsockets PRIVATE ${COMMON_CXX_FLAGS})
target_compile_definitions(lib_steamnetworkingsockets PRIVATE GNUC)

# tool_generate_interfaces
add_executable(tool_generate_interfaces
  tools/generate_interfaces/generate_interfaces.cpp
  helpers/common_helpers.cpp
)
target_include_directories(tool_generate_interfaces PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/helpers"
  "${CMAKE_CURRENT_SOURCE_DIR}/libs"
  "${CMAKE_CURRENT_SOURCE_DIR}/libs/utfcpp"
)
target_compile_options(tool_generate_interfaces PRIVATE ${COMMON_CXX_FLAGS})

# tool_lobby_connect
set(LOBBY_SOURCES ${DLL_SOURCES} ${HELPER_SOURCES} ${CRASH_PRINTER_SOURCES} ${PROTO_SOURCES})
list(FILTER LOBBY_SOURCES EXCLUDE REGEX "dll/flat\\.cpp$")
# lobby_connect does not use gamepad
set(LOBBY_LIB_SOURCES libs/simpleini/ConvertUTF.c)

# lobby_connect defines: same as COMMON_DEFINES but without CONTROLLER_SUPPORT, plus extras
set(LOBBY_DEFINES
  UTF_CPP_CPLUSPLUS=201703L
  EMU_BUILD_STRING=${EMU_BUILD_STRING}
  GNUC
  NO_DISK_WRITES
  LOBBY_CONNECT
)

add_executable(tool_lobby_connect
  ${LOBBY_SOURCES}
  ${LOBBY_LIB_SOURCES}
  tools/lobby_connect/lobby_connect.cpp
)
add_dependencies(tool_lobby_connect proto_gen)
target_include_directories(tool_lobby_connect PRIVATE
  ${COMMON_INCLUDES}
  ${OPUS_INCLUDE_DIRS}
  ${PORTAUDIO_INCLUDE_DIRS}
  ${MBEDTLS_INCLUDE_DIRS}
  ${MBEDCRYPTO_INCLUDE_DIRS}
)
target_compile_definitions(tool_lobby_connect PRIVATE ${LOBBY_DEFINES})
target_compile_options(tool_lobby_connect PRIVATE
  $<$<COMPILE_LANGUAGE:C>:${COMMON_C_FLAGS}>
  $<$<COMPILE_LANGUAGE:CXX>:${COMMON_CXX_FLAGS}>
)
# For executables, protobuf's transitive abseil deps must be linked explicitly.
# CMake doesn't always propagate INTERFACE_LINK_LIBRARIES from imported targets
# for static linking scenarios. List the absl targets protobuf-lite depends on.
target_link_libraries(tool_lobby_connect PRIVATE ${COMMON_LIBS}
  absl::hash absl::status absl::statusor absl::strings absl::cord
  absl::raw_hash_set absl::log_internal_check_op absl::log_internal_message
  absl::log_globals absl::log_sink absl::log_entry absl::log_severity
  absl::strerror absl::synchronization absl::time absl::base absl::raw_logging_internal
)
target_link_directories(tool_lobby_connect PRIVATE ${OPUS_LIBRARY_DIRS} ${PORTAUDIO_LIBRARY_DIRS} ${MBEDCRYPTO_LIBRARY_DIRS})
