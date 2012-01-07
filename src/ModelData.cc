#include "GL/glew.h"

#include "ModelData.h"

#include "SimpleMathGL.h"

#include <iostream>
#include <iomanip>

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

/*
void Segment::draw() {
	glPushMatrix();
	glTranslatef (parent_translation[0], parent_translation[1], parent_translation[2]);
	glRotatef (parent_rotation[0], 0., 0., 1.);
	glRotatef (parent_rotation[1], 0., 1., 0.);
	glRotatef (parent_rotation[2], 1., 0., 0.);
	glScalef (parent_scale[0], parent_scale[1], parent_scale[2]);

	//cerr << "segment " << hex << this << " (mesh count = " << meshes.size() << ", child count = " << children.size() << ")" << endl;

	for (unsigned int mi = 0; mi < meshes.size(); mi++) {
		//cerr << "draw mesh " << mi << endl;
		meshes[mi]->draw();
	}

	for (unsigned int si = 0; si < children.size(); si++) {
		children[si]->draw();
	}

	glPopMatrix();
}
*/

/*********************************
 * Bone
 *********************************/
void Bone::updatePoseTransform(const Matrix44f &parent_pose_transform) {
	// first translate, then rotate Z, Y, X
	pose_transform = 
		smRotate (parent_rotation_ZYXeuler[0], 1.f, 0.f, 0.f)
		* smRotate (parent_rotation_ZYXeuler[1], 0.f, 1.f, 0.f)
		* smRotate (parent_rotation_ZYXeuler[2], 0.f, 0.f, 1.f)
		* smTranslate (parent_translation[0], parent_translation[1], parent_translation[2])
			* parent_pose_transform;

	for (unsigned int ci; ci < children.size(); ci++) {
		children[ci]->updatePoseTransform (pose_transform);
	}
}

/*********************************
 * ModelData
 *********************************/
void ModelData::addBone (const std::string &parent_bone_name,
		const Vector3f &parent_translation,
		const Vector3f &parent_rotation_ZYXeuler,
		const char* bone_name) {

	// create the bone
	BonePtr bone (new Bone);
	bone->name = bone_name;
	bone->parent_translation = parent_translation;
	bone->parent_rotation_ZYXeuler = parent_rotation_ZYXeuler;

	// first find the bone
	BonePtr parent_bone = findBone (parent_bone_name.c_str());
	if (parent_bone == NULL) {
		cerr << "Could not find bone '" << parent_bone_name << "'!" << endl;
		exit (1);
	}

	parent_bone->children.push_back (bone);
	bonemap[bone_name] = bone;
}

void ModelData::addSegment (
		const std::string &bone_name,
		const std::string &segment_name,
		const Vector3f &dimensions,
		const Vector3f &color,
		const MeshData &mesh) {
	Segment segment;
	segment.name = segment_name;
	segment.dimensions = dimensions;
	segment.color = color;
	segment.mesh = mesh;
	segment.bone = findBone (bone_name.c_str());

	assert (segment.bone != NULL);
	segments.push_back (segment);
}

void ModelData::updateBones() {
	Matrix44f base_transform (Matrix44f::Identity());

	for (unsigned int bi = 0; bi < bones.size(); bi++) {
		bones[bi]->updatePoseTransform (base_transform);
	}
}

void ModelData::draw() {
	updateBones();

	SegmentList::iterator seg_iter = segments.begin();
	
	while (seg_iter != segments.end()) {
		glPushMatrix();
		glMultMatrixf (seg_iter->bone->pose_transform.data());
		glColor3f (seg_iter->color[0], seg_iter->color[1], seg_iter->color[2]);
		seg_iter->mesh.draw();
		glPopMatrix();

		seg_iter++;
	}
}
