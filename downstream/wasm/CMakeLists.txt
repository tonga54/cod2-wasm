cmake_minimum_required(VERSION 3.20)
project(cod2_downstream_wasm LANGUAGES C CXX)

if(NOT EMSCRIPTEN)
  message(FATAL_ERROR "Configure this target through scripts/build-web.sh")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(COD2_ROOT "${CMAKE_CURRENT_LIST_DIR}/../..")
set(COD2_SRC "${COD2_ROOT}/src")

file(GLOB_RECURSE COD2_PC_C CONFIGURE_DEPENDS "${COD2_SRC}/PC/*.c")
file(GLOB_RECURSE COD2_MAC_C CONFIGURE_DEPENDS "${COD2_SRC}/Mac/*.c")
file(GLOB_RECURSE COD2_STUB_C CONFIGURE_DEPENDS "${COD2_SRC}/stubs/*.c")
file(GLOB COD2_ROOT_C CONFIGURE_DEPENDS "${COD2_SRC}/*.c")
set(COD2_CLIENT_SOURCES ${COD2_PC_C} ${COD2_MAC_C} ${COD2_STUB_C} ${COD2_ROOT_C})

# Use Emscripten's maintained zlib port, reconstructed portable data C, and a
# downstream browser entrypoint. No native x86/Mach-O assembly enters the build.
list(FILTER COD2_CLIENT_SOURCES EXCLUDE REGEX "/PC/zlib/(inflate|infblock|infcodes|inffast|inftrees|infutil|adler32|zutil)\\.c$")
list(FILTER COD2_CLIENT_SOURCES EXCLUDE REGEX "/(data|import_pointers|literals)\\.c$")
list(FILTER COD2_CLIENT_SOURCES EXCLUDE REGEX "/stubs/(cpp_trampoline|zlib_alloc)\\.c$")
list(FILTER COD2_CLIENT_SOURCES EXCLUDE REGEX "/Mac/Tools/(MacAppleEvents|MacBuilder)\\.c$")
list(FILTER COD2_CLIENT_SOURCES EXCLUDE REGEX "/PC/win32/win_net\\.c$")
list(FILTER COD2_CLIENT_SOURCES EXCLUDE REGEX "/Mac/Tools/MacMSS\\.c$")
# The reconstruction shares startup and prediction callbacks with game_mp and
# server_mp. Keep their real definitions while repairing the portable graph;
# the browser startup selects a remote client, never a local dedicated server.

list(APPEND COD2_CLIENT_SOURCES
  "${COD2_SRC}/blobs/bss.c"
  "${COD2_SRC}/unix/linux_input.c"
  "${COD2_SRC}/unix/linux_net.c"
  "${CMAKE_CURRENT_LIST_DIR}/web_platform.c"
  "${CMAKE_CURRENT_LIST_DIR}/web_net.c"
  "${CMAKE_CURRENT_LIST_DIR}/web_audio.c"
  "${COD2_ROOT}/build/native_gen/data32.c"
  "${COD2_ROOT}/build/native_gen/literals32.c"
  "${COD2_ROOT}/build/native_gen/import_pointers_native.c"
  "${CMAKE_CURRENT_LIST_DIR}/web_main.c"
  "${CMAKE_CURRENT_LIST_DIR}/web_gl_compat.c"
  "${CMAKE_CURRENT_LIST_DIR}/web_jpeg_compat.c")

add_library(cod2_client_objects OBJECT ${COD2_CLIENT_SOURCES})
target_compile_definitions(cod2_client_objects PRIVATE
  COD2_DOWNSTREAM_WASM=1
  COD2_FEATURE_WWW_DOWNLOAD=0
  COD2_FEATURE_PUNKBUSTER=0
  COD2_FEATURE_SCRIPT_DEBUGGER=0
  COD2_FEATURE_GAMEPAD=0
  COD2_FEATURE_SPLITSCREEN=0
  GL_GLEXT_PROTOTYPES=1
  SDL_DISABLE_IMMINTRIN_H=1)
target_include_directories(cod2_client_objects PRIVATE
  "${COD2_ROOT}"
  "${COD2_SRC}"
  "${COD2_SRC}/headers"
  "${COD2_SRC}/PC/jpeg-6"
  "${COD2_SRC}/PC/speex"
  "${CMAKE_CURRENT_LIST_DIR}")
target_compile_options(cod2_client_objects PRIVATE
  "SHELL:-s USE_SDL=2"
  -fno-common -fno-strict-aliasing -fwrapv -fno-strict-overflow
  -Wno-implicit-function-declaration -Wno-incompatible-pointer-types
  -Wno-int-conversion -Wno-return-type -Wno-deprecated-non-prototype
  -Wno-duplicate-decl-specifier -Wno-ignored-attributes)

# Keep the unfinished engine link explicit until the browser runtime works.
# Undefined symbols and ABI warnings must be repaired, never hidden by the
# checksum diagnostic or by permissive linker flags.
add_executable(cod2_client EXCLUDE_FROM_ALL $<TARGET_OBJECTS:cod2_client_objects>)
target_link_options(cod2_client PRIVATE
  -g2
  "SHELL:-s USE_SDL=2"
  "SHELL:-s USE_ZLIB=1"
  "SHELL:--pre-js ${CMAKE_CURRENT_LIST_DIR}/web_audio.js"
  # SDL2 in Emscripten 3.1.64 addresses Module.canvas with the #canvas alias.
  # The framework owns that element and uses its own DOM id.
  "SHELL:-s DISABLE_DEPRECATED_FIND_EVENT_TARGET_BEHAVIOR=0"
  "SHELL:-s USE_WEBGL2=1"
  "SHELL:-s MIN_WEBGL_VERSION=2"
  "SHELL:-s MAX_WEBGL_VERSION=2"
  "SHELL:-s LEGACY_GL_EMULATION=1"
  "SHELL:-s GL_UNSAFE_OPTS=0"
  "SHELL:-s ALLOW_MEMORY_GROWTH=1"
  "SHELL:-s INITIAL_MEMORY=536870912"
  "SHELL:-s MAXIMUM_MEMORY=4294967296"
  "SHELL:-s MODULARIZE=1"
  "SHELL:-s EXPORT_NAME=createCod2Client"
  "SHELL:-s INVOKE_RUN=0"
  "SHELL:-s EXPORTED_RUNTIME_METHODS=FS,callMain"
  "SHELL:-s EXPORTED_FUNCTIONS=_main,_free,_web_client_state,_web_client_connected,_web_capture_lost"
  "SHELL:-s EXIT_RUNTIME=0"
  "SHELL:-s ASSERTIONS=1"
  "SHELL:-s ENVIRONMENT=web"
  "SHELL:-s STACK_SIZE=8388608"
  "SHELL:-Wl,--error-limit=0")
set_target_properties(cod2_client PROPERTIES
  OUTPUT_NAME "cod2"
  SUFFIX ".js"
  LINKER_LANGUAGE CXX
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/site")

find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(COD2_GL_PATCH "${COD2_ROOT}/scripts/patch-emscripten-gl.py")
set_property(TARGET cod2_client APPEND PROPERTY LINK_DEPENDS "${COD2_GL_PATCH}")
set_property(TARGET cod2_client APPEND PROPERTY LINK_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/web_audio.js")
add_custom_command(TARGET cod2_client POST_BUILD
  COMMAND "${Python3_EXECUTABLE}" "${COD2_GL_PATCH}" "$<TARGET_FILE:cod2_client>"
  COMMENT "Repairing pinned Emscripten legacy-GL texture-call generation"
  VERBATIM)

add_executable(cod2_core_probe
  "${CMAKE_CURRENT_LIST_DIR}/core_probe.c"
  "${COD2_SRC}/PC/qcommon/md4.c")
target_compile_definitions(cod2_core_probe PRIVATE COD2_DOWNSTREAM_WASM_CORE_PROBE=1)
target_include_directories(cod2_core_probe PRIVATE "${CMAKE_CURRENT_LIST_DIR}")
target_compile_options(cod2_core_probe PRIVATE
  -Wall -Wextra -Wpedantic -ffunction-sections -fdata-sections)
target_link_options(cod2_core_probe PRIVATE
  "SHELL:-s WASM=1"
  "SHELL:-s ENVIRONMENT=web,node"
  "SHELL:-s MODULARIZE=1"
  "SHELL:-s EXPORT_NAME=createCod2Diagnostic"
  "SHELL:-s EXIT_RUNTIME=1"
  "SHELL:-s ASSERTIONS=1"
  "SHELL:-s ALLOW_MEMORY_GROWTH=1"
  "SHELL:-s FILESYSTEM=0"
  "SHELL:-Wl,--gc-sections")
set_target_properties(cod2_core_probe PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/site"
  SUFFIX ".js")
