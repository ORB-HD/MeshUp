/*
 * MeshUp - A visualization tool for multi-body systems based on skeletal
 * animation and magic.
 *
 * Copyright (c) 2011-2012 Martin Felis <martin.felis@iwr.uni-heidelberg.de>
 *
 * Licensed under the MIT license. See LICENSE for more details.
 */

#include "GL/glew.h"

#include "Animation.h"

#include "SimpleMath/SimpleMathGL.h"
#include "string_utils.h"

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

#include "Model.h"
#include "Animation.h"

using namespace std;

const string invalid_id_characters = "{}[],;: \r\n\t#";

void TransformInfo::applyStateValue (const StateInfo &state_info, float value, const FrameConfig &frame_config) {
	// handle radian
	if (state_info.type==StateInfo::TransformTypeRotation && state_info.is_radian) {
		value *= 180. / M_PI;
	}

	Vector3f axis (0.f, 0.f, 0.f);
	switch (state_info.axis) {
		case StateInfo::AxisTypeX: axis[0] = 1.f; break;
		case StateInfo::AxisTypeY: axis[1] = 1.f; break;
		case StateInfo::AxisTypeZ: axis[2] = 1.f; break;
		case StateInfo::AxisTypeNegativeX: axis[0] = -1.f; break;
		case StateInfo::AxisTypeNegativeY: axis[1] = -1.f; break;
		case StateInfo::AxisTypeNegativeZ: axis[2] = -1.f; break;
		default: cerr << "Error: invalid axis type!"; abort();
	}

	axis = frame_config.axes_rotation.transpose() * axis;

	if (state_info.type == StateInfo::TransformTypeTranslation) {
		translation = translation + axis * value;
	} else if (state_info.type == StateInfo::TransformTypeScale) {
		switch (state_info.axis) {
			case StateInfo::AxisTypeX: scaling[0] = value; break;
			case StateInfo::AxisTypeY: scaling[1] = value; break;
			case StateInfo::AxisTypeZ: scaling[2] = value; break;
			case StateInfo::AxisTypeNegativeX: scaling[0] = -value; break;
			case StateInfo::AxisTypeNegativeY: scaling[1] = -value; break;
			case StateInfo::AxisTypeNegativeZ: scaling[2] = -value; break;
			default: cerr << "Error: invalid axis type!"; abort();	
		}
	} else if (state_info.type == StateInfo::TransformTypeRotation) {
		rotation_quaternion = SimpleMath::GL::Quaternion::fromGLRotate(value, axis[0], axis[1], axis[2]) * rotation_quaternion; 
	}
}

/** \brief Searches for the proper animation interpolants and updates the
 * poses */
void InterpolateModelFramesFromAnimation (MeshupModelPtr model, AnimationPtr animation, float time);

bool Animation::loadFromFile (const char* filename, const FrameConfig &frame_config, bool strict) {
	ifstream file_in (filename);

	if (!file_in) {
		cerr << "Error opening animation file " << filename << "!";

		if (strict)
			exit (1);

		return false;
	}

	configuration = frame_config;

	double force_fps_previous_frame = 0.;
	int force_fps_frame_count = 0;
	int force_fps_skipped_frame_count = 0;
	bool last_line = false;
	bool csv_mode = false;
	raw_values.clear();

	string filename_str (filename);

	if (filename_str.size() > 4 && filename_str.substr(filename_str.size() - 4) == ".csv") 
		csv_mode = true;

	cout << "Loading animation " << filename << endl;

	string previous_line;
	string line;

	bool found_column_section = false;
	bool found_data_section = false;
	bool column_section = false;
	bool data_section = false;
	int state_index = 0;
	int line_number = 0;
	state_descriptor.states.clear();

	std::list<std::string> state_frame_names;

	while (!file_in.eof()) {
		previous_line = line;
		getline (file_in, line);

		// make sure the last line is read no matter what
		if (file_in.eof()) {
			last_line = true;
			line = previous_line;
		} else {
			line_number++;
		}
	
		line = strip_comments (strip_whitespaces( (line)));
		
		// skip lines with no information
		if (line.size() == 0)
			continue;

		// check whether we have a CSV file with just the data
		if (!found_column_section && !found_data_section) {
			float value = 0.f;
			istringstream value_stream (line.substr (0, line.find_first_of (" \t\r") - 1));
			value_stream >> value;
			if (!value_stream.fail()) {
				data_section = true;	
				found_data_section = true;
			}
		}
	
		if (line.substr (0, string("COLUMNS:").size()) == "COLUMNS:") {
			found_column_section = true;
			column_section = true;

			// we set it to -1 and can then easily increasing the value
			state_index = -1;

			line = strip_comments (strip_whitespaces (line.substr(string("COLUMNS:").size(), line.size())));
			if (line.size() == 0)
				continue;
		}

		if (line.substr (0, string("DATA:").size()) == "DATA:") {
			found_data_section = true;
			column_section = false;
			data_section = true;
			continue;
		} else if (!data_section && line.substr (0, string("DATA_FROM:").size()) == "DATA_FROM:") {
			file_in.close();

			boost::filesystem::path data_path (strip_whitespaces(line.substr(string("DATA_FROM:").size(), line.size())));

			// search for the file in the same directory as the original file,
			// unless we have an absolutue path or by starting the file
			// with "./" or ".." to expicilty want the file loaded relative to
			// the current work directory.
			if (data_path.string()[0] != '/' && data_path.string()[0] != '.') {
				boost::filesystem::path file_path (filename);
				boost::filesystem::path data_directory = file_path.parent_path();
				data_path = data_directory /= data_path;
			}

			file_in.open(data_path.string().c_str());
			cout << "Loading animation data from " << data_path.string() << endl;

			if (!file_in) {
				cerr << "Error opening animation file " << data_path.string() << "!" << std::endl;

				if (strict)
					exit (1);

				return false;
			}

			filename_str = data_path.string();
			line_number = 0;

			found_data_section = true;
			column_section = false;
			data_section = true;
			continue;
		}

		if (column_section) {
			// do columny stuff
			// cout << "COLUMN:" << line << endl;

			std::vector<string> elements;
			
			if (csv_mode)
				elements = tokenize_csv_strip_whitespaces (line);
			else
				elements = tokenize_strip_whitespaces (line, ",\t\n\r");

			for (int ei = 0; ei < elements.size(); ei++) {
				// skip elements that had multiple spaces in them
				if (elements[ei].size() == 0)
					continue;

				// it's safe to increase the column index here, as we did
				// initialize it with -1
				state_index++;

				string state_def = strip_whitespaces(elements[ei]);
				// cout << "  E: " << state_def << endl;

				if (tolower(state_def) == "time") {
					if (ei != 0) {
						cerr << "Error: first column must be time column (it was column " << ei <<")!" << endl;
						abort();
					}
					StateInfo state_info;
					state_info.is_time_column = true;
					state_descriptor.states.push_back(state_info);
					// cout << "Setting time column to " << state_index << endl;
					continue;
				}
				if (tolower(state_def) == "empty") {
					StateInfo state_info;
					state_info.is_empty = true;
					state_descriptor.states.push_back(state_info);
					continue;
				}
				
				std::vector<string> spec = tokenize(state_def, ":");
				if (spec.size() < 3 || spec.size() > 4) {
					cerr << "Error: parsing column definition '" << state_def << "' in " << filename << " line " << line_number << endl;

					if (strict)
						abort();

					return false;
				}

				// frame name
				string frame_name = strip_whitespaces(spec[0]).c_str();

				// the transform type
				string type_str = tolower(strip_whitespaces(spec[1]));
				StateInfo::TransformType type = StateInfo::TransformTypeUnknown;
				if (type_str == "rotation" || type_str == "r")
					type = StateInfo::TransformTypeRotation;
				else if (type_str == "translation" || type_str == "t")
					type = StateInfo::TransformTypeTranslation;
				else if (type_str == "scale" || type_str == "s")
					type = StateInfo::TransformTypeScale;
				else {
					cerr << "Error: Unknown transform type '" << spec[1] << "' in " << filename << " line " << line_number << endl;
					
					if (strict)
						exit (1);

					return false;
				}

				// and the axis
				string axis_str = tolower(strip_whitespaces(spec[2]));
				StateInfo::AxisType axis_name;
				if (axis_str == "x")
					axis_name = StateInfo::AxisTypeX;
				else if (axis_str == "y")
					axis_name = StateInfo::AxisTypeY;
				else if (axis_str == "z")
					axis_name = StateInfo::AxisTypeZ;
				else if (axis_str == "-x")
					axis_name = StateInfo::AxisTypeNegativeX;
				else if (axis_str == "-y")
					axis_name = StateInfo::AxisTypeNegativeY;
				else if (axis_str == "-z")
					axis_name = StateInfo::AxisTypeNegativeZ;
				else {
					cerr << "Error: Unknown axis name '" << spec[2] << "' in " << filename << " line " << line_number << endl;

					if (strict)
						exit (1);

					return false;
				}

				bool unit_is_radian = false;
				if (spec.size() == 4) {
					string unit_str = tolower(strip_whitespaces(spec[3]));
					if (unit_str == "r" || unit_str == "rad" || unit_str == "radian" || unit_str == "radians")
						unit_is_radian = true;
				}

				StateInfo col_info;
				col_info.frame_name = frame_name;
				col_info.type = type;
				col_info.axis = axis_name;
				col_info.is_radian = unit_is_radian;

				// cout << "Adding column " << state_index << " " << frame_name << ", " << type << ", " << axis_name << " radians = " << col_info.is_radian << endl;
				state_descriptor.states.push_back (col_info);
			}

			continue;
		}

		if (data_section) {
			// Data part:
			// columns have been read
		
			std::vector<string> columns;
			if (csv_mode)
				columns = tokenize_csv_strip_whitespaces (line);
			else
				columns = tokenize (line);

			if (columns.size() < state_descriptor.states.size()) {
				cerr << "Error: only found " << columns.size() << " data columns in file " 
					<< filename_str << " line " << line_number << ", but " << state_descriptor.states.size() << " columns were specified in the COLUMNS section." << endl;
				abort();
			}

			assert (columns.size() >= state_descriptor.states.size());

			float state_time;
			float value;
			istringstream value_stream (columns[0]);
			if (!(value_stream >> value)) {
				cerr << "Error: could not convert value string '" << value_stream.str() << "' into a number in " << filename << ":" << line_number << "." << endl;
				abort();
			}
			
			state_time = value;

			// convert the data to raw values
			VectorNd state_values (VectorNd::Zero (columns.size()));
			for (int ci = 0; ci < columns.size(); ci++) {
				istringstream value_stream (columns[ci]);
				if (!(value_stream >> value)) {
					cerr << "Error: could not convert value string '" << value_stream.str() << "' into a number in " << filename << ":" << line_number << "." << endl;
					abort();
				}
				state_values[ci] = value;
			}
			raw_values.push_back(state_values);

			force_fps_previous_frame = state_time;
			force_fps_frame_count++;
			// cout << "Reading frame at t = " << scientific << force_fps_previous_frame << endl;

			if (state_time > duration)
				duration = state_time;

			continue;
		}
	}

	if (!found_data_section) {
		cerr << "Error: did not find DATA: section in animation file!" << endl;
		abort();
	}

	animation_filename = filename;

	return true;
}

void InterpolateModelFramePose (FramePtr frame, const TransformInfo &transform_prev, const TransformInfo &transform_next, const float fraction) {
	frame->pose_translation = transform_prev.translation + fraction * (transform_next.translation - transform_prev.translation);
	frame->pose_rotation_quaternion = transform_prev.rotation_quaternion.slerp (fraction, transform_next.rotation_quaternion);
	frame->pose_scaling = transform_prev.scaling + fraction * (transform_next.scaling - transform_prev.scaling);
}

KeyFrame Animation::getKeyFrameAtFrameIndex (int frame_index) {
	KeyFrame keyframe;

	if (raw_values.size() == 0)
		return keyframe;

	keyframe.timestamp = raw_values[frame_index][0];

	for (int ci = 1; ci < state_descriptor.states.size(); ci++) {
		if (state_descriptor.states[ci].is_empty)
			continue;

		if (keyframe.transformations.find(state_descriptor.states[ci].frame_name) == keyframe.transformations.end())
			keyframe.transformations[state_descriptor.states[ci].frame_name] = TransformInfo();

		TransformInfo transform = keyframe.transformations[state_descriptor.states[ci].frame_name];

		transform.applyStateValue (state_descriptor.states[ci], raw_values[frame_index][ci], configuration);

		keyframe.transformations[state_descriptor.states[ci].frame_name] = transform;
	}

	return keyframe;
}

void Animation::getInterpolatingIndices (float time, int *frame_prev, int *frame_next, float *time_fraction) {
	*frame_prev= 0;
	*frame_next= 0;
	*time_fraction = 0.f;

	if (raw_values.size() > 1) {
		while (time > raw_values[*frame_next][0]) {
			*frame_prev = *frame_next;
			(*frame_next) ++;

			if (*frame_next == raw_values.size()) {
				*frame_prev = raw_values.size() - 2;
				*frame_next = raw_values.size() - 1;
				*time_fraction = 1.;
				break;
			}
	
			*time_fraction = (time - raw_values[*frame_prev][0]) / (raw_values[*frame_next][0] - raw_values[*frame_prev][0]);
		}
	}
}

KeyFrame Animation::getKeyFrameAtTime (float time) {
	int frame_prev = 0, frame_next = 0;
	float time_fraction = 0.f;

	getInterpolatingIndices (time, &frame_prev, &frame_next, &time_fraction);

	// compute interpolating frame indices and the time fraction
	// perform interpolation for all frames
	KeyFrame keyframe_prev = getKeyFrameAtFrameIndex (frame_prev);
	KeyFrame keyframe_next = getKeyFrameAtFrameIndex (frame_next);
	KeyFrame keyframe_interpolated = keyframe_prev;

	std::map<std::string, TransformInfo>::iterator frame_iter = keyframe_prev.transformations.begin();

	while (frame_iter != keyframe_prev.transformations.end()) {
		std::string frame_name = frame_iter->first;
		
		TransformInfo transform_prev = keyframe_prev.transformations[frame_name];
		TransformInfo transform_next = keyframe_next.transformations[frame_name];

		transform_prev.translation = transform_prev.translation + time_fraction * (transform_next.translation - transform_prev.translation);

		transform_prev.rotation_quaternion = transform_prev.rotation_quaternion.slerp (time_fraction, transform_next.rotation_quaternion);
		transform_prev.scaling = transform_prev.scaling + time_fraction * (transform_next.scaling - transform_prev.scaling);

		// slerp can return non-normal Quaternion
		transform_prev.rotation_quaternion.normalize();

		keyframe_interpolated.transformations[frame_name] = transform_prev;

		frame_iter++;
	}

	return keyframe_interpolated;
}

void ModelApplyKeyFrame (MeshupModelPtr model, KeyFrame &keyframe) {
	std::map<std::string, TransformInfo>::iterator frame_iter = keyframe.transformations.begin();

	while (frame_iter != keyframe.transformations.end()) {
		std::string frame_name = frame_iter->first;

		if (model->frameExists (frame_name.c_str())) {
			FramePtr model_frame = model->findFrame(frame_name.c_str());
			model_frame->pose_translation = frame_iter->second.translation;
			model_frame->pose_rotation_quaternion = frame_iter->second.rotation_quaternion;
			model_frame->pose_scaling = frame_iter->second.scaling;
		} else if (model->pointExists(frame_name.c_str())){
			unsigned int point_index = model->getPointIndex (frame_name.c_str());

			model->points[point_index].coordinates = frame_iter->second.translation;
		}

		frame_iter++;
	}
}

void UpdateModelFromAnimation (MeshupModelPtr model, AnimationPtr animation, float time) {
	// Use model state descriptor if the animation does not have one
	if (animation->state_descriptor.states.size() == 0) {
		animation->state_descriptor = model->state_descriptor;
		animation->configuration = model->configuration;
	}

	KeyFrame keyframe = animation->getKeyFrameAtTime (time);
	ModelApplyKeyFrame (model, keyframe);

	model->updateFrames();
	model->updateSegments();
}
