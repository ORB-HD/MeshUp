/*
 * QtGLAppBase - Simple Qt Application to get started with OpenGL stuff.
 *
 * Copyright (c) 2011-2018 Martin Felis <martin.felis@iwr.uni-heidelberg.de>
 *
 * Licensed under the MIT license. See LICENSE for more details.
 */

#ifndef _MESHVBO_H
#define _MESHVBO_H

#include <vector>
#include <iostream>
#include <cstddef>
#include <limits>

#include "Math.h"

typedef ptrdiff_t GLsizeiptr;

struct MaterialLib {
};

struct Material {
	std::string name;
	Vector3f color_ambient;
	Vector3f color_diffuse;
	Vector3f color_specular;
	float transparency;
	int illum;

	std::string map_ambient;
	std::string map_diffuse;
	std::string map_specular;
	std::string map_bump;

	unsigned int texture_ambient;
	unsigned int texture_diffuse;
	unsigned int texture_specular;
	unsigned int texture_bump;
};

/** \brief Loads Wavefront VBO files and prepares them for use in
 * OpenGL.
 */
struct MeshVBO {
	MeshVBO() :
		vbo_id(0),
		started(false),
		smooth_shading(true),
		buffer_size (0),
		normal_offset (0),
		color_offset (0),
		bbox_min (std::numeric_limits<float>::max(),
				std::numeric_limits<float>::max(),
				std::numeric_limits<float>::max()),
		bbox_max (-std::numeric_limits<float>::max(),
				-std::numeric_limits<float>::max(),
				-std::numeric_limits<float>::max())
	{}
	MeshVBO (const MeshVBO& mesh);
	MeshVBO& operator= (const MeshVBO& mesh);
	~MeshVBO() {
		if (vbo_id != 0) {
			delete_vbo();
		}
	}

	void begin();
	void end();

	void addVertex4f (float x, float y, float z, float w);
	void addVertex4fv (const float vert[4]);
	void addVertex3f (float x, float y, float z);
	void addVertex3fv (const float vert[3]);
	void addNormal (float x, float y, float z);
	void addNormalfv (const float norm[3]);
	void addColor4f (float x, float y, float z, float a);
	void addColor4fv (const float color[4]);
	void addColor3f (float x, float y, float z);
	void addColor3fv (const float color[3]);

	unsigned int generate_vbo();
	void delete_vbo();
	void debug_vbo();

	void draw(unsigned int mode);

	unsigned int vbo_id;
	bool started;
	bool smooth_shading;

	GLsizeiptr buffer_size;
	GLsizeiptr normal_offset;
	GLsizeiptr color_offset;
	
	Vector3f bbox_min;
	Vector3f bbox_max;

	std::vector<Vector4f> vertices;
	std::vector<Vector3f> normals;
	std::vector<Vector4f> colors;

	void join (const Matrix44f &transformation, const MeshVBO &other);
	void transform(const Matrix44f &transformation);
	void setColor(const Vector4f &color);
	void center ();
	bool loadOBJ (const char* filename, const char* object_name = NULL, bool strict = false);
};

MeshVBO CreateUVSphere (unsigned int rows, unsigned int segments);

MeshVBO CreateCuboid (float width, float height, float depth);

MeshVBO CreateGrid (unsigned int cells_u, unsigned int cells_v, const Vector3f &normal, Vector3f color1, Vector3f color2);

inline MeshVBO CreateCube () {
	return CreateCuboid (1.f, 1.f, 1.f);
}

MeshVBO CreateCylinder (unsigned int segments);
MeshVBO CreateCylinder (unsigned int segments, float length, float scaleRadius, Vector4f color);

MeshVBO CreateCapsule (unsigned int rows, unsigned int segments, float length_z, float radius);

MeshVBO CreateUnit3DArrow(Vector4f color=Vector4f(1.f, 1.f, 1.f, 0.f));
MeshVBO CreateUnit3DCircleArrow(Vector4f color=Vector4f(1.f, 1.f, 1.f, 0.f));

MeshVBO CreateCone(int segments, float height, float radius, Vector4f color);

MeshVBO CreateOpenTorus(int segments, float width, float radius, Vector4f color, bool showmiddle=false);

#endif
