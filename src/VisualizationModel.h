#ifndef _MODELDATA_H
#define _MODELDATA_H

#include "SimpleMath.h"

struct Material {
	Vector4d ambient;
	Vector4d diffuse;
	Vector4d specular;
	double shininess;
};

struct MeshData {
	std::vector<Vector3d> vertices;
	std::vector<Vector3d> normals;
	std::vector<Vector3i> triangle_indices;
};

struct Segment {
	std::string name;
	std::string parent_segment;

	Vector3d parent_translation;
	Vector3d parent_rotation;

	double mass;
	Vector3d com;
	Matrix33d inertia;

	std::vector<MeshData> meshes;
};

struct Model {
	std::string name;
	std::string author;

	std::vector<Segment> segments;
};

#endif
