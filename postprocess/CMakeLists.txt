cmake_minimum_required(VERSION 2.8)
find_package(LLVM)

# Define add_llvm_* macro's.
include(AddLLVM)

add_definitions(${LLVM_DEFINITIONS})
include_directories(${CAJUN_INCLUDE_DIR})
include_directories(${LLVM_INCLUDE_DIRS} /usr/include/x86_64-linux-gnu/c++/4.9)
link_directories(${LLVM_LIBRARY_DIRS})

add_subdirectory(translator)
add_subdirectory(linker)
