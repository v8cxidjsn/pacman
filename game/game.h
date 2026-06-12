/**
 * @file game.h
 * @brief Public game lifecycle, input, update, and rendering functions.
 * @author Oskar Dudziak
 */

#ifndef PACMAN_3D_GAME_H
#define PACMAN_3D_GAME_H

struct GLFWwindow;

/**
 * @brief Handles GLFW errors by printing their description.
 * @param error GLFW error code.
 * @param description Human-readable error description.
 */
void error_callback(int error, const char* description);

/**
 * @brief Updates camera direction from mouse cursor movement.
 * @param window Active GLFW window.
 * @param xpos Current cursor X position.
 * @param ypos Current cursor Y position.
 */
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);

/** @brief Initializes random seed and resets gameplay state. */
void initGame();

/**
 * @brief Loads OpenGL resources needed by the game.
 * @param window Active GLFW window.
 */
void initOpenGLProgram(GLFWwindow* window);

/**
 * @brief Releases OpenGL resources and loaded models.
 * @param window Active GLFW window.
 */
void freeOpenGLProgram(GLFWwindow* window);

/**
 * @brief Processes one simulation step for input and AI.
 * @param window Active GLFW window.
 */
void updateGame(GLFWwindow* window);

/**
 * @brief Renders one complete frame of the game.
 * @param window Active GLFW window.
 */
void drawScene(GLFWwindow* window);

#endif
