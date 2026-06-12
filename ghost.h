/**
 * @file ghost.h
 * @brief Ghost state definitions used by AI, collision, and rendering code.
 * @author Oskar Dudziak
 */

#ifndef PACMAN_3D_GHOST_H
#define PACMAN_3D_GHOST_H

/**
 * @brief Describes the current behavior mode of a ghost.
 */
enum GhostState {
    /** Ghost moves through the maze without a direct target. */
    PATROL,

    /** Ghost can see Pac-Man and moves toward or away from him. */
    CHASE,

    /** Ghost moves toward the last known Pac-Man position. */
    FINDING
};

/**
 * @brief Stores one ghost's position, direction, behavior, and target.
 */
struct Ghost {
    /** @brief Current tile-space horizontal coordinates. */
    float x, z;

    /** @brief Current facing angle in degrees. */
    float angle;

    /** @brief Active AI behavior state. */
    GhostState state;

    /** @brief Tile-space destination used while chasing or searching. */
    float targetX, targetZ;
};

#endif
