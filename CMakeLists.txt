cmake_minimum_required(VERSION 3.16)

project(QtChatApp3 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets Network)

add_library(ChatShared STATIC
    Shared/ChatMessage.cpp
    Shared/ChatMessage.h
)
target_link_libraries(ChatShared
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Network
)

target_include_directories(ChatShared PUBLIC Shared)

add_subdirectory(Client/QtChatClient)
add_subdirectory(Server/QtChatServer)
