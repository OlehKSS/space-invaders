cmake_minimum_required(VERSION 3.20)

project(invaders)

set(CMAKE_CXX_STANDARD 23)

find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

add_executable(invaders
    main.cpp
)

target_link_libraries(invaders glfw OpenGL::GL GLEW::glew)