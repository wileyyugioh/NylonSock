cmake_minimum_required (VERSION 3.1)
project (nylonsock)

IF (NOT WIN32)
set(INCLUDE_DIR ${PROJECT_NAME})
set(LIB_DIR ${PROJECT_NAME}/lib)
set(TEST_DIR ${PROJECT_NAME}/test)

file(MAKE_DIRECTORY ${INCLUDE_DIR})
file(MAKE_DIRECTORY ${LIB_DIR})
file(MAKE_DIRECTORY ${TEST_DIR})

file(COPY NylonSock/include DESTINATION ${INCLUDE_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${LIB_DIR})
set(EXECUTABLE_OUTPUT_PATH ${TEST_DIR})

ENDIF()

set (CMAKE_CXX_STANDARD 14)
set (CMAKE_CXX_STANDARD_REQUIRED on)

include_directories("${PROJECT_SOURCE_DIR}/NylonSock/include/")
include_directories("${PROJECT_SOURCE_DIR}/NylonSock/src/")


IF (WIN32)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
ENDIF()

set(SRCS
NylonSock/src/Socket.cpp
NylonSock/src/Sustainable.cpp
)

add_library(${PROJECT_NAME} SHARED ${SRCS})

if (WIN32)
target_link_libraries(nylonsock ws2_32)
ENDIF()

add_executable(TestServer NylonSock/test/testserver.cpp)
add_executable(TestClient NylonSock/test/testclient.cpp)

target_link_libraries(TestServer ${PROJECT_NAME})
target_link_libraries(TestClient ${PROJECT_NAME})

IF (NOT WIN32)
INSTALL(TARGETS TestServer TestClient DESTINATION ${TEST_DIR})
ENDIF()
