#
#	CMakeLists.txt for the GNOME PKPASS Integration readers and tools.
#
#	The build tree is generated output and is written to dataflow.out/build/.
#	Warnings are treated as errors per prompts/flavors/02-cpp-conventions.md.
#
cmake_minimum_required(VERSION 3.16)
project(gnome_pkpass_integration LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

add_compile_options(-Wall -Wextra -Wpedantic -Werror)
include_directories("${CMAKE_BINARY_DIR}/generated")

find_package(ZLIB REQUIRED)
find_package(PkgConfig QUIET)

add_library(version_info STATIC version/version_info.cpp)
target_include_directories(version_info PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(crypto STATIC crypto/sha1.cpp)
target_include_directories(crypto PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(json_value STATIC json/json_reader.cpp)
target_include_directories(json_value PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(file_util STATIC util/file_io.cpp util/process.cpp)
target_include_directories(file_util PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(zip_reader STATIC zip/zip_reader.cpp)
target_include_directories(zip_reader PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(zip_reader PUBLIC ZLIB::ZLIB file_util)

add_library(pkpass_reader STATIC
    pkpass/pkpass_reader.cpp
    pkpass/pkpass_verify.cpp)
target_include_directories(pkpass_reader PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(pkpass_reader PUBLIC zip_reader json_value crypto file_util)

add_library(pass_render STATIC render/pass_render.cpp)
target_include_directories(pass_render PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(pass_render PUBLIC pkpass_reader zip_reader file_util)

add_library(integration STATIC integration/handler_integrator.cpp)
target_include_directories(integration PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(integration PUBLIC file_util version_info)

add_executable(pkpass-inspect tools/pkpass_inspect.cpp)
target_link_libraries(pkpass-inspect PRIVATE pkpass_reader json_value file_util version_info)

add_executable(pkpass-render tools/pkpass_render.cpp)
target_link_libraries(pkpass-render PRIVATE pass_render pkpass_reader file_util version_info)

add_executable(pkpass-integrate tools/pkpass_integrate.cpp)
target_link_libraries(pkpass-integrate PRIVATE integration file_util version_info)

file(COPY tools/icons DESTINATION "${CMAKE_BINARY_DIR}")

# The graphical viewer is a GTK4 program. It is built only where the GTK4
# development files are present; the command-line tools work without it, and
# `pkpass-integrate handle` then prints the equivalent report instead.
if(PkgConfig_FOUND)
    pkg_check_modules(GTK4 IMPORTED_TARGET gtk4)
endif()
if(TARGET PkgConfig::GTK4)
    add_executable(pkpass-viewer tools/pkpass_viewer_ui.cpp)
    target_link_libraries(pkpass-viewer PRIVATE PkgConfig::GTK4 pkpass_reader zip_reader
        file_util version_info)
else()
    message(STATUS "GTK4 development files not found: pkpass-viewer is not built")
endif()

# --- tests -------------------------------------------------------------------
# The C++ tests are built here and run by the shell tests in tests/.
add_executable(sha1-test "${CMAKE_CURRENT_SOURCE_DIR}/../tests/sha1_test.cpp")
target_link_libraries(sha1-test PRIVATE crypto)

add_executable(json-reader-test "${CMAKE_CURRENT_SOURCE_DIR}/../tests/json_reader_test.cpp")
target_link_libraries(json-reader-test PRIVATE json_value)

add_executable(zip-reader-test "${CMAKE_CURRENT_SOURCE_DIR}/../tests/zip_reader_test.cpp")
target_link_libraries(zip-reader-test PRIVATE zip_reader file_util)

add_executable(pkpass-reader-test "${CMAKE_CURRENT_SOURCE_DIR}/../tests/pkpass_reader_test.cpp")
target_link_libraries(pkpass-reader-test PRIVATE pkpass_reader zip_reader file_util)

add_executable(pass-render-test "${CMAKE_CURRENT_SOURCE_DIR}/../tests/pass_render_test.cpp")
target_link_libraries(pass-render-test PRIVATE pass_render pkpass_reader)

add_executable(handler-integrator-test
    "${CMAKE_CURRENT_SOURCE_DIR}/../tests/handler_integrator_test.cpp")
target_link_libraries(handler-integrator-test PRIVATE integration)
