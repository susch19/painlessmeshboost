cmake_minimum_required (VERSION 2.6)
project (painlessMeshBoost)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

SET(Boost_USE_STATIC_LIBS OFF)
SET(Boost_USE_MULTITHREAD ON)

FIND_PACKAGE(Boost 1.42.0 REQUIRED COMPONENTS system thread regex program_options)
IF(Boost_FOUND)
  INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
  LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})
ENDIF(Boost_FOUND)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

ADD_EXECUTABLE(first src/first.cpp)
target_include_directories(first PUBLIC src/ )
TARGET_LINK_LIBRARIES(first ${Boost_LIBRARIES})

ADD_EXECUTABLE(asyncclient src/asyncclient.cpp)
target_include_directories(asyncclient PUBLIC src/ )
TARGET_LINK_LIBRARIES(asyncclient ${Boost_LIBRARIES})

ADD_EXECUTABLE(asyncserver src/asyncserver.cpp)
target_include_directories(asyncserver PUBLIC src/ )
TARGET_LINK_LIBRARIES(asyncserver ${Boost_LIBRARIES})

ADD_EXECUTABLE(client src/client.cpp)
target_include_directories(client PUBLIC src/ )
TARGET_LINK_LIBRARIES(client ${Boost_LIBRARIES})

ADD_EXECUTABLE(server src/server.cpp)
target_include_directories(server PUBLIC src/ )
TARGET_LINK_LIBRARIES(server ${Boost_LIBRARIES})

ADD_EXECUTABLE(server2 src/server2.cpp)
target_include_directories(server2 PUBLIC src/ )
TARGET_LINK_LIBRARIES(server2 ${Boost_LIBRARIES})

