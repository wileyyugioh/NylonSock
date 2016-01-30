cmake_minimum_required (VERSION 2.8)
project (nylonsock)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y")

include_directories("${PROJECT_SOURCE_DIR}/NylonSock/include/")
include_directories("${PROJECT_SOURCE_DIR}/NylonSock/include/NylonSock")

IF (WIN32)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
ENDIF()

set(SRCS
NylonSock/src/Socket.cpp
NylonSock/src/Sustainable.cpp
)

add_library(${PROJECT_NAME} SHARED ${SRCS})

add_executable(TestServer NylonSock/test/testserver.cpp)
add_executable(TestClient NylonSock/test/testclient.cpp)

target_link_libraries(TestServer ${PROJECT_NAME})
target_link_libraries(TestClient ${PROJECT_NAME})
