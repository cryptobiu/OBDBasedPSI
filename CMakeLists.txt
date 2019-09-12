cmake_minimum_required(VERSION 3.10)
project(OBDBasedPSI)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11 -mbmi2 -msse4.2 -march=native -m64 -pthread -fpermissive")
INCLUDE_DIRECTORIES($ENV{HOME} $ENV{HOME}/libscapi/install/include $ENV{HOME}/libOTe $ENV{HOME}/libOTe/cryptoTools xxHash
        $ENV{HOME}/libscapi/install/include/OTExtensionBristol )
link_directories(/usr/ssl/lib/ $ENV{HOME}/libscapi/install/lib  xxHash/cmake_unofficial)

set(SOURCE_FILES main.cpp ObliviousDictionary.cpp ProtocolParty.cpp gf2e_mat_solve.cpp)


add_executable(OBDBasedPSI main.cpp ObliviousDictionary.cpp ProtocolParty.cpp gf2e_mat_solve.cpp)

TARGET_LINK_LIBRARIES(OBDBasedPSI ntl.a libxxhash.so givaro  blas linbox ntl iml libOTe.a libcryptoTools.a
        $ENV{HOME}/libscapi/libscapi.a gmp
        boost_system pthread crypto dl)