#ifndef _OBJMESH_H
#define _OBJMESH_H

#include <vector>
#include <iostream>
#include <boost/shared_ptr.hpp>
#include <limits>

#include "SimpleMath/SimpleMath.h"
#include "SimpleMath/SimpleMathGL.h"

/** \brief Loads Wavefront OBJ files and prepares them for use in
 * OpenGL.
 */
struct OBJMesh {
	OBJMesh() :
		vbo_id(0),
		started(false),
		smooth_shading(true),
		bbox_min (std::numeric_limits<float>::max(),
				std::numeric_limits<float>::max(),
				std::numeric_limits<float>::max()),
		bbox_max (-std::numeric_limits<float>::max(),
				-std::numeric_limits<float>::max(),
				-std::numeric_limits<float>::max())
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

	bool loadOBJ (const char *filename, bool strict = true);

	// only loads the object in the OBJ file of the given obj_name
	bool loadOBJ (const char *filename, const char *sub_object_name, bool strict = true);

	unsigned int vbo_id;
	bool started;
	bool smooth_shading;

	Vector3f bbox_min;
	Vector3f bbox_max;

	std::vector<Vector3f> vertices;
	std::vector<Vector3f> normals;

	/// \note always 3 succeeding calls of addVertice are assumed to be a
	// triangle!
	std::vector<unsigned int> triangle_indices;
};

#endif
