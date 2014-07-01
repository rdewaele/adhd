cmake_minimum_required (VERSION 3.0)
project (ADHD)

# library resp. program name
set (LNAME adhd)
set (PNAME ADHD)

# compiler settings
include(CompilerConfig.cmake)

# use solution folders in visual studio
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# the library
add_library(${LNAME} benchmark.cpp prettyprint.cpp)

# the executable
include_directories(${ADHD_SOURCE_DIR})
link_directories(${ADHD_BINARY_DIR})
add_executable(${PNAME} main.cpp)
target_link_libraries(${PNAME} ${LNAME})
