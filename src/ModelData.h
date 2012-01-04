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
		parent_scale (0., 0., 0.)
	{}

	void begin();
	void end();

	void addVertice (float x, float y, float z);
	void addVerticefv (float vert[3]);
	void addNormal (float x, float y, float z);
	void addNormalfv (float norm[3]);

	unsigned int generate_vbo();
	void delete_vbo();

	void draw();

	std::string parented_segment;

	unsigned int vbo_id;
	bool started;

	Vector3f parent_translation;
	Vector3f parent_scale;

	std::vector<Vector3f> vertices;
	std::vector<Vector3f> normals;
	/// \note always 3 succeeding calls of addVertice are assumed to be a
	// triangle!
	std::vector<unsigned int> triangle_indices;
};

struct Segment {
	Segment () :
		name ("unnamed"),
		parent_translation (0.f, 0.f, 0.f),
		parent_rotation (0.f, 0.f, 0.f),
		parent_scale (0.f, 0.f, 0.f),
		bbox_min (0.f, 0.f, 0.f),
		bbox_max (0.f, 0.f, 0.f)
	{}

	std::string name;

	Vector3f parent_translation;
	Vector3f parent_rotation;
	Vector3f parent_scale;

	Vector3f bbox_min;
	Vector3f bbox_max;

	/// \note actual storage of items is in the ModelData structure
	std::vector<MeshData* > meshes;
	std::vector<Segment* > children;

	void draw();
};

struct ModelData {
	ModelData() :
		base_segment(NULL)
	{
		// create the base segment and initialize index pointer
		Segment base_seg;
		base_seg.name = "BASE";
		segment_parents.push_back (0);
		segments.push_back (base_seg);

		base_segment = &segments[0];
	}

	Segment *base_segment;

	std::vector<unsigned int> segment_parents;
	std::vector<Segment> segments;
	std::vector<MeshData> meshes;

	unsigned int addSegment (
			unsigned int parent_segment_index,
			Vector3f parent_translation,
			Vector3f parent_rotation,
			Segment segment);
	unsigned int addSegmentMesh (
			unsigned int segment_index,
			Vector3f translation,
			Vector3f scale,
			MeshData mesh);

	void draw();
};

#endif
