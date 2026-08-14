cmake_minimum_required(VERSION 3.20)

project(cod2_downstream_wasm_core_probe C)

if(NOT EMSCRIPTEN)
    message(FATAL_ERROR "Configure this target with emcmake; native CMake is unsupported")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

set(COD2_SOURCE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../..")

add_executable(cod2_core_probe
    core_probe.c
    "${COD2_SOURCE_ROOT}/src/PC/qcommon/md4.c"
)

target_compile_definitions(cod2_core_probe PRIVATE
    COD2_DOWNSTREAM_WASM_CORE_PROBE=1
)

target_include_directories(cod2_core_probe PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}"
)

target_compile_options(cod2_core_probe PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -ffunction-sections
    -fdata-sections
)

target_link_options(cod2_core_probe PRIVATE
    "-sWASM=1"
    "-sENVIRONMENT=web,node"
    "-sEXIT_RUNTIME=1"
    "-sASSERTIONS=1"
    "-sALLOW_MEMORY_GROWTH=1"
    "-sFILESYSTEM=0"
    "-Wl,--gc-sections"
)

set_target_properties(cod2_core_probe PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/site"
    SUFFIX ".js"
)

configure_file(index.html "${CMAKE_BINARY_DIR}/site/index.html" COPYONLY)
