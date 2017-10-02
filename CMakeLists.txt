cmake_minimum_required (VERSION 2.8)
project (nylonsock)

# set(CMAKE_BUILD_TYPE Debug)

option(BUILD_TESTS "Build test programs" ON)
option(BUILD_STATIC "Builds a static library if enabled. Otherwise builds a shared library" OFF)

IF (NOT WIN32)
set(INCLUDE_DIR ${CMAKE_BINARY_DIR}/${PROJECT_NAME})
set(LIB_DIR ${CMAKE_BINARY_DIR}/${PROJECT_NAME}/lib)
set(TEST_DIR ${CMAKE_BINARY_DIR}/${PROJECT_NAME}/test)

file(MAKE_DIRECTORY ${INCLUDE_DIR})
file(MAKE_DIRECTORY ${LIB_DIR})
file(MAKE_DIRECTORY ${TEST_DIR})

file(COPY NylonSock/include DESTINATION ${INCLUDE_DIR})
set(EXECUTABLE_OUTPUT_PATH ${TEST_DIR})

ENDIF(NOT WIN32)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y -pthread")

include_directories("${PROJECT_SOURCE_DIR}/NylonSock/include/")
include_directories("${PROJECT_SOURCE_DIR}/NylonSock/src/")


IF (WIN32)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
ENDIF(WIN32)

set(SRCS
NylonSock/src/Socket.cpp
)

if (BUILD_STATIC)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY  ${LIB_DIR})
add_library(${PROJECT_NAME} STATIC ${SRCS})

else(BUILD_STATIC)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${LIB_DIR})
add_library(${PROJECT_NAME} SHARED ${SRCS})
ENDIF (BUILD_STATIC)

IF (WIN32)
target_link_libraries(nylonsock ws2_32)
ENDIF(WIN32)

IF (BUILD_TESTS)
add_executable(TestServer NylonSock/test/testserver.cpp)
add_executable(TestClient NylonSock/test/testclient.cpp)

target_link_libraries(TestServer ${PROJECT_NAME})
target_link_libraries(TestClient ${PROJECT_NAME})

IF (NOT WIN32)
INSTALL(TARGETS TestServer TestClient DESTINATION ${TEST_DIR})
ENDIF(NOT WIN32)

ENDIF (BUILD_TESTS)
