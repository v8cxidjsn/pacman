#ifndef TEAPOT_H
#define TEAPOT_H

//Utah teapot model made out of triangles
//Contains arrays
//vertices - vertex positions in homogenous coordinates
//normals -vertex normals in homogenous coordinates (flat shading)
//vertexNormals - vertex normals in homogenous coordinates (smooth shading)
//texCoords -  texturing coordinates
//colors - vertex colors (rgba)
//TBN friendly
//Culling GL_CW

#include "model.h"

namespace Models {


	namespace TeapotInternal {
		extern float vertices[];
		extern float normals[];
		extern float vertexNormals[];
		extern float texCoords[];
		extern float colors[];
		extern unsigned int vertexCount;
	}

	class Teapot: public Model {
		public:
			Teapot();
			virtual ~Teapot();
			virtual void drawSolid(bool smooth=true);
	};

	extern Teapot teapot;

}
#endif
