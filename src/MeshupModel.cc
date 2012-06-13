#include "GL/glew.h"

#include "MeshupModel.h"

#include "SimpleMath/SimpleMathGL.h"
#include "string_utils.h"

#include "json/json.h"

#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <ostream>
#include <stack>
#include <limits>

#include <boost/filesystem.hpp>

#include "objloader.h"

using namespace std;

const string invalid_id_characters = "{}[],;: \r\n\t";

std::string find_model_file_by_name (const std::string &model_name) {
	std::string result;

	std::vector<std::string> paths;
	paths.push_back("./");
	paths.push_back("./models/");

	if (getenv ("MESHUP_PATH")) {
		std::string env_meshup_dir (getenv("MESHUP_PATH"));

		if (env_meshup_dir.size() != 0) {
			if (env_meshup_dir[env_meshup_dir.size() - 1] != '/')
				env_meshup_dir += '/';

			paths.push_back (env_meshup_dir);
			paths.push_back (env_meshup_dir + "models/");
		}
	}

	paths.push_back("/usr/local/share/meshup/models/");
	paths.push_back("/usr/share/meshup/models/");

	std::vector<std::string>::iterator iter = paths.begin();
	string model_filename;
	for (iter; iter != paths.end(); iter++) {
		model_filename = *iter + model_name;

//		cout << "checking " << model_filename << endl;
		if (boost::filesystem::is_regular_file(model_filename))
			break;

		model_filename += ".json";
//		cout << "checking " << model_filename << endl;

		if (boost::filesystem::is_regular_file(model_filename))
			break;
	}

	if (iter != paths.end())
		return model_filename;

	return std::string("");
}

std::string find_mesh_file_by_name (const std::string &filename) {
	std::string result;

	std::vector<std::string> paths;
	paths.push_back("./");

	if (getenv ("MESHUP_PATH")) {
		std::string env_meshup_dir (getenv("MESHUP_PATH"));

		if (env_meshup_dir.size() != 0) {
			if (env_meshup_dir[env_meshup_dir.size() - 1] != '/')
				env_meshup_dir += '/';

			paths.push_back (env_meshup_dir);
		}
	}

	paths.push_back("/usr/local/share/meshup/meshes/");
	paths.push_back("/usr/share/meshup/meshes/");

	std::vector<std::string>::iterator iter = paths.begin();
	for (iter; iter != paths.end(); iter++) {
		std::string test_path = *iter;

		if (!boost::filesystem::is_regular_file(test_path + filename))
			continue;

		break;
	}

	if (iter != paths.end())
		return (string(*iter) + string(filename));

	cerr << "Could not find mesh file " << filename << ". Search path: " << endl;
	for (iter = paths.begin(); iter != paths.end(); iter++) {
		cout << "  " << *iter << endl;
	}
	exit(1);

	return std::string("");
}

/*
 * Frame
 */
void Frame::updatePoseTransform(const Matrix44f &parent_pose_transform, const FrameConfig &config) {
	// first translate, then rotate as specified in the angles
	pose_transform = 
		frame_transform
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

void Frame::initFrameTransform(const Matrix44f &parent_frame_transform, const FrameConfig &config) {
	// first translate, then rotate as specified in the angles
	frame_transform =	config.convertAnglesToMatrix (parent_rotation)
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
 * MeshupModel
 *********************************/
void MeshupModel::addFrame (
		const std::string &parent_frame_name,
		const std::string &frame_name,
		const Vector3f &parent_translation,
		const Vector3f &parent_rotation) {
	// mark frame transformations as dirty
	frames_initialized = false;

	// check for invalid characters
	if (frame_name.find_first_of (invalid_id_characters) != string::npos) {
		cerr << "Error: Found invalid character '"
			<< frame_name[frame_name.find_first_of (invalid_id_characters)]
			<< "' in frame name '" << frame_name << "'!" << endl;
		exit (1);
	}

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

void MeshupModel::addSegment (
		const std::string &frame_name,
		const std::string &segment_name,
		const Vector3f &dimensions,
		const Vector3f &scale,
		const Vector3f &color,
		const std::string &mesh_name,
		const Vector3f &translate,
		const Vector3f &mesh_center) {
	Segment segment;
	segment.name = segment_name;
	segment.dimensions = configuration.axes_rotation.transpose() * dimensions;

	//~ // make sure that the dimensions are all positive
	//~ for (int i = 0; i < 3; i++) {
	//~ if (segment.dimensions[i] < 0)
		//~ segment.dimensions[i] = -segment.dimensions[i];
	//~ }

	segment.color = color;
	segment.scale = scale;
	segment.translate = translate;

	// check whether we have the mesh, if not try to load it
	MeshMap::iterator mesh_iter = meshmap.find (mesh_name);
	if (mesh_iter == meshmap.end()) {
		MeshPtr new_mesh (new MeshVBO);

		// check whether we want to extract a sub object within the obj file
		string mesh_filename = mesh_name;
		string submesh_name = "";
		if (mesh_name.find (':') != string::npos) {
			submesh_name = mesh_name.substr (mesh_name.find(':') + 1, mesh_name.size());
			mesh_filename = mesh_name.substr (0, mesh_name.find(':'));
			string mesh_file_location = find_mesh_file_by_name (mesh_filename);
			cout << "Loading sub object " << submesh_name << " from file " << mesh_file_location << endl;
			load_obj (*new_mesh, mesh_file_location.c_str(), submesh_name.c_str());
		} else {
			string mesh_file_location = find_mesh_file_by_name (mesh_name);
			cout << "Loading mesh " << mesh_file_location << endl;
			load_obj (*new_mesh, mesh_file_location.c_str());
		}

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

void MeshupModel::addFramePose (
		const std::string &frame_name,
		float time,
		const Vector3f &frame_translation,
		const Vector3f &frame_rotation,
		const Vector3f &frame_scaling
		) {
	FramePtr frame = findFrame (frame_name.c_str());
	FramePose pose;
	pose.timestamp = time;
	pose.translation = configuration.axes_rotation.transpose() * frame_translation;
	pose.rotation = frame_rotation;
	pose.rotation_quaternion = configuration.convertAnglesToQuaternion (frame_rotation);
	pose.scaling = frame_scaling;

	animation.frametracks[frame].poses.push_back(pose);

	// update the duration of the animation
	if (time > animation.duration)
		animation.duration = time;
}

void MeshupModel::initFrameTransform() {
	Matrix44f base_transform (Matrix44f::Identity());

	for (unsigned int bi = 0; bi < frames.size(); bi++) {
		frames[bi]->initFrameTransform (base_transform, configuration);
	}

	frames_initialized = true;
}

void MeshupModel::updatePose() {
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

void MeshupModel::updateFrames() {
	Matrix44f base_transform (Matrix44f::Identity());

	// check whether the frame transformations are valid
	if (frames_initialized == false)
		initFrameTransform();

	for (unsigned int bi = 0; bi < frames.size(); bi++) {
		frames[bi]->updatePoseTransform (base_transform, configuration);
	}
}

void MeshupModel::updateSegments() {
	SegmentList::iterator seg_iter = segments.begin();

	while (seg_iter != segments.end()) {
		Vector3f bbox_size (seg_iter->mesh->bbox_max - seg_iter->mesh->bbox_min);

		Vector3f scale(1.0f,1.0f,1.0f) ;

		//only scale, if the dimensions are valid, i.e. are set in json-File
		if (seg_iter->dimensions[0] != 0.f) {
			scale = Vector3f(
					fabs(seg_iter->dimensions[0]) / bbox_size[0],
					fabs(seg_iter->dimensions[1]) / bbox_size[1],
					fabs(seg_iter->dimensions[2]) / bbox_size[2]
					);
		} else if (seg_iter->scale[0] > 0.f) {
			scale=seg_iter->scale;
		}
		
		Vector3f translate(0.0f,0.0f,0.0f);
		//only translate with meshcenter if it is defined in json file
		if (!isnan(seg_iter->meshcenter[0])) {
				Vector3f center ( seg_iter->mesh->bbox_min + bbox_size * 0.5f);
				translate[0] = -center[0] * scale[0] + seg_iter->meshcenter[0];
				translate[1] = -center[1] * scale[1] + seg_iter->meshcenter[1];
				translate[2] = -center[2] * scale[2] + seg_iter->meshcenter[2];
		}
		translate+=seg_iter->translate;
		
		// we also have to apply the scaling after the transform:
		seg_iter->gl_matrix = 
			smScale (scale[0], scale[1], scale[2])
			* smTranslate (translate[0], translate[1], translate[2])
			* seg_iter->frame->pose_transform;

		seg_iter++;
	}
}

void MeshupModel::draw() {
	// save current state of GL_NORMALIZE to properly restore the original
	// state
	bool normalize_enabled = glIsEnabled (GL_NORMALIZE);
	if (!normalize_enabled)
		glEnable (GL_NORMALIZE);

	updateSegments();

	SegmentList::iterator seg_iter = segments.begin();

	while (seg_iter != segments.end()) {
		glPushMatrix();
		
		glMultMatrixf (seg_iter->gl_matrix.data());

		// drawing
		glColor3f (seg_iter->color[0], seg_iter->color[1], seg_iter->color[2]);
		
		seg_iter->mesh->draw();

		glPopMatrix();

		seg_iter++;
	}

	// disable normalize if it was previously not enabled
	if (!normalize_enabled)
		glDisable (GL_NORMALIZE);
}

void MeshupModel::drawFrameAxes() {
	// backup the depth test and line width values
	bool depth_test_enabled = glIsEnabled (GL_DEPTH_TEST);
	if (depth_test_enabled)
		glDisable (GL_DEPTH_TEST);

	bool light_enabled = glIsEnabled (GL_LIGHTING);
	if (light_enabled)
		glDisable (GL_LIGHTING);

	float line_width;
	glGetFloatv (GL_LINE_WIDTH, &line_width);

	glLineWidth (2.f);

	// for the rotation of the axes
	Matrix44f axes_rotation_matrix (Matrix44f::Identity());
	axes_rotation_matrix.block<3,3> (0,0) = configuration.axes_rotation;

	FrameMap::iterator frame_iter = framemap.begin();

	while (frame_iter != framemap.end()) {
		if (frame_iter->second->name == "BASE") {
			frame_iter++;
			continue;
		}
		glPushMatrix();

			
		Matrix44f transform_matrix = axes_rotation_matrix * frame_iter->second->pose_transform;
		glMultMatrixf (transform_matrix.data());

		glBegin (GL_LINES);
		glColor3f (1.f, 0.f, 0.f);
		glVertex3f (0.f, 0.f, 0.f);
		glVertex3f (0.1f, 0.f, 0.f);
		glColor3f (0.f, 1.f, 0.f);
		glVertex3f (0.f, 0.f, 0.f);
		glVertex3f (0.f, 0.1f, 0.f);
		glColor3f (0.f, 0.f, 1.f);
		glVertex3f (0.f, 0.f, 0.f);
		glVertex3f (0.f, 0.f, 0.1f);
		glEnd();

		glPopMatrix();

		frame_iter++;
	}

	if (depth_test_enabled)
		glEnable (GL_DEPTH_TEST);

	if (light_enabled)
		glEnable (GL_LIGHTING);

	glLineWidth (line_width);
}

void MeshupModel::drawBaseFrameAxes() {
	// backup the depth test and line width values
	bool depth_test_enabled = glIsEnabled (GL_DEPTH_TEST);
	if (depth_test_enabled)
		glDisable (GL_DEPTH_TEST);

	bool light_enabled = glIsEnabled (GL_LIGHTING);
	if (light_enabled)
		glDisable (GL_LIGHTING);

	float line_width;
	glGetFloatv (GL_LINE_WIDTH, &line_width);

	glLineWidth (2.f);

	// for the rotation of the axes
	Matrix44f axes_rotation_matrix (Matrix44f::Identity());
	axes_rotation_matrix.block<3,3> (0,0) = configuration.axes_rotation;

	glPushMatrix();

	Matrix44f transform_matrix = axes_rotation_matrix * framemap["BASE"]->pose_transform;
	glMultMatrixf (transform_matrix.data());

	glBegin (GL_LINES);
	glColor3f (1.f, 0.f, 0.f);
	glVertex3f (0.f, 0.f, 0.f);
	glVertex3f (1.f, 0.f, 0.f);
	glColor3f (0.f, 1.f, 0.f);
	glVertex3f (0.f, 0.f, 0.f);
	glVertex3f (0.f, 1.f, 0.f);
	glColor3f (0.f, 0.f, 1.f);
	glVertex3f (0.f, 0.f, 0.f);
	glVertex3f (0.f, 0.f, 1.f);
	glEnd();

	glPopMatrix();

	if (depth_test_enabled)
		glEnable (GL_DEPTH_TEST);

	if (light_enabled)
		glEnable (GL_LIGHTING);

	glLineWidth (line_width);
}

Json::Value vec3_to_json (const Vector3f &vec) {
	Json::Value result;
	result[0] = Json::Value(static_cast<float>(vec[0]));
	result[1] = Json::Value(static_cast<float>(vec[1]));
	result[2] = Json::Value(static_cast<float>(vec[2]));

	return result;
}

Vector3f json_to_vec3 (const Json::Value &value, Vector3f defaultvalue=Vector3f (0.f, 0.f, 0.f)) {
	if (value.isNull())
		return defaultvalue;

	Vector3f result (
			value[0].asFloat(),
			value[1].asFloat(),
			value[2].asFloat()
			);

	return result;
}

Json::Value frame_configuration_to_json_value (const FrameConfig &config) {
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

void MeshupModel::saveModelToFile (const char* filename) {
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

bool MeshupModel::loadModelFromFile (const char* filename, bool strict) {
	// we absoulutely have to set the locale to english for numbers.
	// Otherwise we might read false values due to the wrong conversion.
	std::setlocale(LC_NUMERIC, "POSIX");

	using namespace Json;
	Value root;
	Reader reader;

	ifstream file_in (filename);

	if (!file_in) {
		cerr << "Error opening file " << filename << "!" << endl;
		
		if (strict)
			exit(1);
		return false;
	}

	cout << "Loading model " << filename << endl;

	stringstream buffer;
	buffer << file_in.rdbuf();
	file_in.close();

	bool parsing_result = reader.parse (buffer.str(), root);
	if (!parsing_result) {
		cerr << "Error reading model: " << reader.getFormattedErrorMessages();

		if (strict)
			exit (1);
		return false;
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

	configuration.init();

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
				json_to_vec3 (segment_node["scale"]),
				json_to_vec3 (segment_node["color"]),
				segment_node["mesh_filename"].asString(),
				json_to_vec3 (segment_node["translate"]),
				json_to_vec3 (segment_node["mesh_center"], Vector3f(1/0.0,1/0.0,1/0.0))
				);

		node_iter++;
	}

	initFrameTransform();

	model_filename = filename;

	return true;
}

struct ColumnInfo {
	ColumnInfo() :
		frame (FramePtr()),
		type (TypeUnknown),
		axis (AxisUnknown),
		is_time_column (false),
		is_empty (false),
		is_radian (false)
	{}
	enum TransformType {
		TypeUnknown = 0,
		TypeRotation,
		TypeTranslation,
		TypeScale,
	};
	enum AxisName {
		AxisUnknown = 0,
		AxisX,
		AxisY,
		AxisZ,
		AxisMX,
		AxisMY,
		AxisMZ
	};
	FramePtr frame;
	TransformType type;
	AxisName axis;

	bool is_time_column;
	bool is_empty;
	bool is_radian;
};

struct AnimationKeyPoses {
	float timestamp;
	std::vector<ColumnInfo> columns;
	typedef std::map<FramePtr, FramePose> FramePoseMap;
	FramePoseMap frame_poses;
	
	void clearFramePoses() {
		frame_poses.clear();
	}
	bool setValue (int column_index, float value, bool strict = true) {
		assert (column_index <= columns.size());
		ColumnInfo col_info = columns[column_index];

		if (col_info.is_time_column) {
			timestamp = value;
			return true;
		}
		if (col_info.is_empty) {
			return true;
		}

		FramePtr frame = col_info.frame;

		if (frame_poses.find(frame) == frame_poses.end()) {
			// create new frame and insert it
			frame_poses[frame] = FramePose();
		}

		if (col_info.type == ColumnInfo::TypeRotation) {
			if (col_info.axis == ColumnInfo::AxisX) {
				frame_poses[frame].rotation[0] = value;
			}
			if (col_info.axis == ColumnInfo::AxisY) {
				frame_poses[frame].rotation[1] = value;
			}
			if (col_info.axis == ColumnInfo::AxisZ) {
				frame_poses[frame].rotation[2] = value;
			}
			if (col_info.axis == ColumnInfo::AxisMX) {
				frame_poses[frame].rotation[0] = -value;
			}
			if (col_info.axis == ColumnInfo::AxisMY) {
				frame_poses[frame].rotation[1] = -value;
			}
			if (col_info.axis == ColumnInfo::AxisMZ) {
				frame_poses[frame].rotation[2] = -value;
			}
		} else if (col_info.type == ColumnInfo::TypeTranslation) {
			if (col_info.axis == ColumnInfo::AxisX) {
				frame_poses[frame].translation[0] = value;
			}
			if (col_info.axis == ColumnInfo::AxisY) {
				frame_poses[frame].translation[1] = value;
			}
			if (col_info.axis == ColumnInfo::AxisZ) {
				frame_poses[frame].translation[2] = value;
			}	
			if (col_info.axis == ColumnInfo::AxisMX) {
				frame_poses[frame].translation[0] = -value;
			}
			if (col_info.axis == ColumnInfo::AxisMY) {
				frame_poses[frame].translation[1] = -value;
			}
			if (col_info.axis == ColumnInfo::AxisMZ) {
				frame_poses[frame].translation[2] = -value;
			}	
		} else if (col_info.type == ColumnInfo::TypeScale) {
			if (col_info.axis == ColumnInfo::AxisX) {
				frame_poses[frame].scaling[0] = value;
			}
			if (col_info.axis == ColumnInfo::AxisY) {
				frame_poses[frame].scaling[1] = value;
			}
			if (col_info.axis == ColumnInfo::AxisZ) {
				frame_poses[frame].scaling[2] = value;
			}	
			if (col_info.axis == ColumnInfo::AxisMX) {
				frame_poses[frame].scaling[0] = -value;
			}
			if (col_info.axis == ColumnInfo::AxisMY) {
				frame_poses[frame].scaling[1] = -value;
			}
			if (col_info.axis == ColumnInfo::AxisMZ) {
				frame_poses[frame].scaling[2] = -value;
			}	
		} else {
			cerr << "Error: invalid column info type: " << col_info.type << ". Something really weird happened!" << endl;

			if (strict)
				exit (1);

			return false;
		}

		return true;
	}
	void updateTimeValues () {
		for (FramePoseMap::iterator pose_iter = frame_poses.begin(); pose_iter != frame_poses.end(); pose_iter++) {
			pose_iter->second.timestamp = timestamp;
		}
	}
};

bool MeshupModel::loadAnimationFromFile (const char* filename, bool strict) {
	ifstream file_in (filename);

	if (!file_in) {
		cerr << "Error opening animation file " << filename << "!";

		if (strict)
			exit (1);

		return false;
	}

	cout << "Loading animation " << filename << endl;

	string line;

	bool column_section = false;
	bool data_section = false;
	int column_index = 0;
	int line_number = 0;
	AnimationKeyPoses animation_keyposes;

	while (!file_in.eof()) {
		getline (file_in, line);
		line_number++;
	
		line = strip_comments (strip_whitespaces( (line)));
		
		// skip lines with no information
		if (line.size() == 0)
			continue;

		if (line.substr (0, string("COLUMNS:").size()) == "COLUMNS:") {
			column_section = true;

			// we set it to -1 and can then easily increasing the value
			column_index = -1;
			continue;
		}

		if (line.substr (0, string("DATA:").size()) == "DATA:") {
			column_section = false;
			data_section = true;
			continue;
		}

		if (column_section) {
			// do columny stuff
			// cout << "COLUMN:" << line << endl;

			std::vector<string> elements = tokenize(line, ", \t\n\r");
			for (int ei = 0; ei < elements.size(); ei++) {
				// skip elements that had multiple spaces in them
				if (elements[ei].size() == 0)
					continue;

				// it's safe to increase the column index here, as we did
				// initialize it with -1
				column_index++;

				string column_def = strip_whitespaces(elements[ei]);
				// cout << "  E: " << column_def << endl;

				if (tolower(column_def) == "time") {
					ColumnInfo column_info;
					column_info.is_time_column = true;
					animation_keyposes.columns.push_back(column_info);
					// cout << "Setting time column to " << column_index << endl;
					continue;
				}
				if (tolower(column_def) == "empty") {
					ColumnInfo column_info;
					column_info.is_empty = true;
					animation_keyposes.columns.push_back(column_info);
					continue;
				}
				
				std::vector<string> spec = tokenize(column_def, ":");
				if (spec.size() < 3 || spec.size() > 4) {
					cerr << "Error: parsing column definition '" << column_def << "' in " << filename << " line " << line_number << endl;

					if (strict)
						exit(1);

					return false;
				}

				// find the frame
				FramePtr frame = findFrame (strip_whitespaces(spec[0]).c_str());
				if (frame == NULL) {
					cerr << "Error: Unknown frame '" << spec[0] << "' in " << filename << " line " << line_number << endl;

					if (strict)
						exit (1);

					return false;
				}

				// the transform type
				string type_str = tolower(strip_whitespaces(spec[1]));
				ColumnInfo::TransformType type = ColumnInfo::TypeUnknown;
				if (type_str == "rotation"
						|| type_str == "r")
					type = ColumnInfo::TypeRotation;
				else if (type_str == "translation"
						|| type_str == "t")
					type = ColumnInfo::TypeTranslation;
				else if (type_str == "scale"
						|| type_str == "s")
					type = ColumnInfo::TypeScale;
				else {
					cerr << "Error: Unknown transform type '" << spec[1] << "' in " << filename << " line " << line_number << endl;
					
					if (strict)
						exit (1);

					return false;
				}

				// and the axis
				string axis_str = tolower(strip_whitespaces(spec[2]));
				ColumnInfo::AxisName axis_name;
				if (axis_str == "x")
					axis_name = ColumnInfo::AxisX;
				else if (axis_str == "y")
					axis_name = ColumnInfo::AxisY;
				else if (axis_str == "z")
					axis_name = ColumnInfo::AxisZ;
				else if (axis_str == "-x")
					axis_name = ColumnInfo::AxisMX;
				else if (axis_str == "-y")
					axis_name = ColumnInfo::AxisMY;
				else if (axis_str == "-z")
					axis_name = ColumnInfo::AxisMZ;
				else {
					cerr << "Error: Unknown axis name '" << spec[2] << "' in " << filename << " line " << line_number << endl;

					if (strict)
						exit (1);

					return false;
				}

				bool unit_is_radian = false;
				if (spec.size() == 4) {
					string unit_str = tolower(strip_whitespaces(spec[3]));
					if (unit_str == "r" || unit_str == "rad" || unit_str == "radians")
						unit_is_radian = true;
				}

				ColumnInfo col_info;
				col_info.frame = frame;
				col_info.type = type;
				col_info.axis = axis_name;
				col_info.is_radian = unit_is_radian;

				// cout << "Adding column " << column_index << " " << frame->name << ", " << type << ", " << axis_name << " radians = " << col_info.is_radian << endl;
				animation_keyposes.columns.push_back(col_info);
			}

			continue;
		}

		if (data_section) {
			// cout << "DATA  :" << line << endl;
			// parse the DOF description and set the column info in
			// animation_keyposes

			// Data part:
			// columns have been read
			std::vector<string> columns = tokenize (line);
			assert (columns.size() >= animation_keyposes.columns.size());

			// we update all the frame_poses. Once we're done, we add all poses
			// to the given time and clear all frame poses again.
			animation_keyposes.clearFramePoses();

			for (int ci = 0; ci < animation_keyposes.columns.size(); ci++) {
				// parse each column value and submit it to animation_keyposes
				float value;
				istringstream value_stream (columns[ci]);
				value_stream >> value;
				
				// handle radian
				if (animation_keyposes.columns[ci].type==ColumnInfo::TypeRotation && animation_keyposes.columns[ci].is_radian) {
					value *= 180. / M_PI;
				}
				
				// cout << "  col value " << ci << " = " << value << endl;
				animation_keyposes.setValue (ci, value, strict);
			}

			// dispatch the time information to all frame poses
			animation_keyposes.updateTimeValues();

			AnimationKeyPoses::FramePoseMap::iterator frame_pose_iter = animation_keyposes.frame_poses.begin();
			while (frame_pose_iter != animation_keyposes.frame_poses.end()) {
				// call addFramePose()
				FramePtr frame = frame_pose_iter->first;
				FramePose pose = frame_pose_iter->second;

				// cout << "addFramePose("
				// 	<< "  " << frame->name << endl
				// 	<< "  " << pose.timestamp << endl
				// 	<< "  " << pose.translation.transpose() << endl
				// 	<< "  " << pose.rotation.transpose() << endl
				// 	<< "  " << pose.scaling.transpose() << endl;

				addFramePose (frame->name.c_str(),
						pose.timestamp,
						pose.translation,
						pose.rotation,
						pose.scaling
						);

				frame_pose_iter++;
			}
			continue;
		}
	}

	animation_filename = filename;

	return true;
}
