/**
 * @file pacman.h
 * @brief Player state used by movement, collision, and rendering code.
 * @author Oskar Dudziak
 */

#ifndef PACMAN_3D_PACMAN_H
#define PACMAN_3D_PACMAN_H

/**
 * @brief Stores Pac-Man position, viewing angle, and movement speed.
 */
struct Pacman {
    /** @brief Current tile-space horizontal coordinates. */
    float x, z;

    /** @brief Current facing angle in degrees. */
    float angle;

    /** @brief Movement speed in tiles per second. */
    float speed;
};

#endif //PACMAN_3D_PACMAN_H
