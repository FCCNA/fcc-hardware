#
# Standard targets requiring only midas library
#
set(PROGS
   mserver
   mlogger
   mhist
   mstat
   mdump
   mtransition
   mhdump
   odbhist
   melog
   mh2sql
   hwtest
   tmfe_example_multithread
   tmfe_example_everything
   tmfe_example_frontend
   tmfe_example_indexed
   fetest
   test_sleep
   crc32c_sum
   lazylogger
)

find_package (Python3)
if (Python3_FOUND)
   configure_file(mmessenger.py.in  ${CMAKE_BINARY_DIR}/progs/mmessenger @ONLY)
   install(PROGRAMS ${CMAKE_BINARY_DIR}/progs/mmessenger DESTINATION bin)
endif()

if (NO_LOCAL_ROUTINES)
else()
   list(APPEND PROGS odbinit)
   list(APPEND PROGS mchart)
endif()

set(TESTPROGS
   mjson_test
   get_record_test
   odb_lock_test
)

set(MFEPROGS
   mfe_link_test
   mfe_link_test_cxx
   feudp
   msysmon
)

foreach(PROG ${PROGS} ${TESTPROGS})
   add_executable(${PROG} ${PROG}.cxx)
   target_link_libraries(${PROG} midas)
#   set(CMAKE_CXX_FLAGS "-fsanitize=address")
endforeach()

foreach(PROG ${MFEPROGS})
   add_executable(${PROG} ${PROG}.cxx)
   target_link_libraries(${PROG} mfe midas)
endforeach()

add_executable(msequencer msequencer.cxx)
target_link_libraries(msequencer midas)

if (HAVE_NVIDIA)
   add_executable(msysmon-nvidia msysmon)
   target_include_directories(msysmon-nvidia PRIVATE ${NVIDIA_INC})
   target_link_libraries(msysmon-nvidia mfe midas ${NVIDIA_LIBS})
   target_compile_options(msysmon-nvidia PRIVATE -DHAVE_NVIDIA)
endif(HAVE_NVIDIA)

if (HAVE_LM_SENSORS)
   add_executable(msysmon-lmsensors msysmon)
   target_link_libraries(msysmon-lmsensors mfe midas ${libsensors})
   target_compile_options(msysmon-lmsensors PRIVATE -DHAVE_LM_SENSORS)
endif(HAVE_LM_SENSORS)

#
# mhttpd
#
add_executable(mhttpd mhttpd.cxx mongoose616.cxx mgd.cxx ${PROJECT_SOURCE_DIR}/mscb/src/mscb.cxx)
target_compile_options(mhttpd PRIVATE -DHAVE_MSCB)
target_include_directories(mhttpd PRIVATE ${PROJECT_SOURCE_DIR}/mscb/include)

target_compile_options(mhttpd PRIVATE -DHAVE_MONGOOSE616 -DMG_ENABLE_THREADS -DMG_DISABLE_CGI -DMG_ENABLE_EXTRA_ERRORS_DESC -DMG_ENABLE_IPV6 -DMG_ENABLE_HTTP_URL_REWRITES -DMG_ENABLE_HTTP_SSI=0)

if (MBEDTLS_FOUND)
   #add_compile_options(-I../mbedtls/include)
   #add_compile_options(-I../../../mbedtls/include -I../../../mbedtls/crypto/include)
   #set(MBEDTLS_LIBS ${MBEDTLS_LIBS} -L../../../mbedtls/library -lmbedtls -lmbedx509 -L../../../mbedtls/crypto/library -lmbedcrypto)
   set(MBEDTLS_LIBS ${MBEDTLS_LIBS} mbedtls)
   target_compile_options(mhttpd PRIVATE -DMG_ENABLE_SSL -DMG_SSL_IF=2 -DMG_SSL_MBED_DUMMY_RANDOM -I../mbedtls/include)
   target_link_libraries(mhttpd midas ${MBEDTLS_LIBS})
elseif (OPENSSL_FOUND)
   target_compile_options(mhttpd PRIVATE -DMG_ENABLE_SSL -DMG_SSL_IF=1)
   target_include_directories(mhttpd PRIVATE ${OPENSSL_INCLUDE_DIR})
   target_link_libraries(mhttpd midas ${OPENSSL_LIBRARIES})
else ()
   target_link_libraries(mhttpd midas)
endif(MBEDTLS_FOUND)


#
# mhttpd6 (old mongoose)
#
add_executable(mhttpd6 mhttpd.cxx mongoose6.cxx mgd.cxx ${PROJECT_SOURCE_DIR}/mscb/src/mscb.cxx)
target_compile_options(mhttpd6 PRIVATE -DHAVE_MSCB)
target_include_directories(mhttpd6 PRIVATE ${PROJECT_SOURCE_DIR}/mscb/include)
target_include_directories(mhttpd6 PRIVATE ${OPENSSL_INCLUDE_DIR})

# mongoose flags
target_compile_options(mhttpd6 PRIVATE -DHAVE_MONGOOSE6 -DMG_ENABLE_THREADS -DMG_DISABLE_CGI -DMG_ENABLE_EXTRA_ERRORS_DESC)
if (OPENSSL_FOUND)
   target_compile_options(mhttpd6 PRIVATE -DMG_ENABLE_SSL)
   target_link_libraries(mhttpd6 ${OPENSSL_LIBRARIES})
endif(OPENSSL_FOUND)

target_link_libraries(mhttpd6 midas)

#
# odbedit
#

add_executable(odbedit odbedit.cxx cmdedit.cxx)
target_link_libraries(odbedit midas)

#
# lazylogger
#

if (HAVE_CURL)
   target_compile_options(lazylogger PRIVATE -DHAVE_CURL)   
endif (HAVE_CURL)

#
# rmlogger
#

if (HAVE_ROOT)
   add_executable(rmlogger mlogger.cxx)
   target_include_directories(rmlogger PUBLIC ${ROOT_INCLUDE_DIRS})
   target_compile_options(rmlogger PRIVATE -DHAVE_ROOT ${ROOT_CXX_FLAGS})
   target_link_libraries(rmlogger midas ${ROOT_LIBRARIES})
   set_target_properties(rmlogger PROPERTIES LINK_FLAGS "-Wl,-rpath,${ROOT_LIBRARY_DIR}")
endif(HAVE_ROOT)

#
# Installation
#

install(TARGETS ${PROGS} ${MFEPROGS} mhttpd mhttpd6 odbedit msequencer DESTINATION bin)

if (HAVE_NVIDIA)
   install(TARGETS msysmon-nvidia DESTINATION bin)
endif(HAVE_NVIDIA)

if (HAVE_ROOT)
   install(TARGETS rmlogger DESTINATION bin)
endif(HAVE_ROOT)
