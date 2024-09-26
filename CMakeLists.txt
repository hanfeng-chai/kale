cmake_minimum_required(VERSION 3.10)

# Project name
project(kale)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Use Clang and LLD
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_LINKER lld)

# Enable all warnings and treat them as errors
add_compile_options(-Wall -Wextra -Werror)

# Use libc++
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
# set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++")

# Generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Specify the source files recursively
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Specify the header files recursively
file(GLOB_RECURSE HEADERS "inc/*.h")

# Add the executable
add_executable(kale ${SOURCES} ${HEADERS})

# Find and link LLVM
# find_package(LLVM REQUIRED CONFIG)
# llvm_map_components_to_libnames(llvm_libs all)

# Link against LLVM libraries
target_link_libraries(kale LLVM)

# Include directories
target_include_directories(kale PRIVATE inc)
