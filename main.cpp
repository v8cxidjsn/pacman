/**
 * @file main.cpp
 * @brief Application entry point and GLFW/GLEW setup for Pac-Man 3D.
 * @author Oskar Dudziak
 */

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include "game/game.h"

/** @brief Time in seconds between the current and previous frame. */
float deltaTime = 0.0f;

/** @brief Timestamp of the previous rendered frame. */
float lastFrame = 0.0f;

/**
 * @brief Creates the window, initializes OpenGL, and runs the main game loop.
 * @return Process exit code.
 */
int main(void) {
    GLFWwindow* window;

    glfwSetErrorCallback(error_callback);

    if (!glfwInit()) {
        fprintf(stderr, "Nie mozna zainicjowac GLFW.\n");
        exit(EXIT_FAILURE);
    }

    window = glfwCreateWindow(1280, 720, "Pac-Man 3D", NULL, NULL);

    if (!window) {
        fprintf(stderr, "Nie mozna utworzyc okna.\n");
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Nie mozna zainicjowac GLEW.\n");
        exit(EXIT_FAILURE);
    }

    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    initGame();
    initOpenGLProgram(window);

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = (float) glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        if (deltaTime > 0.1f) deltaTime = 0.1f;

        updateGame(window);
        drawScene(window);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    freeOpenGLProgram(window);

    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}
