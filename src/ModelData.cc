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

const bool use_vbo = true;

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

//	cerr << __func__ << ": vert count = " << vertices.size() << endl;

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

	if (use_vbo) {
		glBindBuffer (GL_ARRAY_BUFFER, vbo_id);

		glVertexPointer (3, GL_FLOAT, 0, 0);
		glNormalPointer (GL_FLOAT, 0, (const GLvoid *) (vertices.size() * sizeof (float) * 3));

		glEnableClientState (GL_VERTEX_ARRAY);
		glEnableClientState (GL_NORMAL_ARRAY);

		glDrawArrays (GL_TRIANGLES, 0, vertices.size());
		glBindBuffer (GL_ARRAY_BUFFER, 0);
	} else {
		glBegin (GL_TRIANGLES);
		for (int vi = 0; vi < vertices.size(); vi++) {
			glNormal3fv (normals[vi].data());
			glVertex3fv (vertices[vi].data());
		}
		glEnd();
	}
}

/*********************************
 * Frame
 *********************************/
void Frame::updatePoseTransform(const Matrix44f &parent_pose_transform, const FrameConfiguration &config) {
	// first translate, then rotate as specified in the angles
	pose_transform = 
		frame_transform
//		rotation_angles_to_matrix (parent_rotation)
//		* smTranslate (parent_translation[0], parent_translation[1], parent_translation[2])
		* parent_pose_transform;

	// apply pose transform
	pose_transform =
		smScale (pose_scaling[0], pose_scaling[1], pose_scaling[2])
	  * pose_rotation_quaternion.toGLMatrix()
		* smTranslate (pose_translation[0], pose_translation[1], pose_translation[2])
		* pose_transform;

	for (unsigned int ci = 0; ci < children.size(); ci++) {
		children[ci]->updatePoseTransform (pose_transform, config);
	}
}

void Frame::initFrameTransform(const Matrix44f &parent_frame_transform, const FrameConfiguration &config) {
	// first translate, then rotate as specified in the angles
	frame_transform =	rotation_angles_to_matrix (parent_rotation, config)
		* smTranslate (parent_translation[0], parent_translation[1], parent_translation[2]);

	for (unsigned int ci = 0; ci < children.size(); ci++) {
		children[ci]->initFrameTransform (frame_transform, config);
	}
}

/*********************************
 * FrameAnimationTrack
 *********************************/
FramePose FrameAnimationTrack::interpolatePose (float time) {
	if (poses.size() == 0) {
		return FramePose();
	} else if (poses.size() == 1) {
		return *poses.begin();
	}

	// at this point we have at least two poses
	FramePoseList::iterator pose_iter = poses.begin();

	FramePose start_pose (*pose_iter);
	pose_iter++;
	FramePose end_pose = (*pose_iter);

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
	end_pose.rotation = start_pose.rotation + fraction * (end_pose.rotation - start_pose.rotation);
	end_pose.rotation_quaternion = start_pose.rotation_quaternion.slerp (fraction, end_pose.rotation_quaternion);
	end_pose.scaling = start_pose.scaling + fraction * (end_pose.scaling - start_pose.scaling);

	return end_pose;
}

/*********************************
 * ModelData
 *********************************/
void ModelData::addFrame (
		const std::string &parent_frame_name,
		const std::string &frame_name,
		const Vector3f &parent_translation,
		const Vector3f &parent_rotation) {
	// mark frame transformations as dirty
	frames_initialized = false;

	// create the frame
	FramePtr frame (new Frame);
	frame->name = frame_name;
	frame->parent_translation = configuration.axes_rotation.transpose() * parent_translation;
	frame->parent_rotation = parent_rotation;

	// first find the frame
	FramePtr parent_frame = findFrame (parent_frame_name.c_str());
	if (parent_frame == NULL) {
		cerr << "Could not find frame '" << parent_frame_name << "'!" << endl;
		exit (1);
	}

	parent_frame->children.push_back (frame);
	framemap[frame_name] = frame;
}

void ModelData::addSegment (
		const std::string &frame_name,
		const std::string &segment_name,
		const Vector3f &dimensions,
		const Vector3f &color,
		const std::string &mesh_name,
		const Vector3f &mesh_center) {
	Segment segment;
	segment.name = segment_name;
	segment.dimensions = configuration.axes_rotation.transpose() * dimensions;

	// make sure that the dimensions are all positive
	for (int i = 0; i < 3; i++) {
	if (segment.dimensions[i] < 0)
		segment.dimensions[i] = -segment.dimensions[i];
	}

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
	segment.meshcenter = configuration.axes_rotation.transpose() * mesh_center;
	segment.frame = findFrame (frame_name.c_str());
	segment.mesh_filename = mesh_name;
	assert (segment.frame != NULL);
	segments.push_back (segment);
}

void ModelData::addFramePose (
		const std::string &frame_name,
		float time,
		const Vector3f &frame_translation,
		const Vector3f &frame_rotation,
		const Vector3f &frame_scaling
		) {
	FramePtr frame = findFrame (frame_name.c_str());
	FramePose pose;
	pose.timestamp = time;
	pose.translation = frame_translation;
	pose.rotation = frame_rotation;
	pose.rotation_quaternion = rotation_angles_to_quaternion (frame_rotation, configuration);
	pose.scaling = frame_scaling;

	animation.frametracks[frame].poses.push_back(pose);

	// update the duration of the animation
	if (time > animation.duration)
		animation.duration = time;
}

void ModelData::initFrameTransform() {
	Matrix44f base_transform (Matrix44f::Identity());

	for (unsigned int bi = 0; bi < frames.size(); bi++) {
		frames[bi]->initFrameTransform (base_transform, configuration);
	}

	frames_initialized = true;
}

void ModelData::updatePose() {
	// if there is no animation we can return
	if (animation.frametracks.size() != 0) {
		if (animation.current_time > animation.duration) {
			if (animation.loop) {
				while (animation.current_time > animation.duration)
					animation.current_time -= animation.duration;
			} else {
				animation.current_time = animation.duration;
			}
		}
	}
	FrameAnimationTrackMap::iterator frame_track_iter = animation.frametracks.begin();

	while (frame_track_iter != animation.frametracks.end()) {
		FramePose pose = frame_track_iter->second.interpolatePose (animation.current_time);
		frame_track_iter->first->pose_translation = pose.translation;
		frame_track_iter->first->pose_rotation = pose.rotation;
		frame_track_iter->first->pose_rotation_quaternion = pose.rotation_quaternion;
		frame_track_iter->first->pose_scaling = pose.scaling;

		frame_track_iter++;
	}
}

void ModelData::updateFrames() {
	Matrix44f base_transform (Matrix44f::Identity());

	// check whether the frame transformations are valid
	if (frames_initialized == false)
		initFrameTransform();

	for (unsigned int bi = 0; bi < frames.size(); bi++) {
		frames[bi]->updatePoseTransform (base_transform, configuration);
	}
}

void ModelData::draw() {
	updateFrames();

	SegmentList::iterator seg_iter = segments.begin();

	while (seg_iter != segments.end()) {
		glPushMatrix();

		// we also have to apply the scaling after the transform:
		Matrix44f transform_matrix = 
			smScale (seg_iter->dimensions[0], seg_iter->dimensions[1], seg_iter->dimensions[2])
			* smTranslate (seg_iter->meshcenter[0], seg_iter->meshcenter[1], seg_iter->meshcenter[2])
			* seg_iter->frame->pose_transform;
		glMultMatrixf (transform_matrix.data());

		// drawing
		glColor3f (seg_iter->color[0], seg_iter->color[1], seg_iter->color[2]);
		seg_iter->mesh->draw();
		glPopMatrix();

		seg_iter++;
	}
}

Json::Value vec3_to_json (const Vector3f &vec) {
	Json::Value result;
	result[0] = Json::Value(static_cast<float>(vec[0]));
	result[1] = Json::Value(static_cast<float>(vec[1]));
	result[2] = Json::Value(static_cast<float>(vec[2]));

	return result;
}

Vector3f json_to_vec3 (const Json::Value &value) {
	if (value.isNull())
		return Vector3f (0.f, 0.f, 0.f);

	Vector3f result (
			value[0].asFloat(),
			value[1].asFloat(),
			value[2].asFloat()
			);

	return result;
}

Json::Value frame_configuration_to_json_value (const FrameConfiguration &config) {
	using namespace Json;

	Value result;
	result["axis_front"] = vec3_to_json (config.axis_front);
	result["axis_up"] = vec3_to_json (config.axis_up);
	result["axis_right"] = vec3_to_json (config.axis_right);

	result["rotation_order"][0] = config.rotation_order[0];
	result["rotation_order"][1] = config.rotation_order[1];
	result["rotation_order"][2] = config.rotation_order[2];

	return result;
}

Json::Value frame_to_json_value (const FramePtr &frame) {
	using namespace Json;

	Value result;

	result["name"] = frame->name;
	result["parent_translation"] = vec3_to_json(frame->parent_translation);
	result["parent_rotation"] = vec3_to_json(frame->parent_rotation);

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
	result["frame"] = segment.frame->name;

	return result;
}

void ModelData::saveToFile (const char* filename) {
	// we absoulutely have to set the locale to english for numbers.
	// Otherwise we might wrongly formatted data. 
	std::setlocale(LC_NUMERIC, "POSIX");
	Json::Value root_node;

	root_node["configuration"] = frame_configuration_to_json_value (configuration);

	int frame_index = 0;
	// we have to write out the frames recursively
	for (int bi = 0; bi < frames.size(); bi++) {
		stack<FramePtr> frame_stack;
		frame_stack.push (frames[bi]);

		stack<int> child_index_stack;
		if (frame_stack.top()->children.size() > 0) {
			child_index_stack.push(0);
		}

		if (frame_stack.top()->name != "BASE") {
			root_node["frames"][frame_index] = frame_to_json_value(frame_stack.top());
			frame_index++;
		}

		while (frame_stack.size() > 0) {
			FramePtr cur_frame = frame_stack.top();
			int child_idx = child_index_stack.top();

			if (child_idx < cur_frame->children.size()) {
				FramePtr child_frame = cur_frame->children[child_idx];

				root_node["frames"][frame_index] = frame_to_json_value(child_frame);
				root_node["frames"][frame_index]["parent"] = cur_frame->name;
				frame_index++;
				
				child_index_stack.pop();
				child_index_stack.push (child_idx + 1);

				if (child_frame->children.size() > 0) {
					frame_stack.push (child_frame);
					child_index_stack.push(0);
				}
			} else {
				frame_stack.pop();
				child_index_stack.pop();
			}

//			frame_stack.pop();
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
}

void ModelData::loadFromFile (const char* filename) {
	// we absoulutely have to set the locale to english for numbers.
	// Otherwise we might read false values due to the wrong conversion.
	std::setlocale(LC_NUMERIC, "POSIX");

	using namespace Json;
	Value root;
	Reader reader;

	ifstream file_in (filename);

	if (!file_in) {
		cerr << "Error opening file " << filename << "!" << endl;
		exit(1);
	}

	stringstream buffer;
	buffer << file_in.rdbuf();
	file_in.close();

	bool parsing_result = reader.parse (buffer.str(), root);
	if (!parsing_result) {
		cerr << "Error reading model: " << reader.getFormattedErrorMessages();

		exit (1);

		return;
	}

	// clear the model
	clear();

	// read the configuration, fill with default values if they do not exist
	if (root["configuration"]["axis_front"].isNull())
		root["configuration"]["axis_front"] = vec3_to_json (Vector3f (1.f, 0.f, 0.f));
	if (root["configuration"]["axis_up"].isNull())
		root["configuration"]["axis_up"] = vec3_to_json (Vector3f (0.f, 1.f, 0.f));
	if (root["configuration"]["axis_right"].isNull())
		root["configuration"]["axis_right"] = vec3_to_json (Vector3f (0.f, 0.f, 1.f));
	if (root["configuration"]["rotation_order"][0].isNull())
		root["configuration"]["rotation_order"][0] = 2;
	if (root["configuration"]["rotation_order"][1].isNull())
		root["configuration"]["rotation_order"][1] = 1;
	if (root["configuration"]["rotation_order"][2].isNull())
		root["configuration"]["rotation_order"][2] = 0;

	configuration.axis_front = json_to_vec3(root["configuration"]["axis_front"]);
	configuration.axis_up = json_to_vec3(root["configuration"]["axis_up"]);
	configuration.axis_right = json_to_vec3(root["configuration"]["axis_right"]);
	configuration.rotation_order[0] = root["configuration"]["rotation_order"][0].asInt();
	configuration.rotation_order[1] = root["configuration"]["rotation_order"][1].asInt();
	configuration.rotation_order[2] = root["configuration"]["rotation_order"][2].asInt();

	configuration.update();

//	cout << "front: " << configuration.axis_front.transpose() << endl;
//	cout << "up   : " << configuration.axis_up.transpose() << endl;
//	cout << "right: " << configuration.axis_right.transpose() << endl;
//
//	cout << "rot  : " << configuration.rotation_order[0] 
//		<< ", " << configuration.rotation_order[1] 
//		<< ", " << configuration.rotation_order[2] << endl;
//
//	cout << "axes: " << endl << configuration.axes_rotation << endl;	

	// read the frames:
	ValueIterator node_iter = root["frames"].begin();

	while (node_iter != root["frames"].end()) {
		Value frame_node = *node_iter;

		addFrame (
				frame_node["parent"].asString(),
				frame_node["name"].asString(),
				json_to_vec3 (frame_node["parent_translation"]),
				json_to_vec3 (frame_node["parent_rotation"])
				);

		node_iter++;
	}

	node_iter = root["segments"].begin();
	while (node_iter != root["segments"].end()) {
		Value segment_node = *node_iter;

		addSegment (
				segment_node["frame"].asString(),
				segment_node["name"].asString(),
				json_to_vec3 (segment_node["dimensions"]),
				json_to_vec3 (segment_node["color"]),
				segment_node["mesh_filename"].asString(),
				json_to_vec3 (segment_node["mesh_center"])
				);

		node_iter++;
	}

	initFrameTransform();
}
