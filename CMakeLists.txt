cmake_minimum_required(VERSION 3.20)
project(pacman_3d)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-DGLEW_STATIC)

include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/glew-2.3.1/include
        ${CMAKE_CURRENT_SOURCE_DIR}/glfw-3.4.bin/include
        ${CMAKE_CURRENT_SOURCE_DIR}/glm
)

add_executable(pacman_3d
        main.cpp
        images/lodepng.cpp
        shaders/shaderprogram.cpp
        models/cube.cpp
        models/sphere.cpp
        models/teapot.cpp
        models/torus.cpp
        models/model.cpp
        ghost.h
        pacman.h
        models/objmodel.cpp
        models/objmodel.h
        utils/fonts.h
        map/map_settings.h
        game/game.cpp
        game/game.h
        map/map_generate.h
        game/cleanup.h
)

target_link_libraries(pacman_3d
        PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/glfw-3.4.bin/lib-vc2022/Win64/glfw3.lib"
        "${CMAKE_CURRENT_SOURCE_DIR}/glew-2.3.1/lib/Release/x64/glew32s.lib"
        opengl32
        winmm
)