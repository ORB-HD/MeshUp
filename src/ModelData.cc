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

	cerr << __func__ << ": vert count = " << vertices.size() << endl;

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
	if (smooth_shading)
		glShadeModel(GL_SMOOTH);
	else
		glShadeModel(GL_FLAT);
		
	glBindBuffer (GL_ARRAY_BUFFER, vbo_id);

	glVertexPointer (3, GL_FLOAT, 0, 0);
	glNormalPointer (GL_FLOAT, 0, (const GLvoid *) (vertices.size() * sizeof (float) * 3));

	glEnableClientState (GL_VERTEX_ARRAY);
	glEnableClientState (GL_NORMAL_ARRAY);

	glDrawArrays (GL_TRIANGLES, 0, vertices.size());
	glBindBuffer (GL_ARRAY_BUFFER, 0);
}

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

	// apply pose transform
	pose_transform =
		smScale (pose_scaling[0], pose_scaling[1], pose_scaling[2])
		* smRotate (pose_rotation_ZYXeuler[0], 1.f, 0.f, 0.f)
		* smRotate (pose_rotation_ZYXeuler[1], 0.f, 1.f, 0.f)
		* smRotate (pose_rotation_ZYXeuler[2], 0.f, 0.f, 1.f)
		* smTranslate (pose_translation[0], pose_translation[1], pose_translation[2])
		* pose_transform;

	for (unsigned int ci = 0; ci < children.size(); ci++) {
		children[ci]->updatePoseTransform (pose_transform);
	}
}

/*********************************
 * BoneAnimationTrack
 *********************************/
BonePose BoneAnimationTrack::interpolatePose (float time) {
	if (poses.size() == 0) {
		return BonePose();
	} else if (poses.size() == 1) {
		return *poses.begin();
	}

	// at this point we have at least two poses
	BonePoseList::iterator pose_iter = poses.begin();

	BonePose start_pose (*pose_iter);
	pose_iter++;
	BonePose end_pose = (*pose_iter);

	// find the two frames that surround the time
	while (pose_iter != poses.end() && end_pose.timestamp <= time) {
		start_pose = end_pose;
		pose_iter++;
		end_pose = *pose_iter;
	}

	// if we overshot we have to return the last valid frame (i.e.
	// start_pose) 
	if (pose_iter == poses.end())
		end_pose = start_pose;

//	cout << "start time = " << start_pose.timestamp << " end time = " << end_pose.timestamp << " query time = " << time << endl;

	// we use end_pose as the result
	float duration = end_pose.timestamp - start_pose.timestamp;
	if (end_pose.timestamp - start_pose.timestamp == 0.f)
		return start_pose;

	float fraction = (time - start_pose.timestamp) / (end_pose.timestamp - start_pose.timestamp);
	
	// some handling for over- and undershooting
	if (fraction > 1.f)
		fraction = 1.f;
	if (fraction < 0.f)
		fraction = 0.f;

	// perform the interpolation
	end_pose.timestamp = start_pose.timestamp + fraction * (end_pose.timestamp - start_pose.timestamp);
	end_pose.translation = start_pose.translation + fraction * (end_pose.translation - start_pose.translation);
	end_pose.rotation_ZYXeuler = start_pose.rotation_ZYXeuler + fraction * (end_pose.rotation_ZYXeuler - start_pose.rotation_ZYXeuler);
	end_pose.scaling = start_pose.scaling + fraction * (end_pose.scaling - start_pose.scaling);

	return end_pose;
}

/*********************************
 * ModelData
 *********************************/
void ModelData::addBone (
		const std::string &parent_bone_name,
		const std::string &bone_name,
		const Vector3f &parent_translation,
		const Vector3f &parent_rotation_ZYXeuler) {

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
		const MeshData &mesh,
		const Vector3f &mesh_center) {
	Segment segment;
	segment.name = segment_name;
	segment.dimensions = dimensions;
	segment.color = color;
	segment.mesh = mesh;
	segment.meshcenter = mesh_center;
	segment.bone = findBone (bone_name.c_str());

	assert (segment.bone != NULL);
	segments.push_back (segment);
}

void ModelData::addBonePose (
		const std::string &bone_name,
		float time,
		const Vector3f &bone_translation,
		const Vector3f &bone_rotation_ZYXeuler,
		const Vector3f &bone_scaling
		) {
	BonePtr bone = findBone (bone_name.c_str());
	BonePose pose;
	pose.timestamp = time;
	pose.translation = bone_translation;
	pose.rotation_ZYXeuler = bone_rotation_ZYXeuler;
	pose.scaling = bone_scaling;

	animation.bonetracks[bone].poses.push_back(pose);

	// update the duration of the animation
	if (time > animation.duration)
		animation.duration = time;
}

void ModelData::updatePose(float time_sec) {
	// if there is no animation we can return
	if (animation.bonetracks.size() != 0) {
		animation.current_time += time_sec;
		if (animation.current_time > animation.duration) {
			if (animation.loop) {
				while (animation.current_time > animation.duration)
					animation.current_time -= animation.duration;
			} else {
				animation.current_time = animation.duration;
			}
		}
	}
	BoneAnimationTrackMap::iterator bone_track_iter = animation.bonetracks.begin();

	while (bone_track_iter != animation.bonetracks.end()) {
		BonePose pose = bone_track_iter->second.interpolatePose (animation.current_time);
		bone_track_iter->first->pose_translation = pose.translation;
		bone_track_iter->first->pose_rotation_ZYXeuler = pose.rotation_ZYXeuler;
		bone_track_iter->first->pose_scaling = pose.scaling;

		bone_track_iter++;
	}
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

		// we also have to apply the scaling after the transform:
		Matrix44f transform_matrix = 
			smScale (seg_iter->dimensions[0], seg_iter->dimensions[1], seg_iter->dimensions[2])
			* smTranslate (seg_iter->meshcenter[0], seg_iter->meshcenter[1], seg_iter->meshcenter[2])
			* seg_iter->bone->pose_transform;
		glMultMatrixf (transform_matrix.data());

		// drawing
		glColor3f (seg_iter->color[0], seg_iter->color[1], seg_iter->color[2]);
		seg_iter->mesh.draw();
		glPopMatrix();

		seg_iter++;
	}
}
