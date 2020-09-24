cmake_minimum_required(VERSION 3.10)
project(OBDBasedPSI)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O3 -msse4.2")
INCLUDE_DIRECTORIES($ENV{HOME} $ENV{HOME}/PaXoS_PSI_New $ENV{HOME}/PaXoS_PSI_New/libPSI $ENV{HOME}/PaXoS_PSI_New/libOTe
        $ENV{HOME}/PaXoS_PSI_New/libOTe/cryptoTools
        $ENV{HOME}/libscapi/install/include xxHash)

link_directories(/usr/ssl/lib/ $ENV{HOME}/PaXoS_PSI_New/lib $ENV{HOME}/PaXoS_PSI_New/libOTe/lib
        $ENV{HOME}/libscapi/install/lib xxHash/)

set(SOURCE_FILES main.cpp ObliviousDictionary.cpp ProtocolParty.cpp gf2e_mat_solve.cpp)


add_executable(OBDBasedPSI ${SOURCE_FILES})

#find_library(LUA_LIB $ENV{HOME}/PaXoS_PSI/libOTe/lib/libOTe.a)
#if($ENV{HOME}/PaXoS_PSI/libOTe/lib/libOTe.a)
#    message(FATAL_ERROR "lua library not found")
#    else()
#    message(FATAL_ERROR "lua library found")
#endif()

TARGET_LINK_LIBRARIES(OBDBasedPSI ntl.a libxxhash.so givaro blas linbox iml libntl.a
        liblibPSI.a
        $ENV{HOME}/PaXoS_PSI_New/libOTe/lib/liblibOTe.a
        $ENV{HOME}/PaXoS_PSI_New/libOTe/lib/libcryptoTools.a
        $ENV{HOME}/libscapi/libscapi.a gmp
        libboost_system.a pthread crypto dl)
