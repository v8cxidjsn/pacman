/**
 * @file game.cpp
 * @brief Main gameplay, AI, rendering, lighting, and shadow logic.
 * @author Oskar Dudziak
 */

#define GLM_FORCE_RADIANS

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <string>
#include <vector>
#include <windows.h>

#include "game.h"
#include "cleanup.h"
#include "utils/fonts.h"
#include "models/allmodels.h"
#include "models/objmodel.h"
#include "map/map_settings.h"
#include "shaders/shaderprogram.h"
#include "pacman.h"
#include "ghost.h"
#include "images/lodepng.h"

#pragma comment(lib, "winmm.lib")

extern float deltaTime;

GLuint wallTexture;
GLuint shadowMapFBO = 0;
GLuint shadowMapTexture = 0;
const unsigned int SHADOW_MAP_SIZE = 2048;

glm::vec3 lightDirection = glm::normalize(glm::vec3(-0.45f, -1.0f, -0.35f));
glm::mat4 lightSpaceMatrix = glm::mat4(1.0f);

/**
 * @brief Describes a ceiling lamp that lights the maze and casts blob shadows.
 */
struct CeilingLamp {
    /** @brief World-space lamp position. */
    glm::vec3 position;

    /** @brief Light color emitted by the lamp. */
    glm::vec3 color;

    /** @brief Maximum light reach in world units. */
    float radius;

    /** @brief Light strength multiplier. */
    float intensity;
};

/** @brief Number of ceiling lamps placed in the maze. */
const int CEILING_LAMP_COUNT = 5;

/** @brief Shared yellow color used by every ceiling lamp. */
const glm::vec3 CEILING_LAMP_COLOR = glm::vec3(1.0f, 0.86f, 0.25f);

/** @brief Ceiling lamps used for visible bulbs, lighting, and local shadows. */
CeilingLamp ceilingLamps[CEILING_LAMP_COUNT] = {
    {glm::vec3(2.0f, 2.12f, 2.0f), CEILING_LAMP_COLOR, 8.0f, 1.30f},
    {glm::vec3(16.0f, 2.12f, 2.0f), CEILING_LAMP_COLOR, 8.0f, 1.15f},
    {glm::vec3(2.0f, 2.12f, 16.0f), CEILING_LAMP_COLOR, 8.0f, 1.10f},
    {glm::vec3(16.0f, 2.12f, 16.0f), CEILING_LAMP_COLOR, 8.0f, 1.20f},
    {glm::vec3(9.0f, 2.12f, 9.0f), CEILING_LAMP_COLOR, 9.5f, 1.35f}
};

Pacman pacman = {1.0f, 1.0f, 180.0f, 4.0f};

float lastX = 400.0f;
float lastY = 400.0f;
bool firstMouse = true;
float camPitch = 20.0f;

bool isGameOver = false;
bool isGameWon = false;
bool huntMode = false;
bool godMode = false;
int totalCrystals = 0;
int eatenCrystals = 0;

std::vector<bool> ghostEaten;
std::vector<Ghost> ghosts;

glm::vec4 ghostColors[4] = {
    glm::vec4(1.0f, 0.0f, 0.0f, 1.0f), // Red
    glm::vec4(1.0f, 0.7f, 0.8f, 1.0f), // Pink
    glm::vec4(0.0f, 1.0f, 1.0f, 1.0f), // Aqua
    glm::vec4(1.0f, 0.5f, 0.0f, 1.0f)  // Orange
};

Models::ObjModel* ghostModel = nullptr;
Models::ObjModel* pacmanOpenModel = nullptr;
Models::ObjModel* pacmanClosedModel = nullptr;

/**
 * @brief Converts a world-space coordinate to a map tile index.
 * @param value World-space X or Z coordinate.
 * @return Tile index in the level map.
 */
int worldToTile(float value) {
    return (int)floor((value + 1.0f) * 0.5f);
}

/**
 * @brief Checks whether the requested tile blocks movement and light.
 * @param x Tile X coordinate.
 * @param z Tile Z coordinate.
 * @return True when the tile is outside the map or is a wall.
 */
bool isWallTile(int x, int z) {
    if (x < 0 || x >= MAP_WIDTH || z < 0 || z >= MAP_HEIGHT) {
        return true;
    }

    return levelMap[z][x] == FIELD_WALL;
}

/**
 * @brief Places lamps on the nearest available walkable tiles around anchors.
 */
void updateCeilingLampPositions() {
    int anchorX[CEILING_LAMP_COUNT] = {1, MAP_WIDTH - 2, 1, MAP_WIDTH - 2, MAP_WIDTH / 2};
    int anchorZ[CEILING_LAMP_COUNT] = {1, 1, MAP_HEIGHT - 2, MAP_HEIGHT - 2, MAP_HEIGHT / 2};
    int usedX[CEILING_LAMP_COUNT] = {};
    int usedZ[CEILING_LAMP_COUNT] = {};

    for (int i = 0; i < CEILING_LAMP_COUNT; ++i) {
        int bestX = anchorX[i];
        int bestZ = anchorZ[i];
        int bestScore = 999999;

        for (int z = 1; z < MAP_HEIGHT - 1; ++z) {
            for (int x = 1; x < MAP_WIDTH - 1; ++x) {
                if (isWallTile(x, z)) {
                    continue;
                }

                bool alreadyUsed = false;
                for (int j = 0; j < i; ++j) {
                    if (usedX[j] == x && usedZ[j] == z) {
                        alreadyUsed = true;
                        break;
                    }
                }

                int dx = x - anchorX[i];
                int dz = z - anchorZ[i];
                int score = dx * dx + dz * dz + (alreadyUsed ? 1000 : 0);
                if (score < bestScore) {
                    bestScore = score;
                    bestX = x;
                    bestZ = z;
                }
            }
        }

        usedX[i] = bestX;
        usedZ[i] = bestZ;
        ceilingLamps[i].position = glm::vec3(bestX * 2.0f, 2.12f, bestZ * 2.0f);
    }
}

/**
 * @brief Tests whether a lamp can light a point without a wall between them.
 * @param from World-space point being lit.
 * @param lampPosition World-space lamp position.
 * @return True when no wall tile blocks the lamp.
 */
bool hasLampLineOfSight(glm::vec3 from, glm::vec3 lampPosition) {
    int startX = worldToTile(from.x);
    int startZ = worldToTile(from.z);
    int endX = worldToTile(lampPosition.x);
    int endZ = worldToTile(lampPosition.z);

    if ((startX != endX || startZ != endZ) && isWallTile(endX, endZ)) {
        return false;
    }

    float dx = lampPosition.x - from.x;
    float dz = lampPosition.z - from.z;
    float distance = sqrt(dx * dx + dz * dz);
    int steps = (int)ceil(distance * 8.0f);
    if (steps < 1) steps = 1;
    if (steps > 64) steps = 64;

    for (int i = 1; i <= steps; ++i) {
        float t = (float)i / (float)steps;
        int x = worldToTile(from.x + dx * t);
        int z = worldToTile(from.z + dz * t);

        bool startCell = x == startX && z == startZ;
        bool endCell = x == endX && z == endZ;
        if (!startCell && !endCell && isWallTile(x, z)) {
            return false;
        }
    }

    return true;
}

/**
 * @brief Builds the light-space matrix for the legacy shadow map pass.
 * @return Projection-view matrix from the light direction.
 */
glm::mat4 calculateLightSpaceMatrix() {
    glm::vec3 sceneCenter((MAP_WIDTH - 1) * 1.0f, 0.8f, (MAP_HEIGHT - 1) * 1.0f);
    float sceneRadius = (MAP_WIDTH > MAP_HEIGHT ? MAP_WIDTH : MAP_HEIGHT) * 2.2f;
    glm::vec3 lightPos = sceneCenter - lightDirection * 24.0f;

    glm::mat4 lightProjection = glm::ortho(-sceneRadius, sceneRadius, -sceneRadius, sceneRadius, 1.0f, 60.0f);
    glm::mat4 lightView = glm::lookAt(lightPos, sceneCenter, glm::vec3(0.0f, 1.0f, 0.0f));

    return lightProjection * lightView;
}

/**
 * @brief Allocates the depth texture and framebuffer used by shadow mapping.
 */
void initShadowMap() {
    glGenFramebuffers(1, &shadowMapFBO);
    glGenTextures(1, &shadowMapTexture);

    glBindTexture(GL_TEXTURE_2D, shadowMapTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMapTexture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        printf("Shadow framebuffer is not complete\n");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

/**
 * @brief Sends lamp and wall-occlusion data to the constant shader.
 */
void setLightingUniforms() {
    int wallMap[MAP_WIDTH * MAP_HEIGHT];
    for (int z = 0; z < MAP_HEIGHT; ++z) {
        for (int x = 0; x < MAP_WIDTH; ++x) {
            wallMap[z * MAP_WIDTH + x] = levelMap[z][x] == FIELD_WALL ? 1 : 0;
        }
    }

    glm::vec3 lampPositions[CEILING_LAMP_COUNT];
    glm::vec3 lampColors[CEILING_LAMP_COUNT];
    float lampRadii[CEILING_LAMP_COUNT];
    float lampIntensities[CEILING_LAMP_COUNT];
    for (int i = 0; i < CEILING_LAMP_COUNT; ++i) {
        lampPositions[i] = ceilingLamps[i].position;
        lampColors[i] = ceilingLamps[i].color;
        lampRadii[i] = ceilingLamps[i].radius;
        lampIntensities[i] = ceilingLamps[i].intensity;
    }

    glUniform1i(spConstant->u("useLighting"), 1);
    glUniform1i(spConstant->u("useShadow"), 1);
    glUniform1f(spConstant->u("ambientStrength"), 0.10f);
    glUniform1i(spConstant->u("lampCount"), CEILING_LAMP_COUNT);
    glUniform3fv(spConstant->u("lampPositions[0]"), CEILING_LAMP_COUNT, glm::value_ptr(lampPositions[0]));
    glUniform3fv(spConstant->u("lampColors[0]"), CEILING_LAMP_COUNT, glm::value_ptr(lampColors[0]));
    glUniform1fv(spConstant->u("lampRadii[0]"), CEILING_LAMP_COUNT, lampRadii);
    glUniform1fv(spConstant->u("lampIntensities[0]"), CEILING_LAMP_COUNT, lampIntensities);
    glUniform1i(spConstant->u("mapWidth"), MAP_WIDTH);
    glUniform1i(spConstant->u("mapHeight"), MAP_HEIGHT);
    glUniform1iv(spConstant->u("wallMap[0]"), MAP_WIDTH * MAP_HEIGHT, wallMap);
}

/**
 * @brief Enables or disables lighting flags in the constant shader.
 * @param lighting Whether object lighting should be applied.
 * @param shadow Whether shader shadowing should be applied.
 */
void setLightingEnabled(bool lighting, bool shadow) {
    glUniform1i(spConstant->u("useLighting"), lighting ? 1 : 0);
    glUniform1i(spConstant->u("useShadow"), shadow ? 1 : 0);
}

/**
 * @brief Loads a PNG texture into an OpenGL texture object.
 * @param path Path to the PNG texture file.
 * @return OpenGL texture id, or 0 when loading fails.
 */
GLuint loadTexture(const char* path)
{
    std::vector<unsigned char> image;
    unsigned width, height;

    unsigned error = lodepng::decode(image, width, height, path);

    if (error)
    {
        printf("Texture error: %s\n", lodepng_error_text(error));
        return 0;
    }

    GLuint textureID;

    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        width,
        height,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        image.data()
    );

    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);

    printf("Loaded texture: %s (%u x %u)\n", path, width, height);

    return textureID;
}

/**
 * @brief Draws one textured wall cube at the requested transform.
 * @param cube Cube model used for drawing.
 * @param texture OpenGL texture id for the wall material.
 * @param position World-space wall center.
 * @param scale Wall scale vector.
 */
void drawWall (Models::Cube& cube, GLuint texture, glm::vec3 position, glm::vec3 scale) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    glUniform1i(spConstant->u("textureMap"), 0);
    glUniform1i(spConstant->u("useTexture"), 1);
    glUniform4f(spConstant->u("color"), 1.0f, 1.0f, 1.0f, 1.0f);

    glm::mat4 M = glm::mat4(1.0f);

    M = glm::translate(M, position);
    M = glm::scale(M, scale);

    glUniformMatrix4fv(
        spConstant->u("M"),
        1,
        GL_FALSE,
        glm::value_ptr(M)
    );

    cube.drawSolid();

    glBindTexture(GL_TEXTURE_2D, 0);
}

/**
 * @brief Draws text on screen using the built-in 3x5 bitmap font.
 * @param text Text to draw.
 * @param x Screen-space left position.
 * @param y Screen-space top position.
 * @param size Glyph scale.
 * @param color Text color.
 * @param cube Cube model used as a pixel block.
 */
void drawText(const std::string& text, float x, float y, float size, glm::vec4 color, Models::Cube& cube) {
    glUniform4f(spConstant->u("color"), color.r, color.g, color.b, color.a);
    glUniform1i(spConstant->u("useTexture"), 0);
    float startX = x;

    for (char c : text) {
        unsigned char uc = (unsigned char)toupper(c);
        const char* glyph = customFont[uc];
        if (!glyph) glyph = customFont['?'];

        for (int row = 0; row < 5; ++row) {
            for (int col = 0; col < 3; ++col) {
                if (glyph[row * 3 + col] == '#') {
                    glm::mat4 M = glm::mat4(1.0f);
                    M = glm::translate(M, glm::vec3(startX + col * size, y - row * size, 0.0f));
                    M = glm::scale(M, glm::vec3(size * 0.5f, size * 0.5f, 0.01f));
                    glUniformMatrix4fv(spConstant->u("M"), 1, false, glm::value_ptr(M));
                    cube.drawSolid();
                }
            }
        }
        startX += 4.0f * size;
    }
}

/**
 * @brief Checks whether two map positions have an unobstructed path.
 * @param x1 Start tile-space X coordinate.
 * @param z1 Start tile-space Z coordinate.
 * @param x2 End tile-space X coordinate.
 * @param z2 End tile-space Z coordinate.
 * @return True if no wall blocks the line segment.
 */
bool hasLineOfSight(float x1, float z1, float x2, float z2) {
    float dx = x2 - x1;
    float dz = z2 - z1;
    float dist = sqrt(dx * dx + dz * dz);
    if (dist < 0.1f) return true;
    dx /= dist;
    dz /= dist;

    float step = 0.1f;
    for (float d = 0.0f; d < dist; d += step) {
        float cx = x1 + dx * d;
        float cz = z1 + dz * d;
        int gX = round(cx);
        int gZ = round(cz);
        if (gX >= 0 && gX < MAP_WIDTH && gZ >= 0 && gZ < MAP_HEIGHT) {
            if (levelMap[gZ][gX] == FIELD_WALL) return false;
        } else {
            return false;
        }
    }
    return true;
}

/**
 * @brief Updates ghost AI, movement, collisions, and win/loss interactions.
 */
void updateGhosts() {
    float margin = 0.15f;

    for (size_t i = 0; i < ghosts.size(); i++) {
        if (ghostEaten[i]) continue;

        float ghostSpeed;
        if (huntMode) {
            ghostSpeed = ghosts[i].state == CHASE ? 2.5f : 1.8f;
        } else {
            ghostSpeed = ghosts[i].state == CHASE ? 3.0f : 2.2f;
        }
        ghostSpeed *= deltaTime;

        if (!huntMode) {
            if (hasLineOfSight(ghosts[i].x, ghosts[i].z, pacman.x, pacman.z)) {
                ghosts[i].state = CHASE;
                ghosts[i].targetX = pacman.x;
                ghosts[i].targetZ = pacman.z;
            } else if (ghosts[i].state == CHASE) {
                ghosts[i].state = FINDING;
            }
        } else {
            if (hasLineOfSight(ghosts[i].x, ghosts[i].z, pacman.x, pacman.z)) {
                ghosts[i].state = CHASE;
                ghosts[i].targetX = ghosts[i].x + (ghosts[i].x - pacman.x) * 10.0f;
                ghosts[i].targetZ = ghosts[i].z + (ghosts[i].z - pacman.z) * 10.0f;
            } else {
                ghosts[i].state = PATROL;
            }
        }

        if (ghosts[i].state == FINDING) {
            float distToTarget = sqrt(pow(ghosts[i].targetX - ghosts[i].x, 2) + pow(ghosts[i].targetZ - ghosts[i].z, 2));
            if (distToTarget < 0.2f) {
                ghosts[i].state = PATROL;
            }
        }

        int gX = round(ghosts[i].x);
        int gZ = round(ghosts[i].z);

        if (abs(sin(glm::radians(ghosts[i].angle))) < 0.1f) {
            ghosts[i].x += (gX - ghosts[i].x) * 0.2f;
        } else {
            ghosts[i].z += (gZ - ghosts[i].z) * 0.2f;
        }

        float tryX = ghosts[i].x + sin(glm::radians(ghosts[i].angle)) * ghostSpeed;
        float tryZ = ghosts[i].z + cos(glm::radians(ghosts[i].angle)) * ghostSpeed;

        bool canMove = true;
        int minX = round(tryX - margin);
        int maxX = round(tryX + margin);
        int minZ = round(tryZ - margin);
        int maxZ = round(tryZ + margin);

        for (int z = minZ; z <= maxZ; z++) {
            for (int x = minX; x <= maxX; x++) {
                if (x < 0 || x >= MAP_WIDTH || z < 0 || z >= MAP_HEIGHT || levelMap[z][x] == FIELD_WALL) {
                    canMove = false;
                }
            }
        }

        float dirs[] = {0.0f, 90.0f, 180.0f, 270.0f};
        int dxs[] = {0, 1, 0, -1};
        int dzs[] = {1, 0, -1, 0};

        std::vector<float> validDirs;
        for(int d = 0; d < 4; d++) {
            int checkX = gX + dxs[d];
            int checkZ = gZ + dzs[d];
            if (checkX >= 0 && checkX < MAP_WIDTH && checkZ >= 0 && checkZ < MAP_HEIGHT) {
                if (levelMap[checkZ][checkX] != FIELD_WALL) {
                    validDirs.push_back(dirs[d]);
                }
            }
        }

        if (validDirs.empty()) {
            validDirs.push_back(ghosts[i].angle);
        }

        float oppositeAngle = fmod(ghosts[i].angle + 180.0f, 360.0f);
        if (oppositeAngle < 0) oppositeAngle += 360.0f;

        std::vector<float> forwardDirs;
        for (float d : validDirs) {
            float diff = abs(fmod(d - oppositeAngle + 360.0f, 360.0f));
            if (diff > 1.0f && diff < 359.0f) {
                forwardDirs.push_back(d);
            }
        }
        if (forwardDirs.empty()) forwardDirs = validDirs;

        bool isIntersection = forwardDirs.size() > 1;
        bool nearCenter = (abs(ghosts[i].x - gX) < 0.05f && abs(ghosts[i].z - gZ) < 0.05f);

        if (!canMove || (isIntersection && nearCenter)) {
            float bestDir = ghosts[i].angle;

            if (ghosts[i].state == CHASE || ghosts[i].state == FINDING) {
                float bestDist = 99999.0f;
                for (float d : forwardDirs) {
                    float testX = gX + sin(glm::radians(d));
                    float testZ = gZ + cos(glm::radians(d));
                    float dist = sqrt(pow(ghosts[i].targetX - testX, 2) + pow(ghosts[i].targetZ - testZ, 2));
                    if (dist < bestDist) {
                        bestDist = dist;
                        bestDir = d;
                    }
                }
            } else {
                if (!forwardDirs.empty()) {
                    bestDir = forwardDirs[rand() % forwardDirs.size()];
                }
            }

            if (bestDir != ghosts[i].angle || !canMove) {
                ghosts[i].angle = bestDir;
                ghosts[i].x = gX;
                ghosts[i].z = gZ;
            } else {
                ghosts[i].x = tryX;
                ghosts[i].z = tryZ;
            }
        } else {
            ghosts[i].x = tryX;
            ghosts[i].z = tryZ;
        }

        float dx = ghosts[i].x - pacman.x;
        float dz = ghosts[i].z - pacman.z;
        float distance = sqrt(dx * dx + dz * dz);

        if (distance < 0.45f) {
            if (huntMode) {
                if (!ghostEaten[i]) {
                    ghostEaten[i] = true;

                    bool anyLeft = false;
                    for (bool eaten : ghostEaten) {
                        if (!eaten) {
                            anyLeft = true;
                            break;
                        }
                    }
                    if (anyLeft) {
                        PlaySound(TEXT("sounds/pacman_ghost_eating.wav"), NULL, SND_ASYNC | SND_FILENAME);
                    } else {
                        PlaySound(TEXT("sounds/victory.wav"), NULL, SND_ASYNC | SND_FILENAME);
                        isGameWon = true;
                    }

                }
            } else if (!godMode) {
                if (!isGameOver) {
                    PlaySound(TEXT("sounds/pacman_death.wav"), NULL, SND_ASYNC | SND_FILENAME);
                }
                isGameOver = true;
            }
        }
    }
}

/**
 * @brief Handles keyboard input for movement, reset, god mode, and crystals.
 * @param window Active GLFW window used for key state queries.
 */
void processInput(GLFWwindow* window) {
    static bool gKeyPressed = false;
    if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) {
        if (!gKeyPressed) {
            godMode = !godMode;
            gKeyPressed = true;
        }
    } else {
        gKeyPressed = false;
    }

    static bool rKeyPressed = false;
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        if (!rKeyPressed) {
            resetGame();
            rKeyPressed = true;
        }
    } else {
        rKeyPressed = false;
    }

    if (isGameOver || isGameWon) return;

    float tryX = pacman.x;
    float tryZ = pacman.z;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        tryX += sin(glm::radians(pacman.angle)) * pacman.speed * deltaTime;
        tryZ += cos(glm::radians(pacman.angle)) * pacman.speed * deltaTime;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        tryX -= sin(glm::radians(pacman.angle)) * pacman.speed * deltaTime;
        tryZ -= cos(glm::radians(pacman.angle)) * pacman.speed * deltaTime;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        tryX += cos(glm::radians(pacman.angle)) * pacman.speed * deltaTime;
        tryZ -= sin(glm::radians(pacman.angle)) * pacman.speed * deltaTime;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        tryX -= cos(glm::radians(pacman.angle)) * pacman.speed * deltaTime;
        tryZ += sin(glm::radians(pacman.angle)) * pacman.speed * deltaTime;
    }

    float margin = 0.15f;

    bool canMoveX = true;
    int minX = round(tryX - margin);
    int maxX = round(tryX + margin);
    int minZ = round(pacman.z - margin);
    int maxZ = round(pacman.z + margin);

    for (int z = minZ; z <= maxZ; z++) {
        for (int x = minX; x <= maxX; x++) {
            if (x < 0 || x >= MAP_WIDTH || z < 0 || z >= MAP_HEIGHT || levelMap[z][x] == FIELD_WALL) {
                canMoveX = false;
            }
        }
    }
    if (canMoveX) pacman.x = tryX;

    bool canMoveZ = true;
    minX = round(pacman.x - margin);
    maxX = round(pacman.x + margin);
    minZ = round(tryZ - margin);
    maxZ = round(tryZ + margin);

    for (int z = minZ; z <= maxZ; z++) {
        for (int x = minX; x <= maxX; x++) {
            if (x < 0 || x >= MAP_WIDTH || z < 0 || z >= MAP_HEIGHT || levelMap[z][x] == FIELD_WALL) {
                canMoveZ = false;
            }
        }
    }
    if (canMoveZ) pacman.z = tryZ;

    int gX = round(pacman.x);
    int gZ = round(pacman.z);
    if (gX >= 0 && gX < MAP_WIDTH && gZ >= 0 && gZ < MAP_HEIGHT) {
        if (levelMap[gZ][gX] == FIELD_CRYSTAL) {
            levelMap[gZ][gX] = FIELD_EMPTY;
            eatenCrystals++;

            PlaySound(TEXT("sounds/crystal_eaten.wav"), nullptr, SND_ASYNC | SND_FILENAME);

            if (eatenCrystals >= totalCrystals) {
                huntMode = true;
            }
        }
    }
}

/**
 * @brief Prints GLFW error descriptions to stderr.
 * @param error GLFW error code.
 * @param description Human-readable error description.
 */
void error_callback(int error, const char* description) {
    fputs(description, stderr);
}

/**
 * @brief Rotates the camera and Pac-Man based on mouse movement.
 * @param window Active GLFW window.
 * @param xpos Current cursor X position.
 * @param ypos Current cursor Y position.
 */
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    if (isGameOver || isGameWon) return;

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = ypos - lastY;

    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.15f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    pacman.angle -= xoffset;
    camPitch += yoffset;

    if (camPitch > 60.0f) camPitch = 60.0f;
    if (camPitch < -10.0f) camPitch = -10.0f;
}

/**
 * @brief Initializes gameplay state before the OpenGL resources are loaded.
 */
void initGame() {
    srand((unsigned int)time(NULL));
    resetGame();
}

/**
 * @brief Initializes shaders, HUD font, models, textures, and GL state.
 * @param window Active GLFW window.
 */
void initOpenGLProgram(GLFWwindow* window) {
    initShaders();
    initFont();
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    ghostModel = new Models::ObjModel("pacman_models/ghost.obj");
    pacmanOpenModel = new Models::ObjModel("pacman_models/pacman_open.obj");
    pacmanClosedModel = new Models::ObjModel("pacman_models/pacman_closed.obj");
    wallTexture = loadTexture("images/bricks_diffuse.png");
}

/**
 * @brief Releases shaders, models, textures, and shadow map resources.
 * @param window Active GLFW window.
 */
void freeOpenGLProgram(GLFWwindow* window) {
    freeShaders();
    if (ghostModel) { delete ghostModel; ghostModel = nullptr; }
    if (pacmanOpenModel) { delete pacmanOpenModel; pacmanOpenModel = nullptr; }
    if (pacmanClosedModel) { delete pacmanClosedModel; pacmanClosedModel = nullptr; }
    glDeleteTextures(1, &wallTexture);
    if (shadowMapTexture != 0) {
        glDeleteTextures(1, &shadowMapTexture);
        shadowMapTexture = 0;
    }
    if (shadowMapFBO != 0) {
        glDeleteFramebuffers(1, &shadowMapFBO);
        shadowMapFBO = 0;
    }
}

/**
 * @brief Runs one gameplay update step for the current frame.
 * @param window Active GLFW window.
 */
void updateGame(GLFWwindow* window) {
    glfwSetWindowTitle(window, "Pac-Man 3D");

    processInput(window);
    if (!isGameOver && !isGameWon) {
        updateGhosts();
    }
}

/**
 * @brief Draws soft floor blob shadows from every visible ceiling lamp.
 * @param position World-space object position.
 * @param radius Approximate object radius.
 * @param alpha Base shadow opacity.
 * @param sphereModel Sphere model flattened into a shadow blob.
 */
void drawBlobShadow(glm::vec3 position, float radius, float alpha, Models::Sphere& sphereModel) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glDepthMask(GL_FALSE);

    glUniform1i(spConstant->u("useTexture"), 0);
    setLightingEnabled(false, false);

    for (int i = 0; i < CEILING_LAMP_COUNT; ++i) {
        CeilingLamp lamp = ceilingLamps[i];
        if (!hasLampLineOfSight(position, lamp.position)) {
            continue;
        }

        float dx = position.x - lamp.position.x;
        float dz = position.z - lamp.position.z;
        float distanceXZ = sqrt(dx * dx + dz * dz);
        float distance3D = glm::length(position - lamp.position);
        float attenuation = 1.0f - distance3D / lamp.radius;
        if (attenuation <= 0.0f) {
            continue;
        }

        if (distanceXZ < 0.001f) {
            dx = 0.4f;
            dz = 0.25f;
            distanceXZ = sqrt(dx * dx + dz * dz);
        }

        dx /= distanceXZ;
        dz /= distanceXZ;

        float shadowLength = radius * (0.35f + distanceXZ * 0.06f);
        if (shadowLength > radius * 1.15f) {
            shadowLength = radius * 1.15f;
        }

        float shadowAlpha = alpha * 0.24f * attenuation * attenuation * lamp.intensity;
        if (shadowAlpha > alpha * 0.42f) {
            shadowAlpha = alpha * 0.42f;
        }

        glUniform4f(spConstant->u("color"), 0.0f, 0.0f, 0.0f, shadowAlpha);

        float shadowAngle = atan2(-dz, dx);
        glm::mat4 M = glm::mat4(1.0f);
        M = glm::translate(M, glm::vec3(position.x + dx * shadowLength, 0.03f, position.z + dz * shadowLength));
        M = glm::rotate(M, shadowAngle, glm::vec3(0.0f, 1.0f, 0.0f));
        M = glm::scale(M, glm::vec3(radius * 1.05f, 0.01f, radius * 0.82f));

        glUniformMatrix4fv(spConstant->u("M"), 1, false, glm::value_ptr(M));
        sphereModel.drawSolid();
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    setLightingEnabled(true, true);
}

/**
 * @brief Draws ceiling fixtures, yellow bulbs, and their translucent glow.
 * @param cubeModel Cube model used for lamp fixtures.
 * @param sphereModel Sphere model used for bulbs and glow volumes.
 */
void drawCeilingLamps(Models::Cube& cubeModel, Models::Sphere& sphereModel) {
    glUniform1i(spConstant->u("useTexture"), 0);
    setLightingEnabled(false, false);

    for (int i = 0; i < CEILING_LAMP_COUNT; ++i) {
        CeilingLamp lamp = ceilingLamps[i];

        glUniform4f(spConstant->u("color"), 0.04f, 0.04f, 0.05f, 1.0f);
        glm::mat4 fixture = glm::mat4(1.0f);
        fixture = glm::translate(fixture, glm::vec3(lamp.position.x, 2.32f, lamp.position.z));
        fixture = glm::scale(fixture, glm::vec3(0.26f, 0.045f, 0.26f));
        glUniformMatrix4fv(spConstant->u("M"), 1, false, glm::value_ptr(fixture));
        cubeModel.drawSolid();

        glUniform4f(spConstant->u("color"), lamp.color.r, lamp.color.g, lamp.color.b, 1.0f);
        glm::mat4 bulb = glm::mat4(1.0f);
        bulb = glm::translate(bulb, lamp.position);
        bulb = glm::scale(bulb, glm::vec3(0.16f));
        glUniformMatrix4fv(spConstant->u("M"), 1, false, glm::value_ptr(bulb));
        sphereModel.drawSolid(true);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        glUniform4f(spConstant->u("color"), lamp.color.r, lamp.color.g, lamp.color.b, 0.18f);
        glm::mat4 glow = glm::mat4(1.0f);
        glow = glm::translate(glow, lamp.position);
        glow = glm::scale(glow, glm::vec3(0.45f));
        glUniformMatrix4fv(spConstant->u("M"), 1, false, glm::value_ptr(glow));
        sphereModel.drawSolid(true);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }

    setLightingEnabled(true, true);
}

/**
 * @brief Draws a model into the depth shader using the supplied transform.
 * @param M Model matrix.
 * @param model Model to render.
 * @param smooth Whether to use smooth drawing for supported models.
 */
void drawDepthModel(const glm::mat4& M, Models::Model& model, bool smooth = false) {
    glUniformMatrix4fv(spDepth->u("M"), 1, GL_FALSE, glm::value_ptr(M));
    model.drawSolid(smooth);
}

/**
 * @brief Renders all objects that can cast into the depth shadow map.
 * @param cubeModel Cube model used for walls and fallbacks.
 * @param sphereModel Sphere model used for crystals and fallbacks.
 * @param pX Pac-Man world-space X position.
 * @param pY Pac-Man world-space Y position.
 * @param pZ Pac-Man world-space Z position.
 * @param pacmanAlpha Current Pac-Man visibility alpha.
 */
void renderShadowCasters(Models::Cube& cubeModel, Models::Sphere& sphereModel, float pX, float pY, float pZ, float pacmanAlpha) {
    for (int z = 0; z < MAP_HEIGHT; z++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            if (levelMap[z][x] == FIELD_WALL) {
                glm::mat4 M = glm::mat4(1.0f);
                M = glm::translate(M, glm::vec3(x * 2.0f, 1.0f, z * 2.0f));
                M = glm::scale(M, glm::vec3(1.0f));
                drawDepthModel(M, cubeModel);
            } else if (levelMap[z][x] == FIELD_CRYSTAL) {
                glm::mat4 M = glm::mat4(1.0f);
                M = glm::translate(M, glm::vec3(x * 2.0f, 0.35f, z * 2.0f));
                M = glm::scale(M, glm::vec3(0.15f));
                drawDepthModel(M, sphereModel, true);
            }
        }
    }

    for (size_t i = 0; i < ghosts.size(); i++) {
        if (ghostEaten[i]) continue;

        glm::mat4 M_ghost = glm::mat4(1.0f);
        M_ghost = glm::translate(M_ghost, glm::vec3(ghosts[i].x * 2.0f, 0.7f, ghosts[i].z * 2.0f));
        M_ghost = glm::rotate(M_ghost, glm::radians(ghosts[i].angle), glm::vec3(0.0f, 1.0f, 0.0f));
        M_ghost = glm::scale(M_ghost, glm::vec3(0.4f));

        if (ghostModel && ghostModel->vertexCount > 0) {
            drawDepthModel(M_ghost, *ghostModel);
        } else {
            drawDepthModel(M_ghost, cubeModel);
        }
    }

    if (pacmanAlpha > 0.01f) {
        glm::mat4 M_pac = glm::mat4(1.0f);
        M_pac = glm::translate(M_pac, glm::vec3(pX, pY, pZ));
        M_pac = glm::rotate(M_pac, glm::radians(pacman.angle), glm::vec3(0.0f, 1.0f, 0.0f));
        M_pac = glm::scale(M_pac, glm::vec3(0.4f));

        bool isMouthOpen = (int(glfwGetTime() * 8.0) % 2 == 0);

        if (isMouthOpen && pacmanOpenModel && pacmanOpenModel->vertexCount > 0) {
            drawDepthModel(M_pac, *pacmanOpenModel);
        } else if (!isMouthOpen && pacmanClosedModel && pacmanClosedModel->vertexCount > 0) {
            drawDepthModel(M_pac, *pacmanClosedModel);
        } else {
            drawDepthModel(M_pac, sphereModel, true);
        }
    }
}

/**
 * @brief Renders the scene into the shadow map depth texture.
 * @param cubeModel Cube model used for walls and fallbacks.
 * @param sphereModel Sphere model used for crystals and fallbacks.
 * @param pX Pac-Man world-space X position.
 * @param pY Pac-Man world-space Y position.
 * @param pZ Pac-Man world-space Z position.
 * @param pacmanAlpha Current Pac-Man visibility alpha.
 */
void renderShadowMap(Models::Cube& cubeModel, Models::Sphere& sphereModel, float pX, float pY, float pZ, float pacmanAlpha) {
    if (shadowMapFBO == 0 || shadowMapTexture == 0) return;

    lightSpaceMatrix = calculateLightSpaceMatrix();

    glViewport(0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);

    spDepth->use();
    glUniformMatrix4fv(spDepth->u("lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(2.0f, 4.0f);

    renderShadowCasters(cubeModel, sphereModel, pX, pY, pZ, pacmanAlpha);

    glDisable(GL_POLYGON_OFFSET_FILL);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

/**
 * @brief Renders the full 3D scene and 2D HUD for the current frame.
 * @param window Active GLFW window.
 */
void drawScene(GLFWwindow* window) {
    int windowWidth, windowHeight;
    glfwGetFramebufferSize(window, &windowWidth, &windowHeight);
    if (windowHeight == 0) windowHeight = 1;
    float aspectRatio = (float)windowWidth / (float)windowHeight;

    Models::Cube cubeModel;
    Models::Sphere sphereModel;
    updateCeilingLampPositions();

    glm::mat4 P = glm::perspective(glm::radians(80.0f), aspectRatio, 0.01f, 100.0f);

    float pX = pacman.x * 2.0f;
    float pY = 0.4f;
    float pZ = pacman.z * 2.0f;

    float targetCamX = pX - sin(glm::radians(pacman.angle)) * cos(glm::radians(camPitch)) * 2.5f;
    float targetCamZ = pZ - cos(glm::radians(pacman.angle)) * cos(glm::radians(camPitch)) * 2.5f;
    float targetCamY = pY + sin(glm::radians(camPitch)) * 2.5f;

    float dirX = targetCamX - pX;
    float dirY = targetCamY - pY;
    float dirZ = targetCamZ - pZ;

    float maxDist = sqrt(dirX * dirX + dirY * dirY + dirZ * dirZ);
    if (maxDist > 0.001f) {
        dirX /= maxDist;
        dirY /= maxDist;
        dirZ /= maxDist;
    }

    float hitDist = maxDist;
    float step = 0.02f;
    for (float d = 0.0f; d <= maxDist; d += step) {
        float testX = pX + dirX * d;
        float testY = pY + dirY * d;
        float testZ = pZ + dirZ * d;

        int gX = round(testX / 2.0f);
        int gZ = round(testZ / 2.0f);

        if (gX >= 0 && gX < MAP_WIDTH && gZ >= 0 && gZ < MAP_HEIGHT) {
            if (levelMap[gZ][gX] == FIELD_WALL || testY < 0.05f || testY > 1.8f) {
                hitDist = d;
                break;
            }
        } else {
            hitDist = d;
            break;
        }
    }

    float pushback = 0.35f;
    float finalDist = hitDist - pushback;
    if (finalDist < 0.0f) finalDist = 0.0f;

    float camX = pX + dirX * finalDist;
    float camY = pY + dirY * finalDist;
    float camZ = pZ + dirZ * finalDist;

    glm::vec3 camPos(camX, camY, camZ);
    glm::vec3 lookTarget = camPos - glm::vec3(dirX, dirY, dirZ);

    glm::mat4 V = glm::lookAt(camPos, lookTarget, glm::vec3(0.0f, 1.0f, 0.0f));

    float baseAlpha = 0.65f;
    float pacmanAlpha = (finalDist - 0.4f) / 1.6f;

    if (pacmanAlpha > baseAlpha) pacmanAlpha = baseAlpha;
    if (pacmanAlpha < 0.0f) pacmanAlpha = 0.0f;

    glViewport(0, 0, windowWidth, windowHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    spConstant->use();
    glUniformMatrix4fv(spConstant->u("P"), 1, false, glm::value_ptr(P));
    glUniformMatrix4fv(spConstant->u("V"), 1, false, glm::value_ptr(V));
    setLightingUniforms();

    for (int z = 0; z < MAP_HEIGHT; z++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            glm::vec3 pos = glm::vec3(x * 2.0f, 0.0f, z * 2.0f);

            glUniform1i(spConstant->u("useTexture"), 0);
            glUniform4f(spConstant->u("color"), 0.15f, 0.15f, 0.15f, 1.0f);
            glm::mat4 Mf = glm::mat4(1.0f);
            Mf = glm::translate(Mf, glm::vec3(x * 2.0f, -0.5f, z * 2.0f));
            Mf = glm::scale(Mf, glm::vec3(1.0f, 0.5f, 1.0f));
            glUniformMatrix4fv(spConstant->u("M"), 1, false, glm::value_ptr(Mf));
            cubeModel.drawSolid();

            glUniform4f(spConstant->u("color"), 0.2f, 0.2f, 0.2f, 1.0f);
            glm::mat4 Mc = glm::mat4(1.0f);
            Mc = glm::translate(Mc, glm::vec3(x * 2.0f, 2.5f, z * 2.0f));
            Mc = glm::scale(Mc, glm::vec3(1.0f, 0.5f, 1.0f));
            glUniformMatrix4fv(spConstant->u("M"), 1, false, glm::value_ptr(Mc));
            cubeModel.drawSolid();

            if (levelMap[z][x] == FIELD_WALL) {
                drawWall(
                    cubeModel,
                    wallTexture,
                    glm::vec3(x * 2.0f, 1.0f, z * 2.0f),
                    glm::vec3(1.0f)
                );
            }
            else if (levelMap[z][x] == FIELD_CRYSTAL) {
                drawBlobShadow(glm::vec3(x * 2.0f, 0.0f, z * 2.0f), 0.12f, 0.4f, sphereModel);

                glUniform1i(spConstant->u("useTexture"), 0);
                glUniform4f(spConstant->u("color"), 1.0f, 1.0f, 0.0f, 1.0f);
                glm::mat4 M = glm::mat4(1.0f);
                M = glm::translate(M, glm::vec3(x * 2.0f, 0.35f, z * 2.0f));
                M = glm::scale(M, glm::vec3(0.15f));
                glUniformMatrix4fv(spConstant->u("M"), 1, false, glm::value_ptr(M));
                sphereModel.drawSolid();
            }
        }
    }

    drawCeilingLamps(cubeModel, sphereModel);

    glUniform1i(spConstant->u("useTexture"), 0);

    for (size_t i = 0; i < ghosts.size(); i++) {
        if (ghostEaten[i]) continue;

        drawBlobShadow(glm::vec3(ghosts[i].x * 2.0f, 0.0f, ghosts[i].z * 2.0f), 0.35f, 0.6f, sphereModel);

        glm::vec4 gColor;
        if (huntMode) {
            gColor = glm::vec4(0.2f, 0.4f, 1.0f, 1.0f);
        } else {
            gColor = ghostColors[i % 4];
        }

        glUniform4f(spConstant->u("color"), gColor.r, gColor.g, gColor.b, gColor.a);

        glm::mat4 M_ghost = glm::mat4(1.0f);
        M_ghost = glm::translate(M_ghost, glm::vec3(ghosts[i].x * 2.0f, 0.7f, ghosts[i].z * 2.0f));
        M_ghost = glm::rotate(M_ghost, glm::radians(ghosts[i].angle), glm::vec3(0.0f, 1.0f, 0.0f));
        M_ghost = glm::scale(M_ghost, glm::vec3(0.4f));
        glUniformMatrix4fv(spConstant->u("M"), 1, false, glm::value_ptr(M_ghost));

        if (ghostModel && ghostModel->vertexCount > 0) {
            ghostModel->drawSolid();
        } else {
            cubeModel.drawSolid();
        }
    }

    if (pacmanAlpha > 0.01f) {
        drawBlobShadow(glm::vec3(pX, 0.0f, pZ), 0.4f, 0.6f * pacmanAlpha, sphereModel);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        glUniform4f(spConstant->u("color"), 1.0f, 1.0f, 0.0f, pacmanAlpha);
        glm::mat4 M_pac = glm::mat4(1.0f);
        M_pac = glm::translate(M_pac, glm::vec3(pX, pY, pZ));
        M_pac = glm::rotate(M_pac, glm::radians(pacman.angle), glm::vec3(0.0f, 1.0f, 0.0f));
        M_pac = glm::scale(M_pac, glm::vec3(0.4f));

        bool isMouthOpen = (int(glfwGetTime() * 8.0) % 2 == 0);

        if (isMouthOpen && pacmanOpenModel && pacmanOpenModel->vertexCount > 0) {
            glUniformMatrix4fv(spConstant->u("M"), 1, false, glm::value_ptr(M_pac));
            pacmanOpenModel->drawSolid();
        } else if (!isMouthOpen && pacmanClosedModel && pacmanClosedModel->vertexCount > 0) {
            glUniformMatrix4fv(spConstant->u("M"), 1, false, glm::value_ptr(M_pac));
            pacmanClosedModel->drawSolid();
        } else {
            glUniformMatrix4fv(spConstant->u("M"), 1, false, glm::value_ptr(M_pac));
            sphereModel.drawSolid();
        }

        glDisable(GL_CULL_FACE);
        glDisable(GL_BLEND);
    }

    glDisable(GL_DEPTH_TEST);
    setLightingEnabled(false, false);

    glm::mat4 orthoP = glm::ortho(0.0f, (float)windowWidth, 0.0f, (float)windowHeight, -1.0f, 1.0f);
    glm::mat4 orthoV = glm::mat4(1.0f);
    glUniformMatrix4fv(spConstant->u("P"), 1, false, glm::value_ptr(orthoP));
    glUniformMatrix4fv(spConstant->u("V"), 1, false, glm::value_ptr(orthoV));

    if (isGameWon) {
        std::string winText = "WYGRANA!";
        float scale = 12.0f;
        float widthText = winText.length() * (4.0f * scale);
        drawText(winText, (windowWidth / 2.0f) - (widthText / 2.0f), (windowHeight / 2.0f) + 50.0f, scale, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), cubeModel);
    } else if (isGameOver) {
        std::string overText = "KONIEC GRY";
        float scale = 10.0f;
        float widthText = overText.length() * (4.0f * scale);
        drawText(overText, (windowWidth / 2.0f) - (widthText / 2.0f), (windowHeight / 2.0f) + 50.0f, scale, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f), cubeModel);
    }

    if (!isGameOver && !isGameWon) {
        std::string scoreText = std::to_string(eatenCrystals) + "/" + std::to_string(totalCrystals);
        float textScale = 5.0f;
        float textWidth = scoreText.length() * (4.0f * textScale);

        int totalGhosts = ghosts.size();
        int eatenGhostsCount = 0;
        for (bool eaten : ghostEaten) {
            if (eaten) eatenGhostsCount++;
        }

        std::string ghostScoreText = std::to_string(totalGhosts - eatenGhostsCount) + "/" + std::to_string(totalGhosts);
        float ghostTextWidth = ghostScoreText.length() * (4.0f * textScale);

        if (huntMode) {
            std::string huntText = "ZJEDZ DUCHY";
            float flash = (sin(glfwGetTime() * 10.0f) + 1.0f) / 2.0f;
            float huntWidth = huntText.length() * (4.0f * textScale);
            drawText(huntText, windowWidth - huntWidth - 20.0f, windowHeight - 30.0f, textScale, glm::vec4(0.0f, flash, 1.0f, 1.0f), cubeModel);
        } else {
            drawText(scoreText, windowWidth - textWidth - 20.0f, windowHeight - 30.0f, textScale, glm::vec4(1.0f, 1.0f, 0.0f, 1.0f), cubeModel);
        }

        drawText(ghostScoreText, windowWidth - ghostTextWidth - 20.0f, windowHeight - 65.0f, textScale, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f), cubeModel);

        if (godMode) {
            std::string godText = "GOD MODE";
            float godScale = 4.0f;
            float godWidth = godText.length() * (4.0f * godScale);
            drawText(godText, windowWidth - godWidth - 20.0f, windowHeight - 100.0f, godScale, glm::vec4(1.0f, 0.5f, 0.0f, 1.0f), cubeModel);
        }
    }

    glEnable(GL_DEPTH_TEST);
}
