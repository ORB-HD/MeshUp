/*
 * MeshUp - A visualization tool for multi-body systems based on skeletal
 * animation and magic.
 *
 * Copyright (c) 2012-2018 Martin Felis <martin.felis@iwr.uni-heidelberg.de>
 *
 * Licensed under the MIT license. See LICENSE for more details.
 */

#include "GL/glew.h"

#include "Model.h"

#include "SimpleMath/SimpleMathGL.h"
#include "string_utils.h"
#include "meshup_config.h"

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

#include "Curve.h"
#include "Animation.h"

using namespace std;
using namespace SimpleMath::GL;

std::string find_model_file_by_name (const std::string &model_name) {
	std::string result;

	std::vector<std::string> paths;
	paths.push_back("");
	paths.push_back("./models/");

	if (getenv ("MESHUP_PATH")) {
		std::string env_meshup_dir (getenv("MESHUP_PATH"));

		if (env_meshup_dir.size() != 0) {
			if (env_meshup_dir[env_meshup_dir.size() - 1] != '/')
				env_meshup_dir += '/';

			paths.push_back (env_meshup_dir);
			paths.push_back (env_meshup_dir + "models/");
			paths.push_back (env_meshup_dir + "share/meshup/models/");
		}
	}

	paths.push_back(string(MESHUP_INSTALL_PREFIX) + "/share/meshup/models/");
	paths.push_back("/usr/local/share/meshup/models/");
	paths.push_back("/usr/share/meshup/models/");
	paths.push_back(std::string(MESHUP_INSTALL_PREFIX) + "/meshup/");

	std::vector<std::string>::iterator iter = paths.begin();
	string model_filename;
	for (iter; iter != paths.end(); iter++) {
		model_filename = *iter + model_name;

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
	paths.push_back("");

	if (getenv ("MESHUP_PATH")) {
		std::string env_meshup_dir (getenv("MESHUP_PATH"));

		if (env_meshup_dir.size() != 0) {
			if (env_meshup_dir[env_meshup_dir.size() - 1] != '/')
				env_meshup_dir += '/';

			paths.push_back (env_meshup_dir);
			paths.push_back (env_meshup_dir + "share/meshup/" ) ;
		}
	}

	paths.push_back(string(MESHUP_INSTALL_PREFIX) + "/share/meshup/");
	paths.push_back(string(MESHUP_INSTALL_PREFIX) + "/share/meshup/meshes/");
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
		cout << "  \"" << *iter << "\"" << endl;
	}
	abort();

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
		SimpleMath::GL::ScaleMat44 (pose_scaling[0], pose_scaling[1], pose_scaling[2])
	  * pose_rotation_quaternion.toGLMatrix()
		* SimpleMath::GL::TranslateMat44 (pose_translation[0], pose_translation[1], pose_translation[2])
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

void Frame::resetPoseTransform () {
	pose_translation = Vector3f::Zero();
	pose_rotation = Vector3f::Zero();
	pose_rotation_quaternion = SimpleMath::GL::Quaternion(0.f, 0.f, 0.f, 1.f);
	pose_scaling = Vector3f (1.f, 1.f, 1.f);
	pose_transform = Matrix44f::Identity();

	for (unsigned int i = 0; i < children.size(); i++) {
		children[i]->resetPoseTransform();
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
	// 	<< "  parent_frame_name = " << parent_frame_name << endl
	// 	<< "  frame_name = " << frame_name << endl
	// 	<< "  parent_transform = " << endl << parent_transform << endl;

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
		abort();
	}

	parent_frame->children.push_back (frame);
	framemap[frame->name] = frame;
}

void MeshupModel::addSegment (
		const std::string &frame_name,
        const MeshPtr mesh,
		const Vector3f &dimensions,
		const Vector3f &color,
		const Vector3f &translate,
		const Quaternion &rotate,
		const Vector3f &scale,
		const Vector3f &mesh_center) {
	Segment segment;

	// cout << "addSegment( " << frame_name << "," << endl
	// 	<< "  " << dimensions.transpose() << "," << endl
	// 	<< "  " << scale.transpose() << "," << endl
	// 	<< "  " << color.transpose() << "," << endl
	// 	<< "  " << mesh_name << "," << endl
	// 	<< "  " << translate.transpose() << "," << endl
	// 	<< "  " << mesh_center.transpose() << ")" << endl << endl;

	segment.dimensions = configuration.axes_rotation.transpose() * dimensions;

	//~ // make sure that the dimensions are all positive
	//~ for (int i = 0; i < 3; i++) {
	//~ if (segment.dimensions[i] < 0)
		//~ segment.dimensions[i] = -segment.dimensions[i];
	//~ }

	segment.color = color;
	segment.scale = scale;
	// cout << "configuration = " << endl << configuration.axes_rotation << endl;
	segment.translate = configuration.axes_rotation.transpose() * translate;

	float rotation_angle = 2. * acosf (rotate[3]);
	if (fabs (rotation_angle) > 0.001) {
		Vector3f rotation_axis = Vector3f (rotate[0], rotate[1], rotate[2]) / sinf (rotation_angle) * 2.;
		rotation_axis = (configuration.axes_rotation.transpose() * rotation_axis).normalize();
		segment.rotate = Quaternion::fromGLRotate (rotation_angle * 180. / M_PI, rotation_axis[0], rotation_axis[1], rotation_axis[2]);
	}

	segment.mesh = mesh;
	segment.meshcenter = configuration.axes_rotation.transpose() * mesh_center;
	segment.frame = findFrame ((frame_name).c_str());
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

void MeshupModel::addPoint (
		const std::string &name,
		const std::string &frame_name,
		const Vector3f &coords,
		const Vector3f &color,
		const bool draw_line,
		const float line_width
		) {
	Point point;
	point.name = name;
	point.frame = findFrame (frame_name.c_str());
	point.coordinates = configuration.axes_rotation.transpose() * coords;
	point.color = color;
	point.draw_line = draw_line;
	point.line_width = line_width;

	for (unsigned int i = 0; i < points.size(); i++) {
		if (points[i].name == name) {
			std::cerr << "Error: point '" << name << "' already defined in frame '" << points[i].frame->name << "'!" << endl;
			abort();
		}
	}

	points.push_back(point);
}

void MeshupModel::resetPoses() {
	for (unsigned int i = 0; i < frames.size(); i++) {
		frames[i]->resetPoseTransform();
	}
	frames_initialized = false;
	updateFrames();
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

void MeshupModel::updateSegments() {
	MeshupModel::SegmentList::iterator seg_iter = segments.begin();

	while (seg_iter != segments.end()) {
		Vector3f bbox_size (seg_iter->mesh->bbox_max - seg_iter->mesh->bbox_min);

		Vector3f scale(1.0f,1.0f,1.0f) ;

		//only scale, if the dimensions are valid, i.e. are set in json-File
		if (seg_iter->dimensions.squaredNorm() > 1.0e-4) {
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
			SimpleMath::GL::ScaleMat44 (scale[0], scale[1], scale[2])
			* seg_iter->rotate.toGLMatrix()
			* SimpleMath::GL::TranslateMat44 (translate[0], translate[1], translate[2])
			* seg_iter->frame->pose_transform;

		seg_iter++;
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

void MeshupModel::drawPoints() {
	// save current state of GL_NORMALIZE to properly restore the original
	// state
	bool normalize_enabled = glIsEnabled (GL_NORMALIZE);
	if (!normalize_enabled)
		glEnable (GL_NORMALIZE);

	MeshVBO sphere_mesh = CreateUVSphere (16, 16);

	for (unsigned int i = 0; i < points.size(); i++) {
		Vector3f frame_origin = points[i].frame->getPoseTransformTranslation();
		Vector3f point_location = points[i].frame->getPoseTransformTranslation() + points[i].frame->getPoseTransformRotation() * points[i].coordinates;

		glColor3fv (points[i].color.data());

		if (points[i].draw_line) {
			float line_range[2];
			glGetFloatv (GL_ALIASED_LINE_WIDTH_RANGE, line_range);
			if (points[i].line_width < line_range[0] || points[i].line_width > line_range[1]) {
				cerr << "Warning: Only line widths within range [" << line_range[0] << ", " << line_range[1] << "] are supported by the graphics driver! (Point '" << points[i].name << "' has line_width = " << points[i].line_width << endl;
			}
			glLineWidth (points[i].line_width);
			glBegin (GL_LINES);
			glVertex3fv (frame_origin.data());
			glVertex3fv (point_location.data());
			glEnd();
		}

		glPushMatrix();
		glTranslatef (point_location[0], point_location[1], point_location[2]);
		glScalef (0.025f, 0.025f, 0.025f);
	
		sphere_mesh.draw(GL_TRIANGLES);

		glPopMatrix();
	}

	// disable normalize if it was previously not enabled
	if (!normalize_enabled)
		glDisable (GL_NORMALIZE);
}

string vec3_to_string_no_brackets (const Vector3f &vector) {
	ostringstream out;
	out << vector[0] << ", " << vector[1] << ", " << vector[2];

	return out.str();
}

string frame_to_lua_string (FrameConfig configuration, const FramePtr frame, const string &parent_name, vector<string> meshes, int indent = 0) {
	ostringstream out;
	string indent_str;

	for (int i = 0; i < indent; i++)
		indent_str += "  ";

	out << indent_str << "{" << endl
		<< indent_str << "  name = \"" << frame->name << "\"," << endl
		<< indent_str << "  parent = \"" << parent_name << "\"," << endl;

	Vector3f translation = configuration.axes_rotation * frame->getFrameTransformTranslation();
	Matrix33f rotation = configuration.axes_rotation * frame->getFrameTransformRotation() * configuration.axes_rotation.transpose();

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
			file_out << frame_to_lua_string(configuration, frame_stack.top(), "ROOT", frame_segment_map["ROOT"], 2) << "," << endl;
			frame_index++;
		}

		while (frame_stack.size() > 0) {
			FramePtr cur_frame = frame_stack.top();
			int child_idx = child_index_stack.top();

			if (child_idx < cur_frame->children.size()) {
				FramePtr child_frame = cur_frame->children[child_idx];

				file_out << frame_to_lua_string(configuration, child_frame, cur_frame->name, frame_segment_map[child_frame->name], 2) << "," << endl;
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
	else 
		cerr << "Error: Could not determine filetype for model " << filename << ". Must be a .lua file." << endl;

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
	else {
		cerr << "Error: Could not determine filetype for model " << filename << ". Must be a .lua file." << endl;
		abort();
	}
}

template<> Vector3f LuaTableNode::getDefault<Vector3f>(const Vector3f &default_value) { 
	Vector3f result = default_value;

	if (stackQueryValue()) {
		LuaTable vector_table = LuaTable::fromLuaState (luaTable->L);

		if (vector_table.length() != 3) {
			std::cerr << "LuaModel Error at " << keyStackToString() << " : invalid 3d vector!" << std::endl;
			abort();
		}
		
		result[0] = vector_table[1];
		result[1] = vector_table[2];
		result[2] = vector_table[3];
	}

	stackRestore();

	return result;
}

template<> Matrix33f LuaTableNode::getDefault<Matrix33f>(const Matrix33f &default_value) {
	Matrix33f result = default_value;

	if (stackQueryValue()) {
		LuaTable vector_table = LuaTable::fromLuaState (luaTable->L);
		
		if (vector_table.length() != 3) {
			std::cerr << "LuaModel Error at " << keyStackToString() << " : invalid 3d matrix!" << std::endl;
			abort();
		}

		if (vector_table[1].length() != 3
				|| vector_table[2].length() != 3
				|| vector_table[3].length() != 3) {
			std::cerr << "LuaModel Error at " << keyStackToString() << " : invalid 3d matrix!" << std::endl;
			abort();
		}

		result(0,0) = vector_table[1][1];
		result(0,1) = vector_table[1][2];
		result(0,2) = vector_table[1][3];

		result(1,0) = vector_table[2][1];
		result(1,1) = vector_table[2][2];
		result(1,2) = vector_table[2][3];

		result(2,0) = vector_table[3][1];
		result(2,1) = vector_table[3][2];
		result(2,2) = vector_table[3][3];
	}

	stackRestore();

	return result;
}

bool MeshupModel::loadModelFromLuaFile (const char* filename, bool strict) {
	LuaTable model_table = LuaTable::fromFile (filename);

	clear();

	configuration.axis_front = model_table["configuration"]["axis_front"].getDefault(Vector3f (1.f, 0.f, 0.f));
	configuration.axis_up = model_table["configuration"]["axis_up"].getDefault(Vector3f (0.f, 1.f, 0.f));
	configuration.axis_right = model_table["configuration"]["axis_right"].getDefault(Vector3f (0.f, 0.f, 1.f));

	configuration.init();

	// initialize the model StateDescriptor. First entry must be the time
	// info.
	state_descriptor.clear();
	StateInfo time_info;
	time_info.is_time_column = true;
	state_descriptor.states.push_back (time_info);

	// frames
	int frame_count = model_table["frames"].length();

	// Read points
	int contact_point_count = model_table["points"].length();
	vector<Point> contact_points;
	for (int i = 1; i <= contact_point_count; i++) {
		contact_points.push_back(Point());
		contact_points[i-1].name = model_table["points"][i]["name"].get<std::string>();
		contact_points[i-1].parentBody = model_table["points"][i]["body"].get<std::string>().c_str();
		contact_points[i-1].coordinates = model_table["points"][i]["point"].get<Vector3f>();
		contact_points[i-1].color = model_table["points"][i]["color"].getDefault(Vector3f (1.0, 0.5, 0.5));
		contact_points[i-1].draw_line = model_table["points"][i]["draw_line"].getDefault(true);
		contact_points[i-1].line_width = model_table["points"][i]["line_width"].getDefault(1.f);
	}

	for (int i = 1; i <= frame_count; i++) {
		string parent_frame = model_table["frames"][i]["parent"].get<std::string>();
		string frame_name = model_table["frames"][i]["name"].get<std::string>();

		Vector3f parent_translation = model_table["frames"][i]["joint_frame"]["r"].getDefault<Vector3f>(Vector3f (0.f, 0.f, 0.f));
		Matrix33f parent_rotation = model_table["frames"][i]["joint_frame"]["E"].getDefault<Matrix33f>(Matrix33f::Identity());

		Matrix44f parent_transform = Matrix44f::Identity();
		parent_transform.block<3,3>(0,0) = configuration.axes_rotation.transpose() * parent_rotation * configuration.axes_rotation;
		parent_transform.block<1,3>(3,0) = (configuration.axes_rotation.transpose() * parent_translation).transpose();
		addFrame (parent_frame, frame_name, parent_transform);

		// Read points
		vector<LuaKey> point_keys = model_table["frames"][i]["points"].keys();

		for (vector<LuaKey>::iterator point_iter = point_keys.begin(); point_iter != point_keys.end(); point_iter++) {
			Vector3f coordinates = model_table["frames"][i]["points"][point_iter->string_value.c_str()]["coordinates"];
			Vector3f color = model_table["frames"][i]["points"][point_iter->string_value.c_str()]["color"].getDefault(Vector3f (1.f, 1.f, 1.f));
			bool draw_line = model_table["frames"][i]["points"][point_iter->string_value.c_str()]["draw_line"].getDefault(false);
			float line_width = model_table["frames"][i]["points"][point_iter->string_value.c_str()]["line_width"].getDefault(1.f);

			addPoint (point_iter->string_value, frame_name, coordinates, color, draw_line, line_width);
		}

		// Check if any points exist for current frame_name and add them
		for (int j = 0; j < contact_points.size(); j++) {
			if (frame_name == contact_points[j].parentBody) {
				addPoint (contact_points[j].name, 
					contact_points[j].parentBody, 
					contact_points[j].coordinates, 
					contact_points[j].color, 
					contact_points[j].draw_line, 
					contact_points[j].line_width);
			}
		}

		// Read joints to create model::state_descriptor
		int joint_dofs = model_table["frames"][i]["joint"].length();
		bool specialized_joint_type = true;

		if (joint_dofs == 1) {
			string dof_string = model_table["frames"][i]["joint"][1].getDefault<std::string>("");
			
			if (dof_string == "")
				specialized_joint_type = false;
			else if (dof_string == "JointTypeSpherical") {
				cerr << "Error: JointTypeSpherical not yet supported!" << endl;
				abort();
			} else if (dof_string == "JointTypeEulerZYX") {
				StateInfo state_info;
				state_info.frame_name = frame_name;
				state_info.type	= StateInfo::TransformTypeRotation;
				state_info.is_radian = true;

				state_info.axis = StateInfo::AxisTypeZ;
				state_descriptor.states.push_back (state_info);
				state_info.axis = StateInfo::AxisTypeY;
				state_descriptor.states.push_back (state_info);
				state_info.axis = StateInfo::AxisTypeX;
				state_descriptor.states.push_back (state_info);
			}
			if (dof_string == "JointTypeEulerXYZ") {
				StateInfo state_info;
				state_info.frame_name = frame_name;
				state_info.type	= StateInfo::TransformTypeRotation;
				state_info.is_radian = true;

				state_info.axis = StateInfo::AxisTypeX;
				state_descriptor.states.push_back (state_info);
				state_info.axis = StateInfo::AxisTypeY;
				state_descriptor.states.push_back (state_info);
				state_info.axis = StateInfo::AxisTypeZ;
				state_descriptor.states.push_back (state_info);
			}
			if (dof_string == "JointTypeEulerYXZ") {
				StateInfo state_info;
				state_info.frame_name = frame_name;
				state_info.type	= StateInfo::TransformTypeRotation;
				state_info.is_radian = true;

				state_info.axis = StateInfo::AxisTypeY;
				state_descriptor.states.push_back (state_info);
				state_info.axis = StateInfo::AxisTypeX;
				state_descriptor.states.push_back (state_info);
				state_info.axis = StateInfo::AxisTypeZ;
				state_descriptor.states.push_back (state_info);
			}
			if (dof_string == "JointTypeTranslationXYZ") {
				StateInfo state_info;
				state_info.frame_name = frame_name;
				state_info.type	= StateInfo::TransformTypeTranslation;

				state_info.axis = StateInfo::AxisTypeX;
				state_descriptor.states.push_back (state_info);
				state_info.axis = StateInfo::AxisTypeY;
				state_descriptor.states.push_back (state_info);
				state_info.axis = StateInfo::AxisTypeZ;
				state_descriptor.states.push_back (state_info);
			}
		} else {
			specialized_joint_type = false;
		}

		if (!specialized_joint_type) {
			StateInfo state_info;
			state_info.frame_name = frame_name;

			for (unsigned int di = 1; di < joint_dofs + 1; di++) {
				if (model_table["frames"][i]["joint"][di].length() != 6) {
					cerr << "LuaModel Error: invalid joint model subspace description at frames[" << i << "][\"joint\"][" << di << "]!" << endl;
					cerr << "Expected length 6 but got " << model_table["frames"][i]["joint"][di].length() << endl;
					abort();
				}

				double spatial_vector[6];
				Vector3f m_v, m_w;
				for (unsigned int dj = 0; dj < 3; dj++) {
					m_w[dj] = model_table["frames"][i]["joint"][di][dj + 1];
					m_v[dj] = model_table["frames"][i]["joint"][di][dj + 4];
				}

				if (m_v.squaredNorm() == 0.) {
					state_info.type	= StateInfo::TransformTypeRotation;
					state_info.is_radian = true;
					if (m_w == Vector3f (1.f, 0.f, 0.f))
						state_info.axis = StateInfo::AxisTypeX;
					else if (m_w == Vector3f (-1.f, 0.f, 0.f))
						state_info.axis = StateInfo::AxisTypeNegativeX;
					else if (m_w == Vector3f (0.f, 1.f, 0.f))
						state_info.axis = StateInfo::AxisTypeY;
					else if (m_w == Vector3f (0.f,-1.f, 0.f))
						state_info.axis = StateInfo::AxisTypeNegativeY;
					else if (m_w == Vector3f (0.f, 0.f, 1.f))
						state_info.axis = StateInfo::AxisTypeZ;
					else if (m_w == Vector3f (0.f, 0.f,-1.f))
						state_info.axis = StateInfo::AxisTypeNegativeZ;
					else {
						cerr << "LuaModel Error: only rotations around coordinate axes allowed (frames[" << i << "][\"joint\"][" << di << "])!" << endl;
						abort();
					}
				} else if (m_w.squaredNorm() == 0.) {
					state_info.type	= StateInfo::TransformTypeTranslation;
					if (m_v == Vector3f (1.f, 0.f, 0.f))
						state_info.axis = StateInfo::AxisTypeX;
					else if (m_v == Vector3f (-1.f, 0.f, 0.f))
						state_info.axis = StateInfo::AxisTypeNegativeX;
					else if (m_v == Vector3f (0.f, 1.f, 0.f))
						state_info.axis = StateInfo::AxisTypeY;
					else if (m_v == Vector3f (0.f,-1.f, 0.f))
						state_info.axis = StateInfo::AxisTypeNegativeY;
					else if (m_v == Vector3f (0.f, 0.f, 1.f))
						state_info.axis = StateInfo::AxisTypeZ;
					else if (m_v == Vector3f (0.f, 0.f,-1.f))
						state_info.axis = StateInfo::AxisTypeNegativeZ;
					else {
						cerr << "LuaModel Error: only rotations around coordinate axes allowed (frames[" << i << "][\"joint\"][" << di << "])!" << endl;
						abort();
					}
				} else {
					cerr << "LuaModel Error: only pure rotations around or pure translations along coordinate axes allowed (frames[" << i << "][\"joint\"][" << di << "])!" << endl;
					abort();
				}

				state_descriptor.states.push_back (state_info);
			}
		}

		// Read visuals
		int visual_count = model_table["frames"][i]["visuals"].length();

		for (int vi = 1; vi <= visual_count; vi++) {
			Vector3f dimensions = model_table["frames"][i]["visuals"][vi]["dimensions"].getDefault (Vector3f (0.f, 0.f, 0.f));
;
			Vector3f scale = model_table["frames"][i]["visuals"][vi]["scale"].getDefault (Vector3f (1.f, 1.f, 1.f));
			Vector3f color = model_table["frames"][i]["visuals"][vi]["color"].getDefault (Vector3f (1.f, 1.f, 1.f));
			
			Vector3f translate = model_table["frames"][i]["visuals"][vi]["translate"].getDefault (Vector3f (0.f, 0.f, 0.f));
			Vector3f mesh_center = model_table["frames"][i]["visuals"][vi]["mesh_center"].getDefault (Vector3f (1./0.f, 1./0.f, 1./0.f));

			Quaternion rotate = Quaternion::fromGLRotate (0., 1., 0., 0.);
			if (model_table["frames"][i]["visuals"][vi]["rotate"].exists()) {
				Vector3f axis = model_table["frames"][i]["visuals"][vi]["rotate"]["axis"].getDefault (Vector3f (1., 0., 0.));
				float angle = model_table["frames"][i]["visuals"][vi]["rotate"]["angle"].getDefault (0.f);
				rotate = Quaternion::fromGLRotate (angle, axis[0], axis[1], axis[2]);
			}

            // load the mesh or geometry
            MeshPtr mesh (new MeshVBO);

            string mesh_filename = model_table["frames"][i]["visuals"][vi]["src"].getDefault<std::string>("");
            bool have_geometry = model_table["frames"][i]["visuals"][vi]["geometry"].exists();

            if (have_geometry && mesh_filename != "") {
                cerr << "Error reading model " << model_filename << ": visual " << vi << " in frame " << i << ": attributes 'src' and 'geometry' are exclusive!" << endl;
                abort();
            } else if (have_geometry) {
                if (model_table["frames"][i]["visuals"][vi]["geometry"]["box"].exists()) {
                    Vector3f dimensions = model_table["frames"][i]["visuals"][vi]["geometry"]["box"]["dimensions"].getDefault (Vector3f (1.f, 1.f, 1.f));
                    (*mesh) = CreateCuboid(dimensions[0], dimensions[1], dimensions[2]);
                } else if (model_table["frames"][i]["visuals"][vi]["geometry"]["sphere"].exists()) {
                    float radius = model_table["frames"][i]["visuals"][vi]["geometry"]["sphere"]["radius"].getDefault (1.f);
                    unsigned int rows = static_cast<unsigned int>(model_table["frames"][i]["visuals"][vi]["geometry"]["sphere"]["rows"].getDefault (16.));
                    unsigned int segments = static_cast<unsigned int>(model_table["frames"][i]["visuals"][vi]["geometry"]["sphere"]["segments"].getDefault (16.));
                    mesh->join (SimpleMath::GL::ScaleMat44(radius, radius, radius), CreateUVSphere(rows, segments));
                } else if (model_table["frames"][i]["visuals"][vi]["geometry"]["capsule"].exists()) {
                    float radius = model_table["frames"][i]["visuals"][vi]["geometry"]["capsule"]["radius"].getDefault (1.f);
                    float length = model_table["frames"][i]["visuals"][vi]["geometry"]["capsule"]["length"].getDefault (2.f);
                    unsigned int rows = static_cast<unsigned int>(model_table["frames"][i]["visuals"][vi]["geometry"]["capsule"]["rows"].getDefault (16.));
                    unsigned int segments = static_cast<unsigned int>(model_table["frames"][i]["visuals"][vi]["geometry"]["capsule"]["segments"].getDefault (16.));
                    mesh->join (SimpleMath::GL::RotateMat44(90.f, 1.f, 0.f, 0.f), CreateCapsule(rows, segments, length, radius));
                } else if (model_table["frames"][i]["visuals"][vi]["geometry"]["cylinder"].exists()) {
                    float radius = model_table["frames"][i]["visuals"][vi]["geometry"]["cylinder"]["radius"].getDefault (1.f);
                    float length = model_table["frames"][i]["visuals"][vi]["geometry"]["cylinder"]["length"].getDefault (2.f);
                    unsigned int rows = static_cast<unsigned int>(model_table["frames"][i]["visuals"][vi]["geometry"]["cylinder"]["rows"].getDefault (16.));
                    unsigned int segments = static_cast<unsigned int>(model_table["frames"][i]["visuals"][vi]["geometry"]["cylinder"]["segments"].getDefault (16.));
                    mesh->join (SimpleMath::GL::ScaleMat44(radius, radius, length) * SimpleMath::GL::RotateMat44(90.f, 1.f, 0.f, 0.f) , CreateCylinder(segments));
                } else {
                    vector<LuaKey> keys = model_table["frames"][i]["visuals"][vi]["geometry"].keys();
                    if (keys.size() == 1) {
                        cerr << "Error reading model " << model_filename << ": visual " << vi << " in frame " << i << ": unknown geometry type '" << keys[0] << "'" << endl;
                        abort();
                    } else {
                        cerr << "Error reading model " << model_filename << ": visual " << vi << " in frame " << i << ": invalid geometry description." << endl;
                        abort();
                    }
                }
            } else if (mesh_filename != "") {
                // check whether we have the mesh, if not try to load it
                MeshMap::iterator mesh_iter = meshmap.find (mesh_filename);
                if (mesh_iter == meshmap.end()) {
                    // check whether we want to extract a sub object within the obj file
                    string submesh_name = "";
                    if (mesh_filename.find (':') != string::npos) {
                        submesh_name = mesh_filename.substr (mesh_filename.find(':') + 1, mesh_filename.size());
                        mesh_filename = mesh_filename.substr (0, mesh_filename.find(':'));
                        string mesh_file_location = find_mesh_file_by_name (mesh_filename);
                        cout << "Loading sub object " << submesh_name << " from file " << mesh_file_location << endl;
                        mesh->loadOBJ(mesh_file_location.c_str(), submesh_name.c_str());
                    } else {
                        string mesh_file_location = find_mesh_file_by_name (mesh_filename);
                        cout << "Loading mesh " << mesh_file_location << endl;
                        mesh->loadOBJ(mesh_file_location.c_str());
                    }

                    if (!skip_vbo_generation)
                        mesh->generate_vbo();

                    meshmap[mesh_filename] = mesh;

                    mesh_iter = meshmap.find (mesh_filename);
                }

                mesh = mesh_iter->second;
            } else {
                cerr << "Error reading model " << model_filename << ": visual " << vi << " in frame " << i << ": neither 'src' nor 'geometry' found!" << endl;
                abort();
            }

			addSegment (frame_name, mesh, dimensions, color, translate, rotate, scale, mesh_center);
		}
	}

	initDefaultFrameTransform();

	model_filename = filename;

	return true;
}
