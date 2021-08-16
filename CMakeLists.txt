cmake_minimum_required(VERSION 3.10)
project(OBDBasedPSI)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O3 -msse4.2")
INCLUDE_DIRECTORIES($ENV{HOME}
#        $ENV{HOME}/PaXoS_PSI_New $ENV{HOME}/PaXoS_PSI_New/libPSI $ENV{HOME}/PaXoS_PSI_New/libOTe
#        $ENV{HOME}/PaXoS_PSI_New/libOTe/cryptoTools
        $ENV{HOME}/thirdparty/linbox/linbox
#        $ENV{HOME}/libscapi/install/include
        xxHash)
#
#link_directories(/usr/ssl/lib/ $ENV{HOME}/PaXoS_PSI_New/lib $ENV{HOME}/PaXoS_PSI_New/libOTe/lib
#        $ENV{HOME}/libscapi/install/lib xxHash/)

set(SOURCE_FILES main.cpp ObliviousDictionary.cpp ProtocolParty.cpp gf2e_mat_solve.cpp)


add_executable(OBDBasedPSI ${SOURCE_FILES})

#find_library(LUA_LIB $ENV{HOME}/PaXoS_PSI/libOTe/lib/libOTe.a)
#if($ENV{HOME}/PaXoS_PSI/libOTe/lib/libOTe.a)
#    message(FATAL_ERROR "lua library not found")
#    else()
#    message(FATAL_ERROR "lua library found")
#endif()


# default location for NTL
set(NTL_Dirs "${CMAKE_SOURCE_DIR}/thirdparty/linux/ntl/")
include_directories("${NTL_Dirs}/include/")
link_directories("${NTL_Dirs}/src/")
set(NTL_Lib_Dirs "${NTL_Dirs}/src/")
set(NTL_Include_Dirs "${NTL_Dirs}/include/")

# default location for NTL
set(NTL_LIB "${NTL_Lib_Dirs}libntl.a")


if(NOT EXISTS "${NTL_LIB}")
    # we failed to find it, try using find_library
    find_library(
            NTL_LIB
            NAMES ntl
            HINTS NTL_Lib_Dirs)

    # if we still cant fint it, throw an error
    if(NOT NTL_LIB)
        Message(${NTL_LIB})
        message(FATAL_ERROR "Failed to find NTL at ${NTL_Lib_Dirs} or at system locations." )
    endif()
else()
    include_directories(${NTL_Include_Dirs})
    #message(" ----  NTL -----  ${NTL_Include_Dirs}")
endif()


set(linbox_Dirs "${CMAKE_SOURCE_DIR}/thirdparty/linbox/")
find_library(
        linbox
        NAMES linbox
        HINTS "${linbox_Dirs}")
include_directories("${linbox_Dirs}/linbox/")
link_directories("${linbox_Dirs}/linbox/")

TARGET_LINK_LIBRARIES(OBDBasedPSI ntl.a libxxhash.so linbox iml libntl.a
#        liblibPSI.a
#        $ENV{HOME}/PaXoS_PSI_New/libOTe/lib/liblibOTe.a
#        $ENV{HOME}/PaXoS_PSI_New/libOTe/lib/libcryptoTools.a
#        $ENV{HOME}/libscapi/libscapi.a gmp
        boost_system pthread crypto dl)
