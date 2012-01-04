#include "GL/glew.h"

#include "ModelData.h"

#include <iostream>

using namespace std;

void MeshData::begin() {
	started = true;

	vertices.resize(0);
	normals.resize(0);
	triangle_indices.resize(0);
}

void MeshData::end() {
	if (normals.size()) {
		if (normals.size() != triangle_indices.size()) {
			std::cerr << "Error: number of normals must equal the number of vertices specified!" << endl;
			exit (1);
		}
	}

	started = false;
}

unsigned int MeshData::generate_vbo() {
	assert (vbo_id == 0);
	assert (started == false);
	assert (vertices.size() != 0);
	assert (vertices.size() % 3 == 0);
	assert (normals.size() == vertices.size());

	// create the buffer
	glGenBuffers (1, &vbo_id);

	// initialize the buffer object
	glBindBuffer (GL_ARRAY_BUFFER, vbo_id);
	glBufferData (GL_ARRAY_BUFFER, sizeof(float) * 3 * (vertices.size() + normals.size()), NULL, GL_STATIC_DRAW);

	// fill the data
	
	// multiple sub buffers
	glBufferSubData (GL_ARRAY_BUFFER, 0, sizeof (float) * 3 * vertices.size(), &vertices[0]);
	glBufferSubData (GL_ARRAY_BUFFER, sizeof(float) * 3 * vertices.size(), sizeof (float) * 3 * normals.size(), &normals[0]);

	glBindBuffer (GL_ARRAY_BUFFER, 0);

	return vbo_id;
}

void MeshData::delete_vbo() {
	assert (vbo_id != 0);
	glDeleteBuffers (1, &vbo_id);

	vbo_id = 0;
}

void MeshData::addVertice (float x, float y, float z) {
	Vector3f vertex;
	vertex[0] = x;
	vertex[1] = y;
	vertex[2] = z;
	vertices.push_back(vertex);

	triangle_indices.push_back (vertices.size() - 1);
}

void MeshData::addVerticefv (float vert[3]) {
	addVertice (vert[0], vert[1], vert[2]);
}

void MeshData::addNormal (float x, float y, float z) {
	Vector3f normal;
	normal[0] = x;
	normal[1] = y;
	normal[2] = z;
	normals.push_back (normal);
}

void MeshData::addNormalfv (float normal[3]) {
	addNormal (normal[0], normal[1], normal[2]);
}

void MeshData::draw() {
	glColor3f (0.8, 0.2, 0.2);
	glBindBuffer (GL_ARRAY_BUFFER, vbo_id);

	glVertexPointer (3, GL_FLOAT, 0, 0);
	glNormalPointer (GL_FLOAT, 0, (const GLvoid *) (vertices.size() * sizeof (float) * 3));

	glEnableClientState (GL_VERTEX_ARRAY);
	glEnableClientState (GL_NORMAL_ARRAY);

	glDrawArrays (GL_TRIANGLES, 0, vertices.size());
	glBindBuffer (GL_ARRAY_BUFFER, 0);
}

/*********************************
 * Segment
 *********************************/

void Segment::draw() {
	glPushMatrix();
	glTranslatef (parent_translation[0], parent_translation[1], parent_translation[2]);
	glRotatef (parent_rotation[0], 0., 0., 1.);
	glRotatef (parent_rotation[1], 0., 1., 0.);
	glRotatef (parent_rotation[2], 1., 0., 0.);
	glScalef (parent_scale[0], parent_scale[1], parent_scale[2]);

	for (unsigned int mi = 0; mi < meshes.size(); mi++) {
		meshes[mi]->draw();
	}

	for (unsigned int si = 0; si < children.size(); si++) {
		children[si]->draw();
	}

	glPopMatrix();
}

/*********************************
 * ModelData
 *********************************/
unsigned int ModelData::addSegment (
			unsigned int parent_segment_index,
			Vector3f parent_translation,
			Vector3f parent_rotation,
			Segment segment) {
	// fill values
	segment.parent_translation = parent_translation;
	segment.parent_rotation = parent_rotation;
	segment.parent_scale.set (1.f, 1.f, 1.f);

	// add segment
	segments.push_back (segment);
	segment_parents.push_back(parent_segment_index);

	unsigned int index;
	index = segments.size() - 1;

	// add segment to its parent
	segments[parent_segment_index].children.push_back (&segments[index]);

	return index;
}

unsigned int ModelData::addSegmentMesh (
		unsigned int segment_index,
		Vector3f translation,
		Vector3f scale,
		MeshData mesh) {
	// fill values
	mesh.parent_translation = translation;
	mesh.parent_scale = scale;

	meshes.push_back (mesh);

	unsigned int mesh_index = meshes.size() - 1;
	segments[segment_index].meshes.push_back(&meshes[mesh_index]);

	return mesh_index;
}

void ModelData::draw() {
	unsigned int si;

	assert (base_segment);

	if (base_segment) {
		base_segment->draw();
	}
}
