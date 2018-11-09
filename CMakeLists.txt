cmake_minimum_required(VERSION 2.8.11)

project(rtcdcpp)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/Modules")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static-libgcc -static-libstdc++")
find_package(OpenSSL REQUIRED)
find_package(UsrSCTP REQUIRED)
find_package(LibNice REQUIRED)

link_directories(${OPENSSL_LIB_DIR})
link_directories(${NICE_LIB_DIR})

message("RTCDCPP includes: " ${INCLUDES})
message("RTCDCPP libs: " ${LIBS})

set(HEADERS
	include/Chunk.hpp
	include/ChunkQueue.hpp
	include/DataChannel.hpp
	include/DTLSWrapper.hpp
	include/NiceWrapper.hpp
	include/PeerConnection.hpp
	include/RTCCertificate.hpp
	include/SCTPWrapper.hpp
)

set(SOURCES
	src/DataChannel.cpp
	src/DTLSWrapper.cpp
	src/NiceWrapper.cpp
	src/PeerConnection.cpp
	src/RTCCertificate.cpp
	src/SCTPWrapper.cpp
)

add_library(${PROJECT_NAME} SHARED
	${HEADERS}
	${SOURCES}
)

target_include_directories(${PROJECT_NAME} PUBLIC
	${CMAKE_SOURCE_DIR}/include
	${OPENSSL_INCLUDE_DIR}
#	${INCLUDES}
)

target_link_libraries(${PROJECT_NAME}
	${OPENSSL_LIBRARIES}
	SctpLab::UsrSCTP
#	${LIBS}
	LibNice::LibNice
)

if(WIN32)
  target_link_libraries(${PROJECT_NAME} ws2_32)
endif()
