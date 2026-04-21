cmake_minimum_required(VERSION 3.20)
project(porchlight_edge VERSION 0.1.0 LANGUAGES CXX)

option(PORCHLIGHT_WITH_OPENCV "Enable OpenCV camera/video capture source" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SQLite3 REQUIRED)

add_library(porchlight_edge_core
    src/capture/frame_source.cpp
    src/config/config_manager.cpp
    src/inference/inference_engine.cpp
    src/logic/event_aggregator.cpp
    src/storage/local_event_store.cpp
    src/comms/http_client.cpp
    src/health/health_collector.cpp
    src/runtime/device_runtime.cpp
    src/util/json.cpp
    src/util/logger.cpp
)

target_include_directories(porchlight_edge_core PUBLIC include)
target_link_libraries(porchlight_edge_core PUBLIC SQLite::SQLite3)

target_compile_options(porchlight_edge_core PRIVATE -Wall -Wextra -Wpedantic)

if(PORCHLIGHT_WITH_OPENCV)
    find_package(OpenCV REQUIRED)
    target_compile_definitions(porchlight_edge_core PUBLIC PORCHLIGHT_WITH_OPENCV=1)
    target_sources(porchlight_edge_core PRIVATE src/capture/opencv_camera_source.cpp)
    target_link_libraries(porchlight_edge_core PUBLIC ${OpenCV_LIBS})
endif()

add_executable(porchlight_edge src/main.cpp)
target_link_libraries(porchlight_edge PRIVATE porchlight_edge_core)

include(CTest)
if(BUILD_TESTING)
    add_executable(test_event_logic tests/test_event_logic.cpp)
    target_link_libraries(test_event_logic PRIVATE porchlight_edge_core)
    add_test(NAME event_logic COMMAND test_event_logic)
endif()
