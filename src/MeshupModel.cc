/*
 * MeshUp - A visualization tool for multi-body systems based on skeletal
 * animation and magic.
 *
 * Copyright (c) 2012 Martin Felis <martin.felis@iwr.uni-heidelberg.de>
 *
 * Licensed under the MIT license. See LICENSE for more details.
 */

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

extern "C"
{
   #include <lua.h>
   #include <lauxlib.h>
   #include <lualib.h>
}
#include "luatables.h"

#include "objloader.h"
#include "Curve.h"
#include "Animation.h"

using namespace std;

void bail(lua_State *L, const char *msg){
	std::cerr << msg << lua_tostring(L, -1) << endl;
	abort();
}

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

		string model_filename_json = model_filename + ".json";
		string model_filename_lua = model_filename + ".lua";

		if (boost::filesystem::is_regular_file(model_filename_lua)) {
			return model_filename_lua;
			break;
		} else if (boost::filesystem::is_regular_file(model_filename_json)) {
			return model_filename_json;
			break;
		} 
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

/**
 *
 * \todo get rid of initDefaultFrametransform
 */
void Frame::initDefaultFrameTransform(const Matrix44f &parent_frame_transform, const FrameConfig &config) {
	// first translate, then rotate as specified in the angles
	frame_transform =	parent_transform;

	for (unsigned int ci = 0; ci < children.size(); ci++) {
		children[ci]->initDefaultFrameTransform (frame_transform, config);
	}
}

/*********************************
 * MeshupModel
 *********************************/
void MeshupModel::addFrame (
		const std::string &parent_frame_name,
		const std::string &frame_name,
		const Matrix44f &parent_transform) {
	// cout << "addFrame(" << endl
	//	<< "  parent_frame_name = " << parent_frame_name << endl
	//	<< "  frame_name = " << frame_name << endl
	//	<< "  parent_transform = " << endl << parent_transform << endl;

	// mark frame transformations as dirty
	frames_initialized = false;

	string frame_name_sanitized = sanitize_name (frame_name);
	string parent_frame_name_sanitized = sanitize_name (parent_frame_name);

	// create the frame
	FramePtr frame (new Frame);
	frame->name = frame_name_sanitized;
	frame->parent_transform = parent_transform;
	frame->frame_transform = parent_transform;

	// first find the frame
	FramePtr parent_frame = findFrame (parent_frame_name_sanitized.c_str());
	if (parent_frame == NULL) {
		cerr << "Could not find frame '" << parent_frame_name_sanitized << "'!" << endl;
		exit (1);
	}

	parent_frame->children.push_back (frame);
	framemap[frame->name] = frame;
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

	// cout << "addSegment( " << frame_name << "," << endl
	// 	<< "  " << segment_name << "," << endl
	// 	<< "  " << dimensions.transpose() << "," << endl
	// 	<< "  " << scale.transpose() << "," << endl
	// 	<< "  " << color.transpose() << "," << endl
	// 	<< "  " << mesh_name << "," << endl
	// 	<< "  " << translate.transpose() << "," << endl
	// 	<< "  " << mesh_center.transpose() << ")" << endl << endl;

	string sanitized_segment_name = sanitize_name(segment_name);

	segment.name = sanitized_segment_name;
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

		if (!skip_vbo_generation)
			new_mesh->generate_vbo();

		meshmap[mesh_name] = new_mesh;

		mesh_iter = meshmap.find (mesh_name);
	}

	segment.mesh = mesh_iter->second;
	segment.meshcenter = configuration.axes_rotation.transpose() * mesh_center;
	segment.frame = findFrame (sanitize_name(frame_name).c_str());
	segment.mesh_filename = mesh_name;
	assert (segment.frame != NULL);
	segments.push_back (segment);
}

void MeshupModel::addCurvePoint (
		const std::string &curve_name,
		const Vector3f &coords,
		const Vector3f &color
		) {
	CurveMap::iterator curve_iter = curvemap.find(curve_name);
	if (curve_iter == curvemap.end()) {
		curvemap[curve_name] = CurvePtr (new Curve);
	}

	CurvePtr curve = curvemap[curve_name];
	curve->addPointWithColor (
			coords[0], coords[1], coords[2],
			color[0], color[1], color[2]
			);
}

void MeshupModel::updateFrames() {
	Matrix44f base_transform (Matrix44f::Identity());

	// check whether the frame transformations are valid
	if (frames_initialized == false)
		initDefaultFrameTransform();

	for (unsigned int bi = 0; bi < frames.size(); bi++) {
		frames[bi]->updatePoseTransform (base_transform, configuration);
	}
}

void MeshupModel::initDefaultFrameTransform() {
	Matrix44f base_transform (Matrix44f::Identity());

	for (unsigned int bi = 0; bi < frames.size(); bi++) {
		frames[bi]->initDefaultFrameTransform (base_transform, configuration);
	}

	frames_initialized = true;
}

void MeshupModel::draw() {
	// save current state of GL_NORMALIZE to properly restore the original
	// state
	bool normalize_enabled = glIsEnabled (GL_NORMALIZE);
	if (!normalize_enabled)
		glEnable (GL_NORMALIZE);

	SegmentList::iterator seg_iter = segments.begin();

	while (seg_iter != segments.end()) {
		glPushMatrix();
		
		glMultMatrixf (seg_iter->gl_matrix.data());

		// drawing
		glColor3f (seg_iter->color[0], seg_iter->color[1], seg_iter->color[2]);
		
		seg_iter->mesh->draw(GL_TRIANGLES);

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
		if (frame_iter->second->name == "ROOT") {
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

	Matrix44f transform_matrix = axes_rotation_matrix * framemap["ROOT"]->pose_transform;
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

void MeshupModel::drawCurves() {
	CurveMap::iterator curve_iter = curvemap.begin();
	while (curve_iter != curvemap.end()) {
		curve_iter->second->draw();
		curve_iter++;
	}
}

Json::Value vec3_to_json (const Vector3f &vec) {
	Json::Value result;
	result[0] = Json::Value(static_cast<float>(vec[0]));
	result[1] = Json::Value(static_cast<float>(vec[1]));
	result[2] = Json::Value(static_cast<float>(vec[2]));

	return result;
}

Vector3f json_to_vec3 (const Json::Value &value, Vector3f default_value=Vector3f (0.f, 0.f, 0.f)) {
	if (value.isNull())
		return default_value;

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

Json::Value frame_to_json_value (const FramePtr &frame, FrameConfig frame_config) {
	using namespace Json;

	Value result;

	result["name"] = frame->name;

	result["parent_translation"] = vec3_to_json(frame_config.axes_rotation * frame->getFrameTransformTranslation());
	
	Matrix33f rotation = frame->getFrameTransformRotation();
	if (Matrix33f::Identity() != rotation) {
		cerr << "Error: cannot convert non-zero parent_rotation to Json value." << endl;
		abort();
	}

	return result;
}

Json::Value segment_to_json_value (const Segment &segment, FrameConfig frame_config) {
	using namespace Json;

	Value result;

	result["name"] = segment.name;

	if (Vector3f::Zero() != segment.dimensions)
		result["dimensions"] = vec3_to_json (frame_config.axes_rotation * segment.dimensions);

	if (Vector3f::Zero() != segment.color)
		result["color"] = vec3_to_json (segment.color);

	if (Vector3f::Zero() != segment.scale)
		result["scale"] = vec3_to_json (segment.scale);

	if (!isnan(segment.meshcenter[0])) {
		result["mesh_center"] = vec3_to_json (frame_config.axes_rotation * segment.meshcenter);
	} else {
		result["translate"] = vec3_to_json (frame_config.axes_rotation * segment.translate);
	}

	result["mesh_filename"] = segment.mesh_filename;
	result["frame"] = segment.frame->name;

	return result;
}

void MeshupModel::saveModelToJsonFile (const char* filename) {
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

		if (frame_stack.top()->name != "ROOT") {
			root_node["frames"][frame_index] = frame_to_json_value(frame_stack.top(), configuration);
			frame_index++;
		}

		while (frame_stack.size() > 0) {
			FramePtr cur_frame = frame_stack.top();
			int child_idx = child_index_stack.top();

			if (child_idx < cur_frame->children.size()) {
				FramePtr child_frame = cur_frame->children[child_idx];

				root_node["frames"][frame_index] = frame_to_json_value(child_frame, configuration);
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
		root_node["segments"][segment_index] = segment_to_json_value (*seg_iter, configuration);

		segment_index++;
		seg_iter++;
	}

	ofstream file_out (filename, ios::trunc);
	file_out << root_node << endl;
	file_out.close();
}

string vec3_to_string_no_brackets (const Vector3f &vector) {
	ostringstream out;
	out << vector[0] << ", " << vector[1] << ", " << vector[2];

	return out.str();
}

string frame_to_lua_string (const FramePtr frame, const string &parent_name, vector<string> meshes, int indent = 0) {
	ostringstream out;
	string indent_str;

	for (int i = 0; i < indent; i++)
		indent_str += "  ";

	out << indent_str << "{" << endl
		<< indent_str << "  name = \"" << frame->name << "\"," << endl
		<< indent_str << "  parent = \"" << parent_name << "\"," << endl;

	Vector3f translation = frame->getFrameTransformTranslation();
	Matrix33f rotation = frame->getFrameTransformRotation();

	// only write joint_frame if we actually have a transformation
	if (Vector3f::Zero() != translation
			|| Matrix33f::Identity() != rotation) {
		out << indent_str << "  joint_frame = {" << endl;

		if (Vector3f::Zero() != translation)
			out << indent_str << "    r = { " << vec3_to_string_no_brackets (translation) << " }," << endl;

		if (Matrix33f::Identity() != rotation) {
			out << indent_str << "    E = {" << endl;
			for (unsigned int i = 0; i < 3; i++) {
			out << indent_str << "      { ";
				for (unsigned int j = 0; j < 2; j++) {
					out << setiosflags(ios_base::fixed) << rotation(i,j) << ", ";
				}
				out << setiosflags(ios_base::fixed) << rotation(i,2) << " }," << endl;
			}
			out << indent_str << "    }," << endl;
		}
		out << indent_str << "  }," << endl;
	}

	// output of the meshes
	if (meshes.size() > 0) {
		out << indent_str << "  visuals = {" << endl;

		for (unsigned int i = 0; i < meshes.size(); i++) {
			out << indent_str << "    " << meshes[i] << "," << endl;
		}

		out << indent_str << "  }," << endl;
	}

	out << indent_str << "}";

	return out.str();
}

string segment_to_lua_string (const Segment &segment, FrameConfig frame_config, int indent = 0) {
	ostringstream out;
	string indent_str;

	for (int i = 0; i < indent; i++)
		indent_str += "  ";

	out << indent_str << segment.name << " = {" << endl
		<< indent_str << "  name = \"" << segment.name << "\"," << endl;
	if (Vector3f::Zero() != segment.dimensions)
		out << indent_str << "  dimensions = { " 
			<< vec3_to_string_no_brackets(frame_config.axes_rotation * segment.dimensions) 
			<< "}," << endl;

	if (Vector3f(0.f, 0.f, 0.f) != segment.scale)
		out	<< indent_str << "  scale = { " << vec3_to_string_no_brackets(segment.scale) << "}," << endl;

	if (Vector3f::Zero() != segment.color)
		out	<< indent_str << "  color = { " << vec3_to_string_no_brackets(segment.color) << "}," << endl;

	if (Vector3f::Zero() != segment.meshcenter && !isnan(segment.meshcenter[0]))
		out	<< indent_str << "  mesh_center = { " 
			<< vec3_to_string_no_brackets(frame_config.axes_rotation * segment.meshcenter)
			<< "}," << endl;

	if (Vector3f::Zero() != segment.translate)
		out	<< indent_str << "  translate = { " << vec3_to_string_no_brackets(segment.translate) << "}," << endl;

	out	<< indent_str << "  src = \"" << segment.mesh_filename << "\"," << endl;
	out << indent_str << "}," << endl;

	return out.str();
}

void MeshupModel::saveModelToLuaFile (const char* filename) {
	ofstream file_out (filename, ios::trunc);

	map<string, vector<string> > frame_segment_map;

	// write all segments
	file_out << "meshes = {" << endl;
	SegmentList::iterator seg_iter = segments.begin();
	while (seg_iter != segments.end()) {
		file_out << segment_to_lua_string (*seg_iter, configuration, 1);

		frame_segment_map[seg_iter->frame->name].push_back(string("meshes.") + seg_iter->name);

		seg_iter++;
	}
	file_out << "}" << endl << endl;

	// write configuration
	file_out << "model = {" << endl
		<< "  configuration = {" << endl
		<< "    axis_front = { " << vec3_to_string_no_brackets(configuration.axis_front) << " }," << endl
		<< "    axis_up    = { " << vec3_to_string_no_brackets(configuration.axis_up) << " }," << endl
		<< "    axis_right = { " << vec3_to_string_no_brackets(configuration.axis_right) << " }," << endl
		<< "    rotation_order = { " << configuration.rotation_order[0] << ", "
			<< configuration.rotation_order[1] << ", "
			<< configuration.rotation_order[2] << "}," << endl
		<< "  }," << endl << endl;

	// write frames
	file_out << "  frames = {" << endl;
	int frame_index = 0;
	// we have to write out the frames recursively
	for (int bi = 0; bi < frames.size(); bi++) {
		stack<FramePtr> frame_stack;
		frame_stack.push (frames[bi]);

		stack<int> child_index_stack;
		if (frame_stack.top()->children.size() > 0) {
			child_index_stack.push(0);
		}

		if (frame_stack.top()->name != "ROOT") {
			file_out << frame_to_lua_string(frame_stack.top(), "ROOT", frame_segment_map["ROOT"], 2) << "," << endl;
			frame_index++;
		}

		while (frame_stack.size() > 0) {
			FramePtr cur_frame = frame_stack.top();
			int child_idx = child_index_stack.top();

			if (child_idx < cur_frame->children.size()) {
				FramePtr child_frame = cur_frame->children[child_idx];

				file_out << frame_to_lua_string(child_frame, cur_frame->name, frame_segment_map[child_frame->name], 2) << "," << endl;
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
		}
	}
	file_out << "  }" << endl;
	file_out << "}" << endl << endl;
	file_out << "return model" << endl;

	file_out.close();
}

bool MeshupModel::loadModelFromFile (const char* filename, bool strict) {
	string filename_str (filename);

	if (filename_str.size() < 5) {
		cerr << "Error: Filename " << filename << " too short. Must be at least 5 characters." << endl;

		if (strict)
			abort();

		return false;
	}

	cout << "Load model " << filename << endl;

	if (tolower(filename_str.substr(filename_str.size() - 4, 4)) == ".lua")
		return loadModelFromLuaFile (filename, strict);
	else if (tolower(filename_str.substr(filename_str.size() - 5, 5)) == ".json")
		return loadModelFromJsonFile (filename, strict);

	cerr << "Error: Could not determine filetype for model " << filename << ". Must be either .lua or .json file." << endl;

	if (strict)
		abort();

	return false;
}

void MeshupModel::saveModelToFile (const char* filename) {
	string filename_str (filename);

	if (filename_str.size() < 5) {
		cerr << "Error: Filename " << filename << " too short. Must be at least 5 characters." << endl;
		abort();
	}

	if (tolower(filename_str.substr(filename_str.size() - 4, 4)) == ".lua")
		saveModelToLuaFile (filename);
	else if (tolower(filename_str.substr(filename_str.size() - 5, 5)) == ".json")
		saveModelToJsonFile (filename);

	else {
		cerr << "Error: Could not determine filetype for model " << filename << ". Must be either .lua or .json file." << endl;
		abort();
	}
}

bool MeshupModel::loadModelFromJsonFile (const char* filename, bool strict) {
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
			abort();

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
			abort ();

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

		Vector3f parent_translation = configuration.axes_rotation.transpose() * json_to_vec3(frame_node["parent_translation"]);
		Vector3f parent_rotation = json_to_vec3(frame_node["parent_rotation"]);

		Matrix44f parent_transform = configuration.convertAnglesToMatrix (parent_rotation) 
			* smTranslate (parent_translation[0], parent_translation[1], parent_translation[2]);

		if (frame_node["parent"].asString() == "BASE") {
			cerr << "Warning: global frame should be 'ROOT' instead of 'BASE'!" << endl;
			frame_node["parent"] = "ROOT";
		}

		addFrame (
				frame_node["parent"].asString(),
				frame_node["name"].asString(),
				parent_transform);

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

	initDefaultFrameTransform();

	model_filename = filename;

	return true;
}

Vector3f lua_get_vector3f (lua_State *L, const string &path, int index = -1) {
	Vector3f result;

	std::vector<double> array = get_array (L, path, index);
	if (array.size() != 3) {
		cerr << "Invalid array size for 3d vector variable '" << path << "'." << endl;
		abort();
	}

	for (unsigned int i = 0; i < 3; i++) {
		result[i] = static_cast<float>(array[i]);
	}

	return result;
}

Matrix33f lua_get_matrix3f (lua_State *L, const string &path) {
	Matrix33f result;

	// two ways either as flat array or as a lua table with three columns
	if (get_length (L, path, -1) == 3) {
		Vector3f row = lua_get_vector3f (L, path, 1);
		result(0,0) = row[0];
		result(0,1) = row[1];
		result(0,2) = row[2];

		row = lua_get_vector3f (L, path, 2);
		result(1,0) = row[0];
		result(1,1) = row[1];
		result(1,2) = row[2];

		row = lua_get_vector3f (L, path, 3);
		result(1,0) = row[0];
		result(1,1) = row[1];
		result(1,2) = row[2];

		return result;
	}

	std::vector<double> array = get_array (L, path, -1);
	if (array.size() != 9) {
		cerr << "Invalid array size for 3d matrix variable '" << path << "'." << endl;
		abort();
	}

	for (unsigned int i = 0; i < 9; i++) {
		result.data()[i] = static_cast<float>(array[i]);
	}

	return result;
}

bool lua_read_frame (
		lua_State *L,
		const string &frame_path,
		string &frame_name,
		string &parent_name,
		Vector3f &parent_translation,
		Matrix33f &parent_rotation ) {
	string path;

	if (!value_exists (L, frame_path + ".name")) {
		cerr << "Error: required value .name does not exist for frame '" << frame_path << "'!" << endl;
		return false;
	}
	frame_name = get_string (L, frame_path + ".name");

	if (!value_exists (L, frame_path + ".parent")) {
		cerr << "Error: required value .parent does not exist for frame '" << frame_name << "'!" << endl;
		return false;
	}
	parent_name = get_string (L, frame_path + ".parent");

	parent_translation = Vector3f::Zero();
	parent_rotation = Matrix33f::Identity();
	if (value_exists (L, frame_path + ".joint_frame")) {
		if (value_exists (L, frame_path + ".joint_frame.r")) {
			parent_translation = lua_get_vector3f (L, frame_path + ".joint_frame.r");
		}

		if (value_exists (L, frame_path + ".joint_frame.E")) {
			parent_rotation = lua_get_matrix3f (L, frame_path + ".joint_frame.E");
		}
	}

	return true;
}

bool lua_read_visual_info (
		lua_State *L,
		const string &visual_path,	
		std::string &segment_name,
		Vector3f &dimensions,
		Vector3f &scale,
		Vector3f &color,
		std::string &mesh_filename,
		Vector3f &translate,
		Vector3f &mesh_center) {

	if (value_exists (L, visual_path + ".name")) 
		segment_name = get_string (L, visual_path + ".name");

	if (value_exists (L, visual_path + ".dimensions"))
		dimensions = lua_get_vector3f (L, visual_path + ".dimensions");

	if (value_exists (L, visual_path + ".scale"))
		scale = lua_get_vector3f (L, visual_path + ".scale");

	if (value_exists (L, visual_path + ".color"))
		color = lua_get_vector3f (L, visual_path + ".color");

	if (value_exists (L, visual_path + ".translate"))
		translate = lua_get_vector3f (L, visual_path + ".translate");

	if (value_exists (L, visual_path + ".mesh_center"))
		mesh_center = lua_get_vector3f (L, visual_path + ".mesh_center");

	if (value_exists (L, visual_path + ".src"))
		mesh_filename = get_string (L, visual_path + ".src");

	return true;
}

bool MeshupModel::loadModelFromLuaFile (const char* filename, bool strict) {
	lua_State *L;
	L = luaL_newstate();
	luaL_openlibs(L);

	if (luaL_loadfile(L, filename) || lua_pcall (L, 0, 1, 0)) {
		cerr <<  "Error running file: ";
		std::cerr << lua_tostring(L, -1) << endl;
		if (strict)
			abort();

		return false;
	}

	clear();
	
	// configuration
	if (value_exists (L, "configuration.axis_front")) {
		configuration.axis_front = lua_get_vector3f (L, "configuration.axis_front");	
	}
	if (value_exists (L, "configuration.axis_up")) {
		configuration.axis_up = lua_get_vector3f (L, "configuration.axis_up");	
	}
	if (value_exists (L, "configuration.axis_right")) {
		configuration.axis_right = lua_get_vector3f (L, "configuration.axis_right");	
	}
	if (value_exists (L, "configuration.rotation_order")) {
		Vector3f rotation_order = lua_get_vector3f (L, "configuration.rotation_order");
		configuration.rotation_order[0] = static_cast<int>(rotation_order[0]);
		configuration.rotation_order[1] = static_cast<int>(rotation_order[1]);
		configuration.rotation_order[2] = static_cast<int>(rotation_order[2]);
	}

	configuration.init();
	// cout << "configuration.axes_rotation = " << endl << configuration.axes_rotation << endl;

	// frames
	vector<string> frame_keys = get_keys (L, "frames");

	for (unsigned int i = 0; i < frame_keys.size(); i++) {
		string parent_frame;
		string frame_name;
		Vector3f parent_translation;
		Matrix33f parent_rotation;

		ostringstream frame_path;
		frame_path << "frames." << frame_keys[i];

		if (!lua_read_frame (
					L,
					frame_path.str(),
					frame_name,
					parent_frame,
					parent_translation,
					parent_rotation)) {
			cerr << "Error reading frame " << frame_keys[i] << "." << endl;
			if (strict)
				abort();

			return false;
		}

		Matrix44f parent_transform = Matrix44f::Identity(); 
		parent_transform.block<3,3>(0,0) = parent_rotation.transpose();
		parent_transform.block<1,3>(3,0) = (configuration.axes_rotation.transpose() * parent_translation).transpose();
		addFrame (parent_frame, frame_name, parent_transform);

		string visuals_path = frame_path.str() + ".visuals";
		if (!value_exists (L, visuals_path)) {
			continue;
		} else {
			vector<string> visuals_keys = get_keys (L, visuals_path);

			for (unsigned int j = 0; j < visuals_keys.size(); j++) {
				string visual_path = visuals_path + string (".") + string(visuals_keys[j]);

				string segment_name;
				Vector3f dimensions (0.f, 0.f, 0.f);
				Vector3f scale (0.f, 0.f, 0.f);
				Vector3f color (1.f, 1.f, 1.f);
				string mesh_filename;
				Vector3f translate (0.f, 0.f, 0.f);
				Vector3f mesh_center (1.f/0.f, 1.f/0.f, 1.f/0.f);

				if (!lua_read_visual_info (
							L,
							visual_path,
							segment_name,
							dimensions,
							scale,
							color,
							mesh_filename,
							translate,
							mesh_center)) {
					cerr << "Error reading mesh information " << visual_path << "." << endl;

					if (strict)
						abort();

					return false;
				}

				addSegment (frame_name, segment_name, dimensions,
						scale, color, mesh_filename, translate, mesh_center);
			}
		}
	}

	initDefaultFrameTransform();

	model_filename = filename;

	return true;
}
