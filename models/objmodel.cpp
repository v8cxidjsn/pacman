#include <GL/glew.h>
#include "models/objmodel.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <glm/glm.hpp>

namespace Models {
    ObjModel::ObjModel(const std::string& filepath) {
        vertexCount = 0;
        vertices = nullptr;
        normals = nullptr;
        vertexNormals = nullptr;
        texCoords = nullptr;
        colors = nullptr;

        std::vector<glm::vec4> temp_vertices;
        std::vector<glm::vec4> temp_normals;
        std::vector<glm::vec2> temp_texCoords;

        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "[BLAD] Nie mozna otworzyc pliku OBJ: " << filepath << std::endl;
            return;
        }

        std::string line;
        SubMesh currentSubMesh;

        std::string activeObjectName = "default_object";
        std::string activeGroupName = "default_group";
        std::string activeMaterialName = "default_material";

        auto makeCombinedName = [&]() {
            return activeObjectName + "|" + activeGroupName + "|" + activeMaterialName;
        };

        currentSubMesh.materialName = makeCombinedName();

        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;

            std::stringstream ss(line);
            std::string prefix;
            ss >> prefix;

            if (prefix == "v") {
                float x, y, z;
                ss >> x >> y >> z;
                temp_vertices.push_back(glm::vec4(x, y, z, 1.0f));
            } else if (prefix == "vt") {
                float u, v;
                ss >> u >> v;
                temp_texCoords.push_back(glm::vec2(u, v));
            } else if (prefix == "vn") {
                float x, y, z;
                ss >> x >> y >> z;
                temp_normals.push_back(glm::vec4(x, y, z, 0.0f));
            } else if (prefix == "o" || prefix == "g" || prefix == "usemtl") {
                std::string name;
                ss >> name;

                if (!currentSubMesh.vertices.empty()) {
                    currentSubMesh.vertexCount = currentSubMesh.vertices.size() / 4;
                    subMeshes.push_back(currentSubMesh);
                    currentSubMesh = SubMesh();
                }

                if (prefix == "o") {
                    activeObjectName = name;
                } else if (prefix == "g") {
                    activeGroupName = name;
                } else if (prefix == "usemtl") {
                    activeMaterialName = name;
                }

                currentSubMesh.materialName = makeCombinedName();
            } else if (prefix == "f") {
                std::vector<std::string> tokens;
                std::string token;
                while (ss >> token) {
                    tokens.push_back(token);
                }

                for (size_t i = 1; i < tokens.size() - 1; ++i) {
                    std::string face_tokens[3] = { tokens[0], tokens[i], tokens[i + 1] };
                    for (int j = 0; j < 3; ++j) {
                        int vIdx, tIdx, nIdx;
                        parseVertex(face_tokens[j], vIdx, tIdx, nIdx);

                        if (vIdx >= 0 && vIdx < (int)temp_vertices.size()) {
                            glm::vec4 vert = temp_vertices[vIdx];
                            currentSubMesh.vertices.push_back(vert.x);
                            currentSubMesh.vertices.push_back(vert.y);
                            currentSubMesh.vertices.push_back(vert.z);
                            currentSubMesh.vertices.push_back(vert.w);
                        } else {
                            currentSubMesh.vertices.push_back(0.0f);
                            currentSubMesh.vertices.push_back(0.0f);
                            currentSubMesh.vertices.push_back(0.0f);
                            currentSubMesh.vertices.push_back(1.0f);
                        }

                        if (tIdx >= 0 && tIdx < (int)temp_texCoords.size()) {
                            glm::vec2 tex = temp_texCoords[tIdx];
                            currentSubMesh.texCoords.push_back(tex.x);
                            currentSubMesh.texCoords.push_back(tex.y);
                        } else {
                            currentSubMesh.texCoords.push_back(0.0f);
                            currentSubMesh.texCoords.push_back(0.0f);
                        }

                        if (nIdx >= 0 && nIdx < (int)temp_normals.size()) {
                            glm::vec4 norm = temp_normals[nIdx];
                            currentSubMesh.normals.push_back(norm.x);
                            currentSubMesh.normals.push_back(norm.y);
                            currentSubMesh.normals.push_back(norm.z);
                            currentSubMesh.normals.push_back(norm.w);
                        } else {
                            currentSubMesh.normals.push_back(0.0f);
                            currentSubMesh.normals.push_back(1.0f);
                            currentSubMesh.normals.push_back(0.0f);
                            currentSubMesh.normals.push_back(0.0f);
                        }

                        currentSubMesh.colors.push_back(1.0f);
                        currentSubMesh.colors.push_back(1.0f);
                        currentSubMesh.colors.push_back(1.0f);
                        currentSubMesh.colors.push_back(1.0f);
                    }
                }
            }
        }

        if (!currentSubMesh.vertices.empty()) {
            currentSubMesh.vertexCount = currentSubMesh.vertices.size() / 4;
            subMeshes.push_back(currentSubMesh);
        }

        for (const auto& sm : subMeshes) {
            vertexCount += sm.vertexCount;
        }

        if (vertexCount > 0) {
            std::cout << "[SUKCES] Wczytano model z: " << filepath << " z podzialem na " << subMeshes.size() << " submeshe:" << std::endl;
            for (size_t i = 0; i < subMeshes.size(); ++i) {
                std::cout << "  - Submesh " << i << ": Nazwa=\"" << subMeshes[i].materialName << "\", Wierzcholki=" << subMeshes[i].vertexCount << std::endl;
            }
        }
    }

    void ObjModel::parseVertex(const std::string& token, int& v, int& t, int& n) {
        v = t = n = -1;
        size_t firstSlash = token.find('/');
        if (firstSlash == std::string::npos) {
            v = std::stoi(token) - 1;
            return;
        }
        v = std::stoi(token.substr(0, firstSlash)) - 1;
        size_t secondSlash = token.find('/', firstSlash + 1);
        if (secondSlash == std::string::npos) {
            std::string tStr = token.substr(firstSlash + 1);
            if (!tStr.empty()) t = std::stoi(tStr) - 1;
            return;
        }
        std::string tStr = token.substr(firstSlash + 1, secondSlash - firstSlash - 1);
        if (!tStr.empty()) t = std::stoi(tStr) - 1;
        std::string nStr = token.substr(secondSlash + 1);
        if (!nStr.empty()) n = std::stoi(nStr) - 1;
    }

    ObjModel::~ObjModel() {}

    void ObjModel::drawSolid(bool smooth) {
        if (subMeshes.empty()) return;

        GLint currentProgram;
        glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
        GLint colorLoc = glGetUniformLocation(currentProgram, "color");

        glm::vec4 bodyColor(1.0f, 0.0f, 0.0f, 1.0f);
        if (colorLoc != -1) {
            float currentVal[4];
            glGetUniformfv(currentProgram, colorLoc, currentVal);
            bodyColor = glm::vec4(currentVal[0], currentVal[1], currentVal[2], currentVal[3]);
        }

        bool hasKeywords = false;
        for (const auto& subMesh : subMeshes) {
            std::string mat = subMesh.materialName;
            std::transform(mat.begin(), mat.end(), mat.begin(), ::tolower);
            if (mat.find("eye") != std::string::npos ||
                mat.find("oko") != std::string::npos ||
                mat.find("oczy") != std::string::npos ||
                mat.find("white") != std::string::npos ||
                mat.find("pupil") != std::string::npos ||
                mat.find("zren") != std::string::npos ||
                mat.find("źren") != std::string::npos ||
                mat.find("black") != std::string::npos ||
                mat.find("blue") != std::string::npos ||
                mat.find("niebies") != std::string::npos) {
                hasKeywords = true;
                break;
            }
        }

        std::vector<std::string> roles(subMeshes.size(), "body");

        if (hasKeywords) {
            for (size_t i = 0; i < subMeshes.size(); ++i) {
                std::string mat = subMeshes[i].materialName;
                std::transform(mat.begin(), mat.end(), mat.begin(), ::tolower);

                if (mat.find("pupil") != std::string::npos ||
                    mat.find("zren") != std::string::npos ||
                    mat.find("źren") != std::string::npos ||
                    mat.find("black") != std::string::npos ||
                    mat.find("blue") != std::string::npos ||
                    mat.find("niebies") != std::string::npos) {
                    roles[i] = "pupils";
                } else if (mat.find("eye") != std::string::npos ||
                           mat.find("oko") != std::string::npos ||
                           mat.find("oczy") != std::string::npos ||
                           mat.find("white") != std::string::npos) {
                    roles[i] = "eyes";
                } else {
                    roles[i] = "body";
                }
            }
        } else {
            unsigned int maxVertices = 0;
            size_t bodyIndex = 0;
            for (size_t i = 0; i < subMeshes.size(); ++i) {
                if (subMeshes[i].vertexCount > maxVertices) {
                    maxVertices = subMeshes[i].vertexCount;
                    bodyIndex = i;
                }
            }

            roles[bodyIndex] = "body";

            std::vector<std::pair<unsigned int, size_t>> remaining;
            for (size_t i = 0; i < subMeshes.size(); ++i) {
                if (i != bodyIndex) {
                    remaining.push_back({subMeshes[i].vertexCount, i});
                }
            }

            std::sort(remaining.begin(), remaining.end(), [](const auto& a, const auto& b) {
                return a.first > b.first;
            });

            size_t half = remaining.size() / 2;
            for (size_t i = 0; i < remaining.size(); ++i) {
                if (i < half) {
                    roles[remaining[i].second] = "eyes";
                } else {
                    roles[remaining[i].second] = "pupils";
                }
            }
        }

        for (size_t i = 0; i < subMeshes.size(); ++i) {
            const auto& subMesh = subMeshes[i];
            std::string role = roles[i];

            if (role == "eyes") {
                glUniform4f(colorLoc, 1.0f, 1.0f, 1.0f, 1.0f);
            } else if (role == "pupils") {
                glUniform4f(colorLoc, 0.0f, 0.0f, 0.0f, 1.0f);
            } else {
                glUniform4f(colorLoc, bodyColor.r, bodyColor.g, bodyColor.b, bodyColor.a);
            }

            glEnableVertexAttribArray(0);
            glEnableVertexAttribArray(1);
            glEnableVertexAttribArray(2);
            glEnableVertexAttribArray(3);

            glVertexAttribPointer(0, 4, GL_FLOAT, false, 0, subMesh.vertices.data());
            glVertexAttribPointer(1, 4, GL_FLOAT, false, 0, subMesh.normals.data());
            glVertexAttribPointer(2, 2, GL_FLOAT, false, 0, subMesh.texCoords.data());
            glVertexAttribPointer(3, 4, GL_FLOAT, false, 0, subMesh.colors.data());

            glDrawArrays(GL_TRIANGLES, 0, subMesh.vertexCount);

            glDisableVertexAttribArray(0);
            glDisableVertexAttribArray(1);
            glDisableVertexAttribArray(2);
            glDisableVertexAttribArray(3);
        }

        if (colorLoc != -1) {
            glUniform4f(colorLoc, bodyColor.r, bodyColor.g, bodyColor.b, bodyColor.a);
        }
    }
}