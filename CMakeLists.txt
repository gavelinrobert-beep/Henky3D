cmake_minimum_required(VERSION 3.20)
project(Henky3D VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Find DirectX 12
if(WIN32)
    find_package(directx-headers CONFIG)
    if(NOT directx-headers_FOUND)
        message(STATUS "DirectX headers will be linked from Windows SDK")
    endif()
endif()

# Add external dependencies
add_subdirectory(external)

# Add engine library
add_subdirectory(src/engine)

# Add main application
add_subdirectory(src)
