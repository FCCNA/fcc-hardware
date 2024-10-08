#
# CMakeLists.txt for midas examples/experiment
#
# S.R. 28 May 2019
# K.O. 28 June 2019
#
# This cmakefile is dual use:
# if MIDAS_IN_TREE_BUILD is set, the example experiment is built as part of the midas compilation
# if MIDAS_IN_TREE_BUILD is unset, the example experiment is built against MIDAS installed at $MIDASSYS
#
# Optional settings for this cmakefile:
#
# $MIDASSYS - env.variable pointing to the location of MIDAS
# root-config in the $PATH - sets the location of ROOT for the analyzer
# -DNO_ROOT=1 - do not use ROOT
#
# Normal use:
#
# Copy all files from examples/experiment to the experiment directory:
# mkdir -p $HOME/online/src
# cd $HOME/online/src
# cp $MIDASSYS/examples/experiment/* .
# mkdir build
# cd build
# cmake ..
# or cmake3 ..
# make
#
# For testing the cmakefile inside the midas tree:
#
# cd $HOME/git/midas/examples/experiment
# mkdir build
# cd build
# MIDASSYS=$HOME/git/midas cmake ..
# make
#

cmake_minimum_required(VERSION 3.0)
project(basic)

# check if we are called from parent directory
if (NOT ${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
   message(STATUS "MIDAS experiment: MIDAS in-tree-build")
else()
   message(STATUS "MIDAS experiment: MIDASSYS set to " $ENV{MIDASSYS})
   if (NOT EXISTS $ENV{MIDASSYS})
      message(FATAL_ERROR "Environment variable $MIDASSYS not defined, aborting.")
   endif()
   set(INC_PATH $ENV{MIDASSYS}/include $ENV{MIDASSYS}/mxml)
   link_directories($ENV{MIDASSYS}/lib)
endif()

## default build type
#if (NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
#    message(STATUS "Setting default build type to \"RelWithDebInfo\"")
#    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "" FORCE)
#endif()

add_compile_options("-O2")
add_compile_options("-g")
#add_compile_options("-std=c++11")

# enable certain compile warnings
add_compile_options(-Wall -Wformat=2 -Wno-format-nonliteral -Wno-strict-aliasing -Wuninitialized -Wno-unused-function)

find_package(ZLIB REQUIRED)
if (ZLIB_FOUND)
   message(STATUS "MIDAS experiment: Found ZLIB version " ${ZLIB_VERSION_STRING})
   add_compile_options(-I${ZLIB_INCLUDE_DIRS})
   set(LIBS ${LIBS} ${ZLIB_LIBRARIES})
endif()

# as required linux libraries
if (${CMAKE_SYSTEM_NAME} MATCHES Linux) 
   set(LIBS ${LIBS} -lpthread -lutil -lrt) 
endif() 

add_executable(deferredfe deferredfe.cxx)
target_include_directories(deferredfe PRIVATE ${INC_PATH})
target_link_libraries(deferredfe mfe midas ${LIBS})

add_executable(largefe largefe.cxx)
target_include_directories(largefe PRIVATE ${INC_PATH})
target_link_libraries(largefe mfe midas ${LIBS})

if (NO_ROOT)
   message(STATUS "MIDAS experiment: ROOT support is disabled via NO_ROOT")
else (NO_ROOT)

   if (DEFINED ENV{ROOTSYS})
      # You need to tell CMake where to find the ROOT installation. This can be done in a number of ways:
      #   - ROOT built with classic configure/make use the provided $ROOTSYS/etc/cmake/FindROOT.cmake
      #   - ROOT built with CMake. Add in CMAKE_PREFIX_PATH the installation prefix for ROOT
      list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})

      #---Locate the ROOT package and defines a number of variables (e.g. ROOT_INCLUDE_DIRS)
      find_package(ROOT REQUIRED COMPONENTS MathCore RIO Hist Tree Net)

      #---Define useful ROOT functions and macros (e.g. ROOT_GENERATE_DICTIONARY)
      include(${ROOT_USE_FILE})

      if (ROOT_FOUND)
         separate_arguments(ROOT_CXX_FLAGS UNIX_COMMAND "${ROOT_CXX_FLAGS}")
         message(STATUS "MIDAS experiment: Found ROOT version " ${ROOT_VERSION})
         set(HAVE_ROOT true)
      else (ROOT_FOUND)
         message(STATUS "MIDAS experiment: ROOT not found")
      endif (ROOT_FOUND)
   else ()
      message(STATUS "MIDAS experiment: ROOTSYS not defined")
   endif()

endif(NO_ROOT)


