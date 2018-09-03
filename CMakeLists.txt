cmake_minimum_required(VERSION 2.8.11)

project(rtcdcpp)

set(CMAKE_CXX_STANDARD 14)

link_directories(${USRSCTP_LIB_DIR})
link_directories(${GLIB_LIB_DIR})
link_directories(${NICE_LIB_DIR})
link_directories(${OPENSSL_LIB_DIR})

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
	${GLIB_INCLUDE_DIR}/glib-2.0
	${GLIB_LIB_DIR}/glib-2.0/include
	${USRSCTP_INCLUDE_DIR}
	${NICE_INCLUDE_DIR}
	${OPENSSL_INCLUDE_DIR}
	${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME}
	usrsctp
	glib-2.0
	nice
	ssl
	${OPENSSL_LIB_DIR}/libcrypto.a
)
