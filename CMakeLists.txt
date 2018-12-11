cmake_minimum_required(VERSION 2.8.11)

project(rtcdcpp)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/Modules")
find_package(OpenSSL REQUIRED)
find_package(UsrSCTP REQUIRED)

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

link_directories(${OPENSSL_LIB_DIR})

set(DEPEND_INCLUDES
	${NICE_INCLUDE_DIR}
	${GLIB_INCLUDE_DIR}
	${GLIBCONFIG_INCLUDE_DIR}
	)

target_include_directories(${PROJECT_NAME} PUBLIC
	${DEPEND_INCLUDES}
	${CMAKE_SOURCE_DIR}/include
	${OPENSSL_INCLUDE_DIR}
)

target_link_libraries(${PROJECT_NAME}
	${DEPEND_LIBS}
	${OPENSSL_LIBRARIES}
	SctpLab::UsrSCTP
)

if(WIN32)
  target_link_libraries(${PROJECT_NAME} ws2_32 iphlpapi)
endif()
