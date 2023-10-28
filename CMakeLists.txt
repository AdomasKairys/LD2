cmake_minimum_required(VERSION 3.12)
project(LD2 VERSION 0.1.0 LANGUAGES C CXX)

include(CTest)
enable_testing()
find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH})

add_executable(LD2 main.cpp)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
