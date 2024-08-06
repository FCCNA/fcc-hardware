#
# CMakeLists.txt for midas examples/eventbuilder
#
# $MIDASSYS - env.variable pointing to the location of MIDAS
#
# Normal use:
#
# Copy files from examples/eventbuilder to the experiment directory:
#
# mkdir -p $HOME/online/evb
# cd $HOME/online/evb
# cp $MIDASSYS/examples/eventbuilder/* .
# mkdir build
# cd build
# cmake .. (or cmake3 ..)
# make
#

cmake_minimum_required(VERSION 3.5)

project(example_eventbuilder)

#
# set default build type
#

#if (NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
#    message(STATUS "Setting default build type to \"RelWithDebInfo\"")
#    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "" FORCE)
#endif()

add_compile_options("-O2")
add_compile_options("-g")
#add_compile_options("-std=c++11")

#
# Find MIDAS
#

if (NO_MIDAS)
   message(STATUS "${PROJECT_NAME}: MIDAS support is disabled via NO_MIDAS")
   unset(MIDAS_FOUND)
elseif(TARGET midas)
   message(STATUS "${PROJECT_NAME}: Building as a subproject of MIDAS")
   set(MIDAS_FOUND true)
   set(MIDASSYS ${CMAKE_HOME_DIRECTORY})
elseif(DEFINED ENV{MIDASSYS})
   include($ENV{MIDASSYS}/lib/midas-targets.cmake)
   include($ENV{MIDASSYS}/lib/manalyzer-targets.cmake)
   message(STATUS "${PROJECT_NAME}: Building with MIDAS from $ENV{MIDASSYS}/lib/midas-targets.cmake")
   set(MIDAS_FOUND TRUE)
   set(MIDASSYS $ENV{MIDASSYS})
else()
   message(STATUS "${PROJECT_NAME}: Building without MIDAS")
   unset(MIDAS_FOUND)
endif()

message(STATUS "${PROJECT_NAME}: MIDASSYS: ${MIDASSYS}")

#
# Find pthreads
#

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
#target_link_libraries(my_app PRIVATE Threads::Threads)

#
# enable certain compile warnings
#

add_compile_options(-Wall -Wformat=2 -Wno-format-nonliteral -Wno-strict-aliasing -Wuninitialized -Wno-unused-function)

add_executable(mevb
   mevb.cxx ebuser.cxx
   )

target_link_libraries(mevb mfe midas)

#end
