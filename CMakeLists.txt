cmake_minimum_required(VERSION 3.10)

project(Data_Process)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(DEV_ROOT ~/edge.git)
find_package(Qt5Core)

include_directories(
    include/EMPT
    /usr/include
    /usr/local/python3.7.5/include/python3.7m
    ${DEV_ROOT}/3rds/include
    ${DEV_ROOT}/3rds/class_loader/include
    ${DEV_ROOT}/plugin/include
    ${DEV_ROOT}/plugin/DetectAndSperateForLocation
    ${DEV_ROOT}/plugin/DetectSink
    ${DEV_ROOT}/plugin/Identification
    ${DEV_ROOT}/plugin/Location
    ${DEV_ROOT}/plugin/RadarIQ2PDW
    ${DEV_ROOT}/plugin/RadarIdentification
    ${DEV_ROOT}/plugin/RadarSignalSorting
    ${DEV_ROOT}/plugin/DetectAndClass
    ${DEV_ROOT}/plugin/DetectAndSperate
    ${DEV_ROOT}/plugin/Detection
    ${DEV_ROOT}/plugin/NodeDeployment
    ${DEV_ROOT}/plugin/GetIQDataFromSim
    ${DEV_ROOT}/simulator
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(
    ${PROJECT_NAME}
    "main.cpp"
    "src/pcie_console.cpp"
    )
link_directories(${PROJECT_NAME}
    /usr/lib/aarch64-linux-gnu
    /usr/local/python3.7.5/lib)
#LINK_DIRECTORIES(/usr/local/python3.7.5/lib)
target_link_libraries(${PROJECT_NAME} Qt5::Core glog  boost_system console_bridge class_loader /usr/local/python3.7.5/lib/libpython3.7m.so)
