cmake_minimum_required(VERSION 2.8)

project(qtetris)

find_package(Curses REQUIRED)
add_executable(${PROJECT_NAME} "main.cpp")
target_link_libraries(${PROJECT_NAME} ${CURSES_LIBRARIES})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR})
