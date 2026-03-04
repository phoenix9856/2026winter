
cmake_minimum_required(VERSION 3.10)
project(Orangepi_Core_Project)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(DEV_ROOT /home/HwHiAiUser/2026winter/edge.git)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_BUILD_TYPE Debug)

# 设置包含目录
include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/EMPT
        ${DEV_ROOT}/3rds/include
        ${DEV_ROOT}/3rds/class_loader/include
        ${DEV_ROOT}/plugin/include
        ${DEV_ROOT}/plugin/DetectSink/include
        ${DEV_ROOT}/plugin/Identification/include
        ${DEV_ROOT}/plugin/RadarIQ2PDW/include
        ${DEV_ROOT}/plugin/RadarIdentification/include
        ${DEV_ROOT}/plugin/RadarSignalSorting/include
        ${DEV_ROOT}/plugin/DetectAndClass/include
        ${DEV_ROOT}/plugin/DetectAndSperate/include
        ${DEV_ROOT}/plugin/Detection/include
        /usr/include
        /usr/include/nlohmann
        /usr/include/boost
)

# 查找必要的库
find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
find_library(GLOG_LIBRARY glog)
find_library(BOOST_SYSTEM_LIBRARY boost_system)
find_library(CLASS_LOADER_LIBRARY class_loader)

# 设置源文件
set(SOURCES
        main.cpp
        src/TCPServer.cpp
        src/ProtocolHandler.cpp
        src/ClientManager.cpp 
        src/NetworkUtils.cpp   
        src/CommandInterface.cpp
        src/GlobalCollectState.cpp  # 添加这行
)

# 添加可执行文件
add_executable(${PROJECT_NAME} ${SOURCES})

# 链接库
target_link_libraries(${PROJECT_NAME}
        ${GLOG_LIBRARY}
        ${BOOST_SYSTEM_LIBRARY}
        ${CLASS_LOADER_LIBRARY}
        Python3::Python
        console_bridge
        pthread
        readline
        dl
        rt
)