cmake_minimum_required (VERSION 2.8)
project (nylonsock)

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

add_executable(NylonSockTest NylonSock/test/testserver.cpp)
target_link_libraries(NylonSockTest ${PROJECT_NAME})