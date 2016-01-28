cmake_minimum_required (VERSION 2.8)
project (nylonsock)

include_directories("${PROJECT_SOURCE_DIR}/NylonSock/include/")
include_directories("${PROJECT_SOURCE_DIR}/NylonSock/include/NylonSock")

set(SRCS
NylonSock/src/Socket.cpp
NylonSock/src/Sustainable.cpp
)

add_library(${PROJECT_NAME} SHARED ${SRCS})