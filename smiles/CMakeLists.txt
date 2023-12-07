cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
project(challenge VERSION 1.0)
enable_language(CXX)

#####]==-----------------------------------------
##  Look for external dependencies
#####]==-----------------------------------------

# look for the MPI dependency
find_package(MPI REQUIRED C)
find_package(OpenMP)

#####]==-----------------------------------------
##  Change the default behaviour
#####]==-----------------------------------------

# compile in Release mode, unless the user say otherwise
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "The type of build" FORCE)
  message(STATUS "Setting build type to '${CMAKE_BUILD_TYPE}' as none was specified")
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo")
endif()

# add default compiler flags on top of the CMake ones
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  add_compile_options(-Wall -Wextra -Wpedantic -Wl,-z,defs -Wl,-z,now -Wl,-z,relro -fdiagnostics-color=always)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Wdouble-promotion -fcolor-diagnostics)
endif ()

#####]==-----------------------------------------
##  Define the application sources
#####]==-----------------------------------------

# application headers
set(header_path "${CMAKE_CURRENT_SOURCE_DIR}/src")
list(APPEND header_files
  "${header_path}/mpi_error_check.hpp"
)

# application sources
set(source_path "${CMAKE_CURRENT_SOURCE_DIR}/src")
list(APPEND source_files
  "${source_path}/main.cpp"
  "${source_path}/mpi_error_check.cpp"
)

#####]==-----------------------------------------
##  Define the building process
#####]==-----------------------------------------

# define the building step
add_executable(main ${header_files} ${source_files})
target_include_directories(main PRIVATE "${header_path}")
set_target_properties(main
    PROPERTIES
      CXX_STANDARD 17
      CXX_STANDARD_REQUIRED ON
      CXX_EXTENSIONS OFF
  )

# link against MPI
# NOTE: we don't want to use the C++ bindings. However, the find_package
#       seems to ignore the MPI_CXX_SKIP_MPICXX variable. Therefore, we need
#       to set them manually >.>
target_compile_definitions(main PUBLIC "OMPI_SKIP_MPICXX") # OpenMPI
target_compile_definitions(main PUBLIC "MPICH_SKIP_MPICXX") # MPICH
target_link_libraries(main PUBLIC MPI::MPI_C)

# link against OpenMP
target_link_libraries(main PUBLIC OpenMP::OpenMP_CXX)
