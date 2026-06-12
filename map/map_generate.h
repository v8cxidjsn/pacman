/**
 * @file map_generate.h
 * @brief Procedural maze generation for the Pac-Man 3D level.
 * @author Oskar Dudziak
 */

#ifndef PACMAN_3D_MAP_GENERATE_H
#define PACMAN_3D_MAP_GENERATE_H

#include <stdlib.h>
#include <algorithm>
#include "map/map_settings.h"

/**
 * @brief Recursively carves maze corridors starting from the given tile.
 * @param cx Current tile X coordinate.
 * @param cy Current tile Z coordinate.
 */
inline void carveMaze(int cx, int cy) {
    levelMap[cy][cx] = FIELD_EMPTY;

    int directions[4][2] = {
        {0, -2},
        {0, 2},
        {-2, 0},
        {2, 0}
    };

    for (int i = 0; i < 4; i++) {
        int r = rand() % 4;

        std::swap(directions[i][0], directions[r][0]);
        std::swap(directions[i][1], directions[r][1]);
    }

    for (int i = 0; i < 4; i++) {
        int nx = cx + directions[i][0];
        int ny = cy + directions[i][1];

        if (
            nx > 0 &&
            nx < MAP_WIDTH - 1 &&
            ny > 0 &&
            ny < MAP_HEIGHT - 1 &&
            levelMap[ny][nx] == FIELD_WALL
        ) {
            levelMap[cy + directions[i][1] / 2][cx + directions[i][0] / 2] = FIELD_EMPTY;
            carveMaze(nx, ny);
        }
    }
}

/**
 * @brief Fills the level map with a randomized maze and collectible crystals.
 */
inline void generateProceduralMap() {
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            levelMap[y][x] = FIELD_WALL;
        }
    }

    carveMaze(1, 1);

    int loopsToAdd = (int) (0.72 * MAP_HEIGHT);
    while (loopsToAdd > 0) {
        int rx = 1 + rand() % (MAP_WIDTH - 2);
        int ry = 1 + rand() % (MAP_HEIGHT - 2);

        if (levelMap[ry][rx] == FIELD_WALL) {
            levelMap[ry][rx] = FIELD_EMPTY;
            loopsToAdd--;
        }
    }

    for (int y = 1; y < MAP_HEIGHT - 1; y++) {
        for (int x = 1; x < MAP_WIDTH - 1; x++) {
            if (levelMap[y][x] == FIELD_EMPTY) {
                levelMap[y][x] = FIELD_CRYSTAL;
            }
        }
    }

    levelMap[1][1] = FIELD_EMPTY;

    if (MAP_WIDTH > 3 && MAP_HEIGHT > 3) {
        if (levelMap[1][MAP_WIDTH - 2] != FIELD_WALL) {
            levelMap[1][MAP_WIDTH - 2] = FIELD_CRYSTAL;
        }

        if (levelMap[MAP_HEIGHT - 2][1] != FIELD_WALL) {
            levelMap[MAP_HEIGHT - 2][1] = FIELD_CRYSTAL;
        }

        if (levelMap[MAP_HEIGHT - 2][MAP_WIDTH - 2] != FIELD_WALL) {
            levelMap[MAP_HEIGHT - 2][MAP_WIDTH - 2] = FIELD_CRYSTAL;
        }
    }

    int startX = MAP_WIDTH / 2;
    int startY = MAP_HEIGHT / 2;

    if (
        startX > 0 &&
        startX < MAP_WIDTH - 1 &&
        startY > 0 &&
        startY < MAP_HEIGHT - 1
    ) {
        levelMap[startY][startX] = FIELD_EMPTY;
    }
}

/**
 * @brief Generates a new map using the procedural maze generator.
 */
inline void generateRandomMap() {
    generateProceduralMap();
}

#endif
