cmake_minimum_required(VERSION 3.22)
project(godot_gis LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# If you used: conan install . --output-folder=build --build=missing
# then the toolchain file is at build/conan_toolchain.cmake (pass it on the command line)
# and CMakeDeps packages live in build/generators. Teach CMake where to find them:
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}/generators")

# godot-cpp (expects submodule at extern/godot-cpp, branch 4.3/4.4 matching editor)
add_subdirectory(extern/godot-cpp)

# Find Conan-provided packages (from CMakeDeps)
find_package(gdal CONFIG REQUIRED)   # provides target gdal::gdal
find_package(proj CONFIG QUIET)      # provides target proj::proj

file(GLOB_RECURSE SRC CONFIGURE_DEPENDS
    src/*.cpp
    src/**/*.cpp
)

add_library(godot-gis SHARED ${SRC})
target_include_directories(godot-gis PRIVATE src)

target_link_libraries(godot-gis PRIVATE godot-cpp GDAL::GDAL)
if (TARGET proj::proj)
    target_link_libraries(godot-gis PRIVATE proj::proj)
endif()

if (MSVC)
  target_compile_options(godot-gis PRIVATE /W4 /permissive- /wd4100)
else()
  # enable warnings but suppress unused-parameter which commonly appears in generated headers
  target_compile_options(godot-gis PRIVATE -Wall -Wextra -Wpedantic -Wno-unused-parameter)
endif()

set_target_properties(godot-gis PROPERTIES
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
