cmake_minimum_required(VERSION 3.5)
project(epics_fe)

# Check for MIDASSYS environment variable
if (NOT DEFINED ENV{MIDASSYS})
   message(SEND_ERROR "MIDASSYS environment variable not defined.")
endif()
if (NOT DEFINED ENV{EPICSSYS})
   message(SEND_ERROR "EPICSSYS environment variable not defined.")
endif()
if (NOT DEFINED ENV{EPICS_HOST_ARCH})
   message(SEND_ERROR "EPICS_HOST_ARCH environment variable not defined.")
endif()

set(CMAKE_CXX_STANDARD 11)
set(MIDASSYS $ENV{MIDASSYS})
set(EPICSSYS $ENV{EPICSSYS})
set(EPICS_HOST_ARCH $ENV{EPICS_HOST_ARCH})

if (${CMAKE_SYSTEM_NAME} MATCHES Linux)
   link_directories(${EPICSSYS}/lib/${EPICS_HOST_ARCH})
   set(LIBS ${LIBS} -lpthread -lutil -lrt -lca -ldl)
endif()
if (${CMAKE_SYSTEM_NAME} MATCHES Darwin)
   link_directories(${EPICSSYS}/lib/darwin-aarch64)
   set(LIBS ${LIBS} -lutil -lca)
endif()

add_executable(epics_fe epics_fe.cxx
   ${MIDASSYS}/src/mfe.cxx
   ${MIDASSYS}/src/mfed.cxx
   ${MIDASSYS}/src/odbxx.cxx
   )

target_include_directories(epics_fe PRIVATE
   ${MIDASSYS}/include
   ${MIDASSYS}/mxml
   ${MIDASSYS}/mscb/include
   ${EPICSSYS}/include
   ${EPICSSYS}/include/os/Linux
   ${EPICSSYS}/include/os/Darwin
   ${EPICSSYS}/include/compiler/gcc
   ${EPICSSYS}/include/compiler/clang
   )

target_link_libraries(epics_fe
   ${MIDASSYS}/lib/libmfe.a
   ${MIDASSYS}/lib/libmidas.a
   ${LIBS}
   )
