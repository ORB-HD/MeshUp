#include "GL/glew.h"

#include "ModelData.h"

#include "SimpleMathGL.h"

#include "json/json.h"

#include <iostream>
#include <iomanip>
#include <fstream>
#include <ostream>
#include <stack>

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
		const std::string &mesh_name,
		const Vector3f &mesh_center) {
	Segment segment;
	segment.name = segment_name;
	segment.dimensions = dimensions;
	segment.color = color;

	// check whether we have the mesh, if not try to load it
	MeshMap::iterator mesh_iter = meshmap.find (mesh_name);
	if (mesh_iter == meshmap.end()) {
		// load it
		cout << "Loading mesh " << mesh_name << endl;

		MeshPtr new_mesh (new MeshData);
		loadOBJ (&(*new_mesh), mesh_name.c_str());
		new_mesh->generate_vbo();

		meshmap[mesh_name] = new_mesh;

		mesh_iter = meshmap.find (mesh_name);
	}

	segment.mesh = mesh_iter->second;
	segment.meshcenter = mesh_center;
	segment.bone = findBone (bone_name.c_str());
	segment.mesh_filename = mesh_name;
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
		seg_iter->mesh->draw();
		glPopMatrix();

		seg_iter++;
	}
}

void serialize_bone (ostream &stream_out, const std::string& parent_name, BonePtr bone, const string &tabs) {
	stream_out << tabs << "\"" << bone->name << "\" : {" << endl;
	stream_out << tabs << "  \"parent_bone\": \"" << parent_name << "\"," << endl;
	stream_out << tabs << "  \"parent_translation\": " << bone->parent_translation.transpose() << "," << endl;
	stream_out << tabs << "  \"parent_rotation_ZYXeuler\": " << bone->parent_rotation_ZYXeuler.transpose() << "," << endl;
	stream_out << tabs << "}," << endl;
}

void serialize_segment (ostream &stream_out, const Segment &segment, const string &tabs) {
	stream_out << tabs << "\"" << segment.name << "\" : {" << endl;
	stream_out << tabs << "  \"dimensions\": " << segment.dimensions.transpose() << "," << endl;
	stream_out << tabs << "  \"color\": " << segment.color.transpose() << "," << endl;
	stream_out << tabs << "  \"meshcenter\": " << segment.meshcenter.transpose() << "," << endl;
	stream_out << tabs << "  \"mesh_filename\": \"" << segment.mesh_filename << "\"," << endl;
	stream_out << tabs << "}," << endl;
}

Json::Value vec3_to_json (const Vector3f &vec) {
	Json::Value result;
	result[0] = vec[0];
	result[1] = vec[1];
	result[2] = vec[2];
	return result;
}

Vector3f json_to_vec3 (const Json::Value &value) {
	Vector3f result (
			value.get(Json::ArrayIndex(0),0.f).asFloat(),
			value.get(Json::ArrayIndex(1),0.f).asFloat(),
			value.get(Json::ArrayIndex(2),0.f).asFloat()
			);
}

Json::Value bone_to_json_value (const BonePtr &bone) {
	using namespace Json;

	Value result;

	result["name"] = bone->name;
	result["parent_translation"] = vec3_to_json(bone->parent_translation);
	result["parent_rotation_ZYXeuler"] = vec3_to_json(bone->parent_rotation_ZYXeuler);

	return result;
}

Json::Value segment_to_json_value (const Segment &segment) {
	using namespace Json;

	Value result;

	result["name"] = segment.name;
	result["dimensions"] = vec3_to_json (segment.dimensions);
	result["color"] = vec3_to_json (segment.color);
	result["mesh_center"] = vec3_to_json (segment.meshcenter);
	result["mesh_filename"] = segment.mesh_filename;
	result["bone"] = segment.bone->name;

	return result;
}

void ModelData::saveToFile (const char* filename) {
	Json::Value root_node;

	int bone_index = 0;
	// we have to write out the bones recursively
	for (int bi = 0; bi < bones.size(); bi++) {
		stack<BonePtr> bone_stack;
		bone_stack.push (bones[bi]);

		stack<int> child_index_stack;
		if (bone_stack.top()->children.size() > 0) {
			child_index_stack.push(0);
		}

		if (bone_stack.top()->name != "BASE") {
			root_node["bones"][bone_index] = bone_to_json_value(bone_stack.top());
			bone_index++;
		}

		while (bone_stack.size() > 0) {
			BonePtr cur_bone = bone_stack.top();
			int child_idx = child_index_stack.top();

			if (child_idx < cur_bone->children.size()) {
				BonePtr child_bone = cur_bone->children[child_idx];

				root_node["bones"][bone_index] = bone_to_json_value(child_bone);
				root_node["bones"][bone_index]["parent"] = cur_bone->name;
				bone_index++;
				
				child_index_stack.pop();
				child_index_stack.push (child_idx + 1);

				if (child_bone->children.size() > 0) {
					bone_stack.push (child_bone);
					child_index_stack.push(0);
				}
			} else {
				bone_stack.pop();
				child_index_stack.pop();
			}

//			bone_stack.pop();
		}
	}

	// segments
	
	int segment_index = 0;
	SegmentList::iterator seg_iter = segments.begin();
	while (seg_iter != segments.end()) {
		root_node["segments"][segment_index] = segment_to_json_value (*seg_iter);

		segment_index++;
		seg_iter++;
	}

	ofstream file_out (filename, ios::trunc);
	file_out << root_node << endl;
	file_out.close();

	cout << root_node << endl;
}

void ModelData::loadFromFile (const char* filename) {
	using namespace Json;
	Value root;
	Reader reader;

	ifstream file_in (filename);
	stringstream buffer;
	buffer << file_in.rdbuf();

	bool parsing_result = reader.parse (buffer.str(), root);
	if (!parsing_result) {
		cerr << "Error reading model: " << reader.getFormattedErrorMessages();

		exit (1);

		return;
	}

	// clear the model
	clear();

	// read the bones:
	ValueIterator node_iter = root["bones"].begin();

	while (node_iter != root["bones"].end()) {
		Value bone_node = *node_iter;

		addBone (
				bone_node["parent"].asString(),
				bone_node["name"].asString(),
				json_to_vec3 (bone_node["parent_translation"]),
				json_to_vec3 (bone_node["parent_translation"])
				);

		node_iter++;
	}

	node_iter = root["segments"].begin();
	while (node_iter != root["segments"].end()) {
		Value segment_node = *node_iter;

		addSegment (
				segment_node["bone"].asString(),
				segment_node["name"].asString(),
				json_to_vec3 (segment_node["dimensions"]),
				json_to_vec3 (segment_node["color"]),
				segment_node["mesh_filename"].asString(),
				json_to_vec3 (segment_node["mesh_center"])
				);

		node_iter++;
	}
}
