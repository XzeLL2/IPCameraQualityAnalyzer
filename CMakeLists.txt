cmake_minimum_required(VERSION 3.10)
project(IPCameraQualityAnalyzer VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 REQUIRED COMPONENTS Widgets)
find_package(OpenCV REQUIRED)

message(STATUS "OpenCV include dirs: ${OpenCV_INCLUDE_DIRS}")
message(STATUS "OpenCV libraries: ${OpenCV_LIBS}")

add_executable(IPCameraQualityAnalyzer
    src/main.cpp
    src/mainwindow.h
    src/mainwindow.cpp
    src/mainwindow.ui
    src/cameraworker.h
    src/cameraworker.cpp
    src/imagequalityanalyzer.h
    src/imagequalityanalyzer.cpp
)

target_link_libraries(IPCameraQualityAnalyzer
    Qt5::Widgets
    ${OpenCV_LIBS}
)

target_include_directories(IPCameraQualityAnalyzer PRIVATE ${OpenCV_INCLUDE_DIRS})

# Настройка установки
install(TARGETS IPCameraQualityAnalyzer
    RUNTIME DESTINATION bin
)

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources.qrc")
    message(STATUS "Found resources.qrc")
endif()
