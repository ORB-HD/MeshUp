#include "GL/glew.h"

#include "Animation.h"

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
#include "MeshupModel.h"
#include "Animation.h"

using namespace std;

const string invalid_id_characters = "{}[],;: \r\n\t#";

void InterpolateModelFramesFromAnimation (MeshupModelPtr model, AnimationPtr animation, float time);

/** \brief Description of the description of a column section entry. */
struct ColumnInfo {
	ColumnInfo() :
		frame_name (""),
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
		AxisMX, // -X
		AxisMY, // -Y
		AxisMZ  // -Z
	};
	std::string frame_name;
	TransformType type;
	AxisName axis;

	bool is_time_column;
	bool is_empty;
	bool is_radian;
};

/** \brief Keeps transformation information for all model frames at a single keyframe 
 *
 * This struct is used to assemble the pose information for all model
 * frames in a single keyframe. It maps from the column index to the actual
 * model frame and transformation type.
 */
struct AnimationKeyPoses {
	float timestamp;
	std::vector<ColumnInfo> columns;
	typedef std::map<std::string, FramePoseInfo> FramePoseMap;
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

		string frame_name = col_info.frame_name;

		if (frame_poses.find(frame_name) == frame_poses.end()) {
			// create new frame and insert it
			frame_poses[frame_name] = FramePoseInfo();
		}

		if (col_info.type == ColumnInfo::TypeRotation) {
			if (col_info.axis == ColumnInfo::AxisX) {
				frame_poses[frame_name].rotation_angles[0] = value;
			}
			if (col_info.axis == ColumnInfo::AxisY) {
				frame_poses[frame_name].rotation_angles[1] = value;
			}
			if (col_info.axis == ColumnInfo::AxisZ) {
				frame_poses[frame_name].rotation_angles[2] = value;
			}
			if (col_info.axis == ColumnInfo::AxisMX) {
				frame_poses[frame_name].rotation_angles[0] = -value;
			}
			if (col_info.axis == ColumnInfo::AxisMY) {
				frame_poses[frame_name].rotation_angles[1] = -value;
			}
			if (col_info.axis == ColumnInfo::AxisMZ) {
				frame_poses[frame_name].rotation_angles[2] = -value;
			}
		} else if (col_info.type == ColumnInfo::TypeTranslation) {
			if (col_info.axis == ColumnInfo::AxisX) {
				frame_poses[frame_name].translation[0] = value;
			}
			if (col_info.axis == ColumnInfo::AxisY) {
				frame_poses[frame_name].translation[1] = value;
			}
			if (col_info.axis == ColumnInfo::AxisZ) {
				frame_poses[frame_name].translation[2] = value;
			}	
			if (col_info.axis == ColumnInfo::AxisMX) {
				frame_poses[frame_name].translation[0] = -value;
			}
			if (col_info.axis == ColumnInfo::AxisMY) {
				frame_poses[frame_name].translation[1] = -value;
			}
			if (col_info.axis == ColumnInfo::AxisMZ) {
				frame_poses[frame_name].translation[2] = -value;
			}	
		} else if (col_info.type == ColumnInfo::TypeScale) {
			if (col_info.axis == ColumnInfo::AxisX) {
				frame_poses[frame_name].scaling[0] = value;
			}
			if (col_info.axis == ColumnInfo::AxisY) {
				frame_poses[frame_name].scaling[1] = value;
			}
			if (col_info.axis == ColumnInfo::AxisZ) {
				frame_poses[frame_name].scaling[2] = value;
			}	
			if (col_info.axis == ColumnInfo::AxisMX) {
				frame_poses[frame_name].scaling[0] = -value;
			}
			if (col_info.axis == ColumnInfo::AxisMY) {
				frame_poses[frame_name].scaling[1] = -value;
			}
			if (col_info.axis == ColumnInfo::AxisMZ) {
				frame_poses[frame_name].scaling[2] = -value;
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

float Animation::getPrevKeyFrameTime () const {
	AnimationTrackMap::const_iterator track_iter = frame_animation_tracks.begin();

	float closest = 0.f;

	while (track_iter != frame_animation_tracks.end()) {
		KeyFrameList::const_iterator keyframe_iter = track_iter->second.keyframes.begin();

		bool time_before_current_time = true;
		float closest_for_track = 0.f;

		while (keyframe_iter != track_iter->second.keyframes.end()) {
			if (keyframe_iter->timestamp >= current_time) {
				break;
			}

			closest_for_track = keyframe_iter->timestamp;
			keyframe_iter++;
		}

		if (closest_for_track > closest)
			closest = closest_for_track;

		track_iter++;
	}

	return closest;	
}

float Animation::getNextKeyFrameTime () const {
	AnimationTrackMap::const_iterator track_iter = frame_animation_tracks.begin();

	float closest = duration;

	while (track_iter != frame_animation_tracks.end()) {
		KeyFrameList::const_iterator keyframe_iter = track_iter->second.keyframes.begin();

		bool time_before_current_time = true;
		float closest_for_track = duration;

		while (keyframe_iter != track_iter->second.keyframes.end()) {
			if (keyframe_iter->timestamp <= current_time) {
				keyframe_iter++;
				continue;
			}
			closest_for_track = keyframe_iter->timestamp;
			break;
		}

		if (closest_for_track < closest)
			closest = closest_for_track;

		track_iter++;
	}

	return closest;	
}

void Animation::addFramePose (
			const std::string &frame_name,
			float time,
			const Vector3f &frame_translation,
			const Vector3f &frame_rotation_angles,
			const Vector3f &frame_scaling
		) {
	FramePoseInfo pose;
	pose.timestamp = time;
	pose.translation = configuration.axes_rotation.transpose() * frame_translation;
	pose.rotation_angles = frame_rotation_angles;
	pose.rotation_quaternion = configuration.convertAnglesToQuaternion (frame_rotation_angles);
	pose.scaling = frame_scaling;

	frame_animation_tracks[frame_name].keyframes.push_back(pose);

	// update the duration of the animation
	if (time > duration)
		duration = time;
}

bool Animation::loadFromFile (const char* filename, bool strict) {
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

				// frame name
				string frame_name = strip_whitespaces(spec[0]).c_str();

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
				col_info.frame_name = frame_name;
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
				FramePoseInfo pose = frame_pose_iter->second;

				// cout << "addFramePose("
				// 	<< "  " << frame->name << endl
				// 	<< "  " << pose.timestamp << endl
				// 	<< "  " << pose.translation.transpose() << endl
				// 	<< "  " << pose.rotation.transpose() << endl
				// 	<< "  " << pose.scaling.transpose() << endl;

				addFramePose (frame_pose_iter->first,
						pose.timestamp,
						pose.translation,
						pose.rotation_angles,
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

void AnimationTrack::findInterpolationPoses (float time, FramePoseInfo &pose_start, FramePoseInfo &pose_end, float &fraction) {
	if (keyframes.size() == 0) {
		pose_start = FramePoseInfo();
		pose_end = FramePoseInfo();
		fraction = 0.;
	} else if (keyframes.size() == 1) {
		pose_start = *(keyframes.begin());
		pose_end = *(keyframes.begin());
		fraction = 0.;
	}

	KeyFrameList::iterator frame_prev_iter = keyframes.begin();
	KeyFrameList::iterator frame_next_iter = keyframes.begin();
	KeyFrameList::iterator frame_iter = keyframes.begin();

	while (frame_iter != keyframes.end() && frame_iter->timestamp <= time) {
		frame_prev_iter = frame_iter;
		frame_iter++;
		frame_next_iter = frame_iter;
	}

	if (frame_iter == keyframes.end()) {
		frame_next_iter = frame_prev_iter;
	}

	pose_start = *(frame_prev_iter);
	pose_end = *(frame_next_iter);

	float duration = frame_next_iter->timestamp - frame_prev_iter->timestamp;
	if (duration == 0.f) {
		pose_end = *(frame_prev_iter);
		fraction = 0.;
		return;
	}

	fraction = (time - pose_start.timestamp) / (duration);

	if (fraction > 1.f)
		fraction = 1.f;
	if (fraction < 0.f)
		fraction = 0.f;
}

void InterpolateModelFramePose (FramePtr frame, const FramePoseInfo &start_pose, const FramePoseInfo &end_pose, const float fraction) {
	frame->pose_translation = start_pose.translation + fraction * (end_pose.translation - start_pose.translation);
	frame->pose_rotation_quaternion = start_pose.rotation_quaternion.slerp (fraction, end_pose.rotation_quaternion);
	frame->pose_scaling = start_pose.scaling + fraction * (end_pose.scaling - start_pose.scaling);
}

void InterpolateModelFramesFromAnimation (MeshupModelPtr model, AnimationPtr animation, float time) {
	// update the time
	animation->current_time = time;

	if (animation->current_time > animation->duration) {
		if (animation->loop) {
			while (animation->current_time > animation->duration)
				animation->current_time -= animation->duration;
		} else {
			animation->current_time = animation->duration;
		}
	}

	// loop over the animation tracks and update the pose informations in the
	// model frames
	AnimationTrackMap::iterator track_iter = animation->frame_animation_tracks.begin();
	while (track_iter != animation->frame_animation_tracks.end()) {
		FramePtr model_frame = model->findFrame ((track_iter->first).c_str());
		FramePoseInfo start_pose, end_pose;
		float fraction;
	
		track_iter->second.findInterpolationPoses (time, start_pose, end_pose, fraction);
		InterpolateModelFramePose (model_frame, start_pose, end_pose, fraction);

		track_iter++;
	}
}

void UpdateModelSegmentTransformations (MeshupModelPtr model) {
	MeshupModel::SegmentList::iterator seg_iter = model->segments.begin();

	while (seg_iter != model->segments.end()) {
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

void UpdateModelFromAnimation (MeshupModelPtr model, AnimationPtr animation, float time) {
	InterpolateModelFramesFromAnimation (model, animation, time);

	model->updateFrames();

	UpdateModelSegmentTransformations(model);
}
