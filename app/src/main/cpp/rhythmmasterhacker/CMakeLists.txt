cmake_minimum_required(VERSION 3.22.1)
project(rhythmmasterhacker)

add_library(${PROJECT_NAME} SHARED
        utils.cpp
        rhythmmaster.cpp)

target_link_libraries(${PROJECT_NAME}
        android
        log
        dobby)