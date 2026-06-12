#ifndef PACMAN_3D_OBJMODEL_H
#define PACMAN_3D_OBJMODEL_H

#include "model.h"
#include <string>
#include <vector>
#include <glm/glm.hpp>

namespace Models {
    struct SubMesh {
        std::string materialName;
        std::vector<float> vertices;
        std::vector<float> normals;
        std::vector<float> texCoords;
        std::vector<float> colors;
        unsigned int vertexCount = 0;
    };

    class ObjModel : public Model {
    private:
        std::vector<SubMesh> subMeshes;
        void parseVertex(const std::string& token, int& v, int& t, int& n);

    public:
        ObjModel(const std::string& filepath);
        virtual ~ObjModel();
        virtual void drawSolid(bool smooth = false) override;
    };
}

#endif //PACMAN_3D_OBJMODEL_H