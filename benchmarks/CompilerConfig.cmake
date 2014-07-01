# compiler flags; source:
# http://www.cmake.org/cmake/help/v2.8.10/cmake.html#variable:CMAKE_LANG_COMPILER_ID
set (WARNFLAGS -Wall -Wextra -pedantic -Wconversion -Wshadow -Wpointer-arith
  -Wcast-qual -Wcast-align -Wwrite-strings -Waggregate-return -fno-common)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	add_definitions(-std=c++11)
  add_definitions(${WARNFLAGS})

elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  add_definitions(-std=c++11)
  add_definitions(${WARNFLAGS})

elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
  add_definitions(-std=c++11)
  add_definitions(${WARNFLAGS})

elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")

endif()

# libraries

if(UNIX)
  SET (UNIX_LIBRARIES pthread)
else(UNIX)
  SET (WIN_LIBRARIES "")
endif(UNIX)

link_libraries(papi ${UNIX_LIBRARIES})
