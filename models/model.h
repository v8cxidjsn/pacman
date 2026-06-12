#ifndef MODEL_H
#define MODEL_H


#include <GL/glew.h>
#include <vector>
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>
#include "../utils/constants.h"

namespace Models {

	class Model {
		public:
			int vertexCount;
			float *vertices;
			float *normals;
			float *vertexNormals;
			float *texCoords;
			float *colors;

			virtual void drawSolid(bool smooth)=0;
			virtual void drawWire(bool smooth=false);
	};
}

#endif
