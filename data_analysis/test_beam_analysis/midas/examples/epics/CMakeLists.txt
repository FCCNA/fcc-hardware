#
# CMakeLists.txt for midas examples/epics
#
# $MIDASSYS - env.variable pointing to the location of MIDAS
# $EPICSSYS - env.variable pointing to the location of EPICS, i.e. $HOME/packages/epics-base
#

cmake_minimum_required(VERSION 3.5)

project(example_epics)

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

#
# Find EPICS
#

if (NO_EPICS)
   message(STATUS "${PROJECT_NAME}: EPICS support is disabled via NO_EPICS")
   unset(EPICS_FOUND)
elseif(DEFINED ENV{EPICSSYS})
   message(STATUS "${PROJECT_NAME}: Building with EPICS from $ENV{EPICSSYS}")
   set(EPICS_FOUND TRUE)
   set(EPICSSYS $ENV{EPICSSYS})
else()
   message(STATUS "${PROJECT_NAME}: Building without EPICS")
   unset(EPICS_FOUND)
endif()

message(STATUS "${PROJECT_NAME}: MIDASSYS: ${MIDASSYS}")
message(STATUS "${PROJECT_NAME}: EPICSSYS: ${EPICSSYS}")

#
# enable certain compile warnings
#

add_compile_options(-Wall -Wformat=2 -Wno-format-nonliteral -Wno-strict-aliasing -Wuninitialized -Wno-unused-function)

if (EPICS_FOUND)

add_compile_options(-I${MIDASSYS}/drivers -I${EPICSSYS}/include -I${EPICSSYS}/include/os/Linux -I${EPICSSYS}/include/os/Darwin -I${EPICSSYS}/include/compiler/gcc -I${EPICSSYS}/include/compiler/clang)

link_directories(${EPICSSYS}/lib/darwin-aarch64 ${EPICSSYS}/lib/linux-x86_64 ${EPICSSYS}/lib/RHEL8-x86_64)

add_executable(fe_epics
   frontend.cxx ${MIDASSYS}/drivers/device/epics_ca.cxx ${MIDASSYS}/drivers/class/generic.cxx
   )

target_link_libraries(fe_epics mfe midas ca)

endif()

#end
