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

bool compare_keyframe_timestamp (KeyFrame first, KeyFrame second) {
	if (first.timestamp == second.timestamp) {
		cerr << "Error: found two keyframes with the same timestamp at time " << first.timestamp << endl;
		abort();
	}

	if (first.timestamp < second.timestamp)
		return true;
	return false;
}

bool compare_keyvalues_index (KeyValue first, KeyValue second) {
	if (first.index == second.index) {
		cerr << "Error: found two keyvalues with the same index of " << first.index << endl;
		abort();
	}

	if (first.index < second.index)
		return true;

	return false;
}

float RawValues::getPrevKeyFrameTime (const float time) const {
	RawKeyFrameList::const_iterator frame_iter = frames.begin();

	// check if the value is before the first keyframe
	if (frame_iter != frames.end() && frame_iter->timestamp > time) {
		return frame_iter->timestamp;
	}

	while (frame_iter != frames.end() && frame_iter->timestamp < time) {
		frame_iter++;
	}

	frame_iter--;
	
	return frame_iter->timestamp;
}

float RawValues::getNextKeyFrameTime (const float time) const {
	RawKeyFrameList::const_iterator frame_iter = frames.begin();

	// check if the value is before the first keyframe
	if (frame_iter != frames.end() && frame_iter->timestamp > time) {
		return frame_iter->timestamp;
	}

	while (frame_iter != frames.end() && frame_iter->timestamp < time) {
		frame_iter++;
	}

	if (frame_iter == frames.end()) {
		frame_iter--;
	}

	return frame_iter->timestamp;
}

void RawValues::addKeyValue (const float time, unsigned int index, float value) {
	RawKeyFrameList::iterator frame_iter = getKeyFrameIter (time);

	if (frame_iter == frames.end()) {
		KeyFrame frame;
		frame.timestamp = time;

		frames.push_back (frame);
		frames.sort(compare_keyframe_timestamp);

		frame_iter = getKeyFrameIter(time);
	}

	assert (frame_iter != frames.end());

	if (haveKeyValue (time, index)) {
		deleteKeyValue (time, index);
	}

	frame_iter->value_list.push_back (KeyValue (value, index));
	frame_iter->value_list.sort(compare_keyvalues_index);
}

float RawValues::getKeyValue (const float time, unsigned int index) {
	assert (haveKeyValue (time, index));

	RawKeyFrameList::iterator frame_iter = getKeyFrameIter (time);

	if (frame_iter == frames.end()) {
		KeyFrame frame;
		frame.timestamp = time;

		frames.push_back (frame);
		frames.sort(compare_keyframe_timestamp);

		frame_iter = getKeyFrameIter(time);
	}

	assert (frame_iter != frames.end());

	KeyValueList::iterator value_iter = frame_iter->value_list.begin();

	while (value_iter != frame_iter->value_list.end()) {
		if (value_iter->index == index)
			return value_iter->value;
		value_iter++;
	}

	cerr << "Error: could not find value at time " << time << " and index " << index<< endl;
	abort();

	return std::numeric_limits<float>::quiet_NaN();
}

float RawValues::getPrevKeyValue (const float time, unsigned int index, float *frame_time) const {
	RawKeyFrameList::const_iterator frame_iter = frames.begin();

	bool found_any_value = false;
	float result_value = 0.; 
	float result_time = 0.f;

	while (frame_iter != frames.end()) { 
		if (frame_iter->timestamp > time && found_any_value) {
			break;
		}
		
		if (haveKeyValue (frame_iter->timestamp, index)) {
			result_value = const_cast<RawValues*>(this)->getKeyValue  (frame_iter->timestamp, index);
			result_time = frame_iter->timestamp;
			found_any_value = true;
		}
 
		frame_iter++;
	}

	if (frame_time)
		*frame_time = result_time;

	return result_value;
}

float RawValues::getNextKeyValue (const float time, unsigned int index, float *frame_time) const {
	RawKeyFrameList::const_iterator frame_iter = frames.end();

	bool found_any_value = false;
	float result_value = 0.; 
	float result_time = 0.f;

	while (frame_iter != frames.begin()) { 
		frame_iter--;

		if (frame_iter->timestamp < time && found_any_value) {
			break;
		}
		
		if (haveKeyValue (frame_iter->timestamp, index)) {
			result_value = const_cast<RawValues*>(this)->getKeyValue  (frame_iter->timestamp, index);
			result_time = frame_iter->timestamp;
			found_any_value = true;
		}
	}

	if (frame_time)
		*frame_time = result_time;

	return result_value;
}

void RawValues::deleteKeyValue (const float time, unsigned int index) {
	if (!haveKeyValue (time, index)) {
		return;
	}

	RawKeyFrameList::iterator frame_iter = getKeyFrameIter (time);
	if (frame_iter == frames.end()) {
		KeyFrame frame;
		frame.timestamp = time;

		frames.push_back (frame);
		frames.sort(compare_keyframe_timestamp);

		frame_iter = getKeyFrameIter(time);
	}

	assert (frame_iter != frames.end());

	KeyValueList::iterator value_iter = frame_iter->value_list.begin();
	unsigned int old_size = frame_iter->value_list.size();

	while (value_iter != frame_iter->value_list.end()) {
		if (value_iter->index == index) {
			frame_iter->value_list.erase (value_iter);
			return;
		}
		value_iter++;
	}
	assert (old_size - 1 == frame_iter->value_list.size());

	cerr << "Error: could not find value at time " << time << " and index " << index<< endl;
	abort();
}

bool RawValues::haveKeyValue (const float time, unsigned int index) const {
	RawKeyFrameList::iterator frame_iter = const_cast<RawValues*>(this)->getKeyFrameIter (time);
	if (frame_iter == frames.end())
		return false;

	KeyValueList::iterator value_iter = frame_iter->value_list.begin();

	while (value_iter != frame_iter->value_list.end()) {
		if (value_iter->index == index)
			return true;
		value_iter++;
	}
	
	return false;
}

RawKeyFrameList::iterator RawValues::getKeyFrameIter (const float time) {
	RawKeyFrameList::iterator frame_iter = frames.begin();

	while (frame_iter != frames.end()) {
		if (fabs (frame_iter->timestamp - time) < 1.0e-4) {
			break;
		}
		frame_iter++;
	}

	return frame_iter;
}

bool RawValues::haveKeyFrame (const float time) const {
	RawKeyFrameList::iterator frame_iter = const_cast<RawValues*>(this)->getKeyFrameIter (time);

	if (frame_iter != frames.end()) {
		return true;
	}

	return false;
}

void RawValues::moveKeyFrame (const float old_time, const float new_time) {
	RawKeyFrameList::iterator frame_old = getKeyFrameIter (old_time);

	if (fabsf(old_time - new_time) < 1.0e-5)
		return;

	if (frame_old == frames.end()) {
		cerr << "Error: cannot move key frames. No key frame found at time " << old_time << endl;
		abort();
	}

	RawKeyFrameList::iterator frame_new = getKeyFrameIter (new_time);

	if (frame_new != frames.end()) {
		// we merge the existing into the current and overwrite possible
		// existing values.
		KeyValueList::iterator value_iter = frame_new->value_list.begin();

		while (value_iter != frame_new->value_list.end()) {
			if (!haveKeyValue (old_time, value_iter->index)) {
				addKeyValue (old_time, value_iter->index, value_iter->value);
			}

			value_iter++;
		}

		frames.erase (frame_new);
	}

	frame_old->timestamp = new_time;
	frames.push_back (*frame_old);
	frames.erase (frame_old);
	frames.sort(compare_keyframe_timestamp);
}

float RawValues::getInterpolatedValue (const float time, unsigned int index) const {
	float prev_key_time;
	float prev_key_value = getPrevKeyValue (time, index, &prev_key_time);

	float next_key_time;
	float next_key_value = getNextKeyValue (time, index, &next_key_time);

	float duration = next_key_time - prev_key_time;
	if (fabs (duration) < 1.0e-6) {
		return next_key_value;
	}

	float fraction = (time - prev_key_time) / duration;

	return prev_key_value + fraction * (next_key_value - prev_key_value);
}

void InterpolateModelFramesFromAnimation (MeshupModelPtr model, AnimationPtr animation, float time);

/** \brief Keeps transformation information for all model frames at a single keyframe 
 *
 * This struct is used to assemble the pose information for all model
 * frames in a single keyframe. It maps from the column index to the actual
 * model frame and transformation type.
 */
struct AnimationKeyPoses {
	float timestamp;
	typedef std::map<std::string, PoseInfo> FramePoseMap;
	FramePoseMap frame_poses;
	
	void clearFramePoses() {
		frame_poses.clear();	
	}
	bool setValue (int column_index, const std::vector<ColumnInfo> columns, float value, bool strict = true) {
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
			frame_poses[frame_name] = PoseInfo();
		}

		if (col_info.type == ColumnInfo::TransformTypeRotation) {
			if (col_info.axis == ColumnInfo::AxisTypeX) {
				frame_poses[frame_name].rotation_angles[0] = value;
			}
			if (col_info.axis == ColumnInfo::AxisTypeY) {
				frame_poses[frame_name].rotation_angles[1] = value;
			}
			if (col_info.axis == ColumnInfo::AxisTypeZ) {
				frame_poses[frame_name].rotation_angles[2] = value;
			}
			if (col_info.axis == ColumnInfo::AxisTypeNegativeX) {
				frame_poses[frame_name].rotation_angles[0] = -value;
			}
			if (col_info.axis == ColumnInfo::AxisTypeNegativeY) {
				frame_poses[frame_name].rotation_angles[1] = -value;
			}
			if (col_info.axis == ColumnInfo::AxisTypeNegativeZ) {
				frame_poses[frame_name].rotation_angles[2] = -value;
			}
		} else if (col_info.type == ColumnInfo::TransformTypeTranslation) {
			if (col_info.axis == ColumnInfo::AxisTypeX) {
				frame_poses[frame_name].translation[0] = value;
			}
			if (col_info.axis == ColumnInfo::AxisTypeY) {
				frame_poses[frame_name].translation[1] = value;
			}
			if (col_info.axis == ColumnInfo::AxisTypeZ) {
				frame_poses[frame_name].translation[2] = value;
			}	
			if (col_info.axis == ColumnInfo::AxisTypeNegativeX) {
				frame_poses[frame_name].translation[0] = -value;
			}
			if (col_info.axis == ColumnInfo::AxisTypeNegativeY) {
				frame_poses[frame_name].translation[1] = -value;
			}
			if (col_info.axis == ColumnInfo::AxisTypeNegativeZ) {
				frame_poses[frame_name].translation[2] = -value;
			}	
		} else if (col_info.type == ColumnInfo::TransformTypeScale) {
			if (col_info.axis == ColumnInfo::AxisTypeX) {
				frame_poses[frame_name].scaling[0] = value;
			}
			if (col_info.axis == ColumnInfo::AxisTypeY) {
				frame_poses[frame_name].scaling[1] = value;
			}
			if (col_info.axis == ColumnInfo::AxisTypeZ) {
				frame_poses[frame_name].scaling[2] = value;
			}	
			if (col_info.axis == ColumnInfo::AxisTypeNegativeX) {
				frame_poses[frame_name].scaling[0] = -value;
			}
			if (col_info.axis == ColumnInfo::AxisTypeNegativeY) {
				frame_poses[frame_name].scaling[1] = -value;
			}
			if (col_info.axis == ColumnInfo::AxisTypeNegativeZ) {
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

void Animation::addFramePose (
			const std::string &frame_name,
			const float time,
			const Vector3f &frame_translation,
			const smQuaternion &frame_rotation_quaternion,
			const Vector3f &frame_scaling
		) {
//		cerr << __func__ << " (" << frame_name 
//			<< ", " << time << ", "	<< frame_translation.transpose()
//			<< ", " << frame_rotation_quaternion.transpose() << ", "
//			<< frame_scaling.transpose() << ")" << endl;

	PoseInfo pose;
	pose.timestamp = time;
	pose.translation = configuration.axes_rotation.transpose() * frame_translation;
	pose.rotation_quaternion = frame_rotation_quaternion;
	pose.scaling = frame_scaling;

	// cerr << "frame tracks.size() = " << frame_animation_tracks.size() << endl;
	// cerr << "firstr name = " << frame_animation_tracks.begin()->first << endl;
	// cerr << "frame_tracks.keyframes.size() = " << frame_animation_tracks[frame_name].keyframes.size() << endl;
	frame_animation_tracks[frame_name].keyframes.push_back(pose);

	// update the duration of the animation
	if (time > duration)
		duration = time;
}

void Animation::updateAnimationFromRawValues () {
	RawKeyFrameList::const_iterator frame_iter = values.frames.begin();

	frame_animation_tracks.clear();

	while (frame_iter != values.frames.end()) {
		float time = frame_iter->timestamp;

		Vector3f pose_translation (0.f, 0.f, 0.f);
		smQuaternion pose_rotation;
		Vector3f pose_scale (1.f, 1.f, 1.f);
		Vector3f pose_rotation_angles;

		string current_frame_name;
		string last_frame_name;
		unsigned int ci;
		for (ci = 0.; ci < column_infos.size(); ci++) {
			if (column_infos[ci].is_time_column || column_infos[ci].is_empty)
				continue;

			float raw_value = values.getInterpolatedValue (time, ci);

			current_frame_name = column_infos[ci].frame_name;
			if (last_frame_name.size() != 0 && current_frame_name != last_frame_name) {
				// we already get the data for the next model frame so we have
				// everything for the current model frame
				addFramePose (last_frame_name, 
						time,
						pose_translation,
						pose_rotation,
						pose_scale
						);

				// reset transformations
				pose_translation.set(0.f, 0.f, 0.f);
				pose_scale.set(1.f, 1.f, 1.f);
				pose_rotation = smQuaternion();
				pose_rotation_angles.setZero();
			}

			if (column_infos[ci].type == ColumnInfo::TransformTypeTranslation) {
				switch (column_infos[ci].axis) {
					case ColumnInfo::AxisTypeX: pose_translation[0] = raw_value; break;
					case ColumnInfo::AxisTypeY: pose_translation[1] = raw_value; break;
					case ColumnInfo::AxisTypeZ: pose_translation[2] = raw_value; break;
					default: cerr << "Invalid axis!" << endl; abort();
				}
			} else if (column_infos[ci].type == ColumnInfo::TransformTypeScale) {
				switch (column_infos[ci].axis) {
					case ColumnInfo::AxisTypeX: pose_scale[0] = raw_value; break;
					case ColumnInfo::AxisTypeY: pose_scale[1] = raw_value; break;
					case ColumnInfo::AxisTypeZ: pose_scale[2] = raw_value; break;
					default: cerr << "Invalid axis!" << endl; abort();
				}
			} else if (column_infos[ci].type == ColumnInfo::TransformTypeRotation) {
				Vector3f axis_rotation;
				switch (column_infos[ci].axis) {
					case ColumnInfo::AxisTypeX: axis_rotation.set(raw_value, 0.f, 0.f); break;
					case ColumnInfo::AxisTypeY: axis_rotation.set(0.f, raw_value, 0.f); break;
					case ColumnInfo::AxisTypeZ: axis_rotation.set(0.f, 0.f, raw_value); break;
					default: cerr << "Invalid axis!" << endl; abort();
				}
				pose_rotation_angles += axis_rotation;
				pose_rotation *= configuration.convertAnglesToQuaternion (axis_rotation);
			}

			last_frame_name = current_frame_name;
		}
		frame_iter++;
	}
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
	column_infos.clear();

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
					column_infos.push_back(column_info);
					// cout << "Setting time column to " << column_index << endl;
					continue;
				}
				if (tolower(column_def) == "empty") {
					ColumnInfo column_info;
					column_info.is_empty = true;
					column_infos.push_back(column_info);
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
				ColumnInfo::TransformType type = ColumnInfo::TransformTypeUnknown;
				if (type_str == "rotation"
						|| type_str == "r")
					type = ColumnInfo::TransformTypeRotation;
				else if (type_str == "translation"
						|| type_str == "t")
					type = ColumnInfo::TransformTypeTranslation;
				else if (type_str == "scale"
						|| type_str == "s")
					type = ColumnInfo::TransformTypeScale;
				else {
					cerr << "Error: Unknown transform type '" << spec[1] << "' in " << filename << " line " << line_number << endl;
					
					if (strict)
						exit (1);

					return false;
				}

				// and the axis
				string axis_str = tolower(strip_whitespaces(spec[2]));
				ColumnInfo::AxisType axis_name;
				if (axis_str == "x")
					axis_name = ColumnInfo::AxisTypeX;
				else if (axis_str == "y")
					axis_name = ColumnInfo::AxisTypeY;
				else if (axis_str == "z")
					axis_name = ColumnInfo::AxisTypeZ;
				else if (axis_str == "-x")
					axis_name = ColumnInfo::AxisTypeNegativeX;
				else if (axis_str == "-y")
					axis_name = ColumnInfo::AxisTypeNegativeY;
				else if (axis_str == "-z")
					axis_name = ColumnInfo::AxisTypeNegativeZ;
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
				column_infos.push_back(col_info);
			}

			continue;
		}

		if (data_section) {
			// Data part:
			// columns have been read
		
			// cout << "DATA  :" << line << endl;
			// parse the DOF description and set the column info in
			// animation_keyposes
			std::vector<string> columns = tokenize (line);
			assert (columns.size() >= column_infos.size());
			std::vector<float> column_values;
			float column_time;

			// convert all values to floats and search for the time
			for (int ci = 0; ci < column_infos.size(); ci++) {
				istringstream value_stream (columns[ci]);
				float value;
				value_stream >> value;
				if (column_infos[ci].is_time_column) {
					column_time = value;
				}
			
				// handle radian
				if (column_infos[ci].type==ColumnInfo::TransformTypeRotation && column_infos[ci].is_radian) {
					value *= 180. / M_PI;
				}

				column_values.push_back(value);
			}

			// submit all values to the raw values
			for (int ci = 0; ci < column_infos.size(); ci++) {
				values.addKeyValue(column_time, ci, column_values[ci]);
			}

			continue;
		}
	}

	updateAnimationFromRawValues ();

	animation_filename = filename;

	return true;
}

void AnimationTrack::findInterpolationPoses (float time, PoseInfo &pose_start, PoseInfo &pose_end, float &fraction) {
	if (keyframes.size() == 0) {
		pose_start = PoseInfo();
		pose_end = PoseInfo();
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

void InterpolateModelFramePose (FramePtr frame, const PoseInfo &start_pose, const PoseInfo &end_pose, const float fraction) {
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
		PoseInfo start_pose, end_pose;
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
