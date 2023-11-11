cmake_minimum_required(VERSION 3.22.1)
project(nikkihacker)

add_library(${PROJECT_NAME} SHARED
        utils.cpp
        nikki.cpp)

target_link_libraries(${PROJECT_NAME}
        android
        log
        dobby)