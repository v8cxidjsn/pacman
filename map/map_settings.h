/**
 * @file map_settings.h
 * @brief Map dimensions, tile types, and shared map storage.
 * @author Oskar Dudziak
 */

#ifndef PACMAN_3D_MAP_SETTINGS_H
#define PACMAN_3D_MAP_SETTINGS_H

/** @brief Number of tiles along the X axis. */
inline const int MAP_WIDTH = 10;

/** @brief Number of tiles along the Z axis. */
inline const int MAP_HEIGHT = 10;

/**
 * @brief Tile types used by the generated maze.
 */
enum FieldType {
    /** Empty walkable tile. */
    FIELD_EMPTY = -1,

    /** Walkable tile containing a collectible crystal. */
    FIELD_CRYSTAL = 0,

    /** Blocking wall tile. */
    FIELD_WALL = 1
};

/** @brief Current level grid indexed as levelMap[z][x]. */
inline int levelMap[MAP_HEIGHT][MAP_WIDTH] = {};

#endif //PACMAN_3D_MAP_SETTINGS_H
