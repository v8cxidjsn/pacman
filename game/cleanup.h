/**
 * @file cleanup.h
 * @brief Helpers for resetting gameplay state, map data, and ghost spawns.
 * @author Oskar Dudziak
 */

#ifndef PACMAN_3D_CLEANUP_H
#define PACMAN_3D_CLEANUP_H

#include "pacman.h"
#include "ghost.h"
#include "map/map_settings.h"
#include "map/map_generate.h"
#include <vector>
#include <utility>
#include <algorithm>
#include <random>
#include <stdlib.h>
#include <math.h>

extern Pacman pacman;
extern bool isGameOver;
extern bool isGameWon;
extern bool huntMode;
extern bool godMode;
extern int eatenCrystals;
extern int totalCrystals;

extern std::vector<Ghost> ghosts;
extern std::vector<bool> ghostEaten;

/**
 * @brief Restores a fresh game state and generates a new playable maze.
 */
inline void resetGame() {
    pacman.x = 1.0f;
    pacman.z = 1.0f;
    pacman.angle = 180.0f;
    isGameOver = false;
    isGameWon = false;
    huntMode = false;
    godMode = false;
    eatenCrystals = 0;
    totalCrystals = 0;

    generateRandomMap();

    int mapArea = MAP_WIDTH * MAP_HEIGHT;
    int numGhosts = mapArea / 40;
    if (numGhosts < 2) numGhosts = 2;

    ghosts.clear();
    ghostEaten.clear();

    std::vector<std::pair<int, int>> safeSpawnPoints;
    for (int z = 1; z < MAP_HEIGHT - 1; z++) {
        for (int x = 1; x < MAP_WIDTH - 1; x++) {
            if (levelMap[z][x] == FIELD_CRYSTAL) {
                float dist = sqrtf(powf((float)x - 1.0f, 2.0f) + powf((float)z - 1.0f, 2.0f));
                if (dist >= 6.0f) {
                    safeSpawnPoints.push_back({x, z});
                }
            }
        }
    }

    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(safeSpawnPoints.begin(), safeSpawnPoints.end(), g);

    int availablePoints = (int)safeSpawnPoints.size();
    int toSpawn = (availablePoints < numGhosts) ? availablePoints : numGhosts;

    for (int i = 0; i < toSpawn; i++) {
        Ghost newGhost;
        newGhost.x = (float)safeSpawnPoints[i].first;
        newGhost.z = (float)safeSpawnPoints[i].second;
        newGhost.angle = (float)(rand() % 4) * 90.0f;
        newGhost.state = PATROL;
        newGhost.targetX = 0.0f;
        newGhost.targetZ = 0.0f;

        ghosts.push_back(newGhost);
        ghostEaten.push_back(false);
    }

    for (int z = 0; z < MAP_HEIGHT; z++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            if (levelMap[z][x] == FIELD_CRYSTAL) {
                totalCrystals++;
            }
        }
    }
}

#endif
