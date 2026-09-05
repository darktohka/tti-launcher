cmake_minimum_required(VERSION 3.16)

project(tti-launcher VERSION 1.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)
find_package(BZip2 REQUIRED)

add_executable(tti-launcher
    src/main.cpp
    src/draggablewindow.cpp
    src/draggablewindow.h
    src/launcherwindow.cpp
    src/launcherwindow.h
    src/updater.cpp
    src/updater.h
    src/resources.qrc
)

target_include_directories(tti-launcher PRIVATE src)

target_link_libraries(tti-launcher PRIVATE
    Qt6::Widgets
    Qt6::Network
    BZip2::BZip2
)

set_target_properties(tti-launcher PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

install(TARGETS tti-launcher
    RUNTIME DESTINATION .
    BUNDLE DESTINATION .
)