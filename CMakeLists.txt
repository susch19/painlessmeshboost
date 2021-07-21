cmake_minimum_required (VERSION 3.5)
project (painlessMeshBoost)

add_definitions(-DLINUX_ENVIRONMENT -DCONFIG_CTRL_IFACE -DCONFIG_CTRL_IFACE_UNIX)
set(USE_CLANG ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

SET(Boost_USE_STATIC_LIBS OFF)
SET(Boost_USE_MULTITHREAD ON)

FIND_PACKAGE(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})

FIND_PACKAGE(Boost 1.60.0 REQUIRED COMPONENTS filesystem system thread regex program_options)
IF(Boost_FOUND)
  INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
  LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})
ENDIF(Boost_FOUND)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -g")


ADD_EXECUTABLE(painlessMeshBoost 
src/main.cpp src/fake_serial.cpp src/chat_client.cpp src/filesystemwatcher.cpp  src/LittleFSWrapper.cpp
libs/painlessmesh/src/painlessMeshConnection.cpp libs/painlessmesh/src/painlessmesh/packageTypeProvider.cpp libs/painlessmesh/src/scheduler.cpp
libs/PainlessMeshSmarthome/src/Device.cpp libs/PainlessMeshSmarthome/src/painlessMeshPlugins/MeshDevice.cpp libs/PainlessMeshSmarthome/src/painlessMeshPlugins/MeshOta.cpp libs/PainlessMeshSmarthome/src/Ota.cpp
libs/wpasupplicant/src/utils/os_unix.c libs/wpasupplicant/src/common/wpa_ctrl.c)

target_include_directories(painlessMeshBoost PUBLIC src/ libs/wpasupplicant/src/ 
libs/wpasupplicant/src/utils
libs/esptility/src/ libs/TaskScheduler/src/ libs/ArduinoJson/ libs/uuid/include/ 
libs/painlessmesh/src/ libs/PainlessMeshSmarthome/src/ libs/PainlessMeshSmarthome/src/painlessMeshPlugins)

TARGET_LINK_LIBRARIES(painlessMeshBoost ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES})

install(TARGETS painlessMeshBoost DESTINATION bin)
