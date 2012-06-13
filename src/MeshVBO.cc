#include "GL/glew.h"

#include "MeshVBO.h"

#include "SimpleMath/SimpleMathGL.h"
#include "string_utils.h"

#include <iomanip>
#include <fstream>
#include <limits>

using namespace std;

const bool use_vbo = true;

void MeshVBO::begin() {
	started = true;

	vertices.resize(0);
	normals.resize(0);
	triangle_indices.resize(0);
	colors.resize(0);
}

void MeshVBO::end() {
	if (normals.size()) {
		if (normals.size() != triangle_indices.size()) {
			std::cerr << "Error: number of normals must equal the number of vertices specified!" << endl;
			exit (1);
		}
	}

	started = false;
}

unsigned int MeshVBO::generate_vbo() {
	if (normals.size() == 0)
		have_normals = false;
	else
		have_normals = true;

	if (colors.size() == 0)
		have_colors = false;
	else
		have_colors = true;

	assert (vbo_id == 0);
	assert (started == false);
	assert (vertices.size() != 0);
	assert (!have_normals || (normals.size() == vertices.size()));
	assert (!have_colors || (colors.size() == vertices.size()));

//	cerr << __func__ << ": vert count = " << vertices.size() << endl;

	// create the buffer
	glGenBuffers (1, &vbo_id);

	// initialize the buffer object
	glBindBuffer (GL_ARRAY_BUFFER, vbo_id);

	buffer_size = sizeof(float) * 3 * vertices.size();
	normal_offset = 0;
	color_offset = 0;
	
	if (have_normals) {
		normal_offset = buffer_size;
		buffer_size += sizeof(float) * 3 * normals.size();
	}
	if (have_colors) {
		color_offset = buffer_size;
		buffer_size += sizeof(float) * 3 * colors.size();
	}

	glBufferData (GL_ARRAY_BUFFER, buffer_size, NULL, GL_STATIC_DRAW);

	// fill the data
	
	// multiple sub buffers
	glBufferSubData (GL_ARRAY_BUFFER, 0, sizeof (float) * 3 * vertices.size(), &vertices[0]);

	if (have_normals) {
		glBufferSubData (GL_ARRAY_BUFFER, normal_offset, sizeof (float) * 3 * normals.size(), &normals[0]);
	}

	if (have_colors)
		glBufferSubData (GL_ARRAY_BUFFER, color_offset, sizeof (float) * 3 * colors.size(), &colors[0]);

	glBindBuffer (GL_ARRAY_BUFFER, 0);

	return vbo_id;
}

void MeshVBO::delete_vbo() {
	assert (vbo_id != 0);
	glDeleteBuffers (1, &vbo_id);

	vbo_id = 0;
}

void MeshVBO::addVertice (float x, float y, float z) {
	Vector3f vertex;
	vertex[0] = x;
	vertex[1] = y;
	vertex[2] = z;
	vertices.push_back(vertex);

	bbox_max[0] = max (vertex[0], bbox_max[0]);
	bbox_max[1] = max (vertex[1], bbox_max[1]);
	bbox_max[2] = max (vertex[2], bbox_max[2]);

	bbox_min[0] = min (vertex[0], bbox_min[0]);
	bbox_min[1] = min (vertex[1], bbox_min[1]);
	bbox_min[2] = min (vertex[2], bbox_min[2]);

	triangle_indices.push_back (vertices.size() - 1);
}

void MeshVBO::addVerticefv (float vert[3]) {
	addVertice (vert[0], vert[1], vert[2]);
}

void MeshVBO::addNormal (float x, float y, float z) {
	Vector3f normal;
	normal[0] = x;
	normal[1] = y;
	normal[2] = z;
	normals.push_back (normal);
}

void MeshVBO::addNormalfv (float normal[3]) {
	addNormal (normal[0], normal[1], normal[2]);
}

void MeshVBO::addColor (float r, float g, float b) {
	Vector3f color;
	color[0] = r;
	color[1] = g;
	color[2] = b;
	colors.push_back (color);
}

void MeshVBO::addColorfv (float color[3]) {
	addColor (color[0], color[1], color[2]);
}

void MeshVBO::draw(unsigned int mode) {
	if (smooth_shading)
		glShadeModel(GL_SMOOTH);
	else
		glShadeModel(GL_FLAT);

	if (use_vbo) {
		glBindBuffer (GL_ARRAY_BUFFER, vbo_id);

		glVertexPointer (3, GL_FLOAT, 0, 0);
	
		if (have_normals) {
			glNormalPointer (GL_FLOAT, 0, (const GLvoid *) normal_offset);
		}

		if (have_colors) {
			glColorPointer (3, GL_FLOAT, 0, (const GLvoid *) (color_offset));
		}

		glEnableClientState (GL_VERTEX_ARRAY);

		if (have_normals) {
			glEnableClientState (GL_NORMAL_ARRAY);
		} else {
			glDisableClientState (GL_NORMAL_ARRAY);
		}

		if (have_colors) {
			glEnableClientState (GL_COLOR_ARRAY);
		} else {
			glDisableClientState (GL_COLOR_ARRAY);
		}
	
		glDrawArrays (mode, 0, vertices.size());
		glBindBuffer (GL_ARRAY_BUFFER, 0);
	} else {
		glBegin (mode);
		for (int vi = 0; vi < vertices.size(); vi++) {
			if (have_colors)
				glColor3fv (colors[vi].data());
			if (have_normals)
				glNormal3fv (normals[vi].data());
			glVertex3fv (vertices[vi].data());

		}
		glEnd();
	}
}


