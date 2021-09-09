cmake_minimum_required(VERSION 3.10)
project(OBDBasedPSI)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O3 -msse4.2 -fPIE")
set(SOURCE_FILES main.cpp ObliviousDictionary.cpp gf2e_mat_solve.cpp)


add_executable(OBDBasedPSI ${SOURCE_FILES})

include_directories(./xxHash)

target_link_directories(OBDBasedPSI
	PUBLIC	./xxHash
)

TARGET_LINK_LIBRARIES(OBDBasedPSI
		ntl
		xxhash
		linbox
		iml
		boost_system
		pthread
		crypto
		dl
		blas
		givaro
		gmp
		gomp
)