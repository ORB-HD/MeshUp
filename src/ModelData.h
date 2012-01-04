#ifndef _MODELDATA_H
#define _MODELDATA_H

#include <vector>

#include "SimpleMath.h"

struct Material {
	Vector4d ambient;
	Vector4d diffuse;
	Vector4d specular;
	double shininess;
};

struct MeshData {
	MeshData() :
		parented_segment(""),
		vbo_id(0),
		started(false),
		parent_translation (0., 0., 0.),
		parent_scale (0., 0., 0.),
		dimensions (0., 0., 0.) 
	{}

	void begin();
	void end();

	void addVertice (float x, float y, float z);
	void addVerticefv (float vert[3]);
	void addNormal (float x, float y, float z);
	void addNormalfv (float norm[3]);

	unsigned int generate_vbo();
	void delete_vbo();

	std::string parented_segment;

	unsigned int vbo_id;
	bool started;

	Vector3d parent_translation;
	Vector3d parent_scale;
	Vector3d dimensions;

	std::vector<Vector3f> vertices;
	std::vector<Vector3f> normals;
	/// \note always 3 succeeding calls of addVertice are assumed to be a
	// triangle!
	std::vector<unsigned int> triangle_indices;
};

struct Segment {
	std::string name;
	std::string parent_segment;

	Vector3d parent_translation;
	Vector3d parent_rotation;
	Vector3d parent_scale;

	double mass;
	Vector3d com;
	Matrix33d inertia;

	std::vector<MeshData> meshes;
};

struct ModelData {
	std::string name;
	std::string author;

	std::vector<Segment> segments;
	std::vector<MeshData> meshes;
};

#endif
