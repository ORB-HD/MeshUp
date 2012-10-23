/*
 * MeshUp - A visualization tool for multi-body systems based on skeletal
 * animation and magic.
 *
 * Copyright (c) 2012 Martin Felis <martin.felis@iwr.uni-heidelberg.de>
 *
 * Licensed under the MIT license. See LICENSE for more details.
 */

#ifndef _ANIMATION_H
#define _ANIMATION_H

#include <vector>
#include <list>
#include <iostream>
#include <map>
#include <boost/shared_ptr.hpp>
#include <limits>

#include "SimpleMath/SimpleMath.h"
#include "SimpleMath/SimpleMathGL.h"

#include "FrameConfig.h"
#include "Curve.h"

/** \brief Description of the description of a column section entry of the
 * animation file.
 *
 * This data structure is also used to convert between raw Degree Of
 * Freedom (DOF) vectors to the actual frame transformation information.
 */
struct ColumnInfo {
	ColumnInfo() :
		frame_name (""),
		type (TransformTypeUnknown),
		axis (AxisTypeUnknown),
		is_time_column (false),
		is_empty (false),
		is_radian (false)
	{}
	enum TransformType {
		TransformTypeUnknown = 0,
		TransformTypeRotation,
		TransformTypeTranslation,
		TransformTypeScale,
		TransformTypeLast
	};
	enum AxisType {
		AxisTypeUnknown = 0,
		AxisTypeX,
		AxisTypeY,
		AxisTypeZ,
		AxisTypeNegativeX,
		AxisTypeNegativeY,
		AxisTypeNegativeZ 
	};

	std::string toString();

	std::string frame_name;
	TransformType type;
	AxisType axis;

	bool is_time_column;
	bool is_empty;
	bool is_radian;
};

struct KeyValue {
	KeyValue () :
		value (0.f),
		index (std::numeric_limits<unsigned int>::max()) 
	{}
	KeyValue (float v, unsigned int i) :
		value (v),
		index (i)
	{}
	float value;
	unsigned int index;
};
typedef std::list<KeyValue> KeyValueList;

struct KeyFrame {
	float timestamp;
	KeyValueList value_list;
};
typedef std::list<KeyFrame> RawKeyFrameList;

struct RawValues {
	RawKeyFrameList frames;

	float getPrevKeyFrameTime (const float time) const;
	float getNextKeyFrameTime (const float time) const;
	RawKeyFrameList::iterator getKeyFrameIter (const float time);

	void addKeyValue (const float time, unsigned int index, float value);
	void deleteKeyValue (const float time, unsigned int index);
	float getKeyValue (const float time, unsigned int index);
	float getNextKeyValue (const float time, unsigned int index, float *frame_time = NULL) const;
	float getPrevKeyValue (const float time, unsigned int index, float *frame_time = NULL) const;
	bool haveKeyValue (const float time, unsigned int index) const;

	bool haveKeyFrame (const float time) const;
	void moveKeyFrame (const float old_time, const float new_time);
	std::vector<float> getInterpolatedValues (const float time) const;
	float getInterpolatedValue (const float time, unsigned int index) const;
};

/** \brief A single pose of a frame at a given time */
struct PoseInfo {
	PoseInfo() :
		timestamp (0.),
		translation (0.f, 0.f, 0.f),
		rotation_angles (0.f, 0.f, 0.f),
		rotation_quaternion (0.f, 0.f, 0.f, 1.f),
		scaling (1.f, 1.f, 1.f)
	{}

	float timestamp;
	Vector3f translation;
	Vector3f rotation_angles;
	smQuaternion rotation_quaternion;
	Vector3f scaling;
};
typedef std::list<PoseInfo> KeyFrameList;

/** \brief Map key is the model frame name, value is the animation track */
struct AnimationTrack {
	KeyFrameList keyframes;

	void findInterpolationPoses (float time, PoseInfo &pose_start, PoseInfo &pose_end, float &fraction);
};
typedef std::map<std::string, AnimationTrack> AnimationTrackMap;

struct Animation {
	Animation() :
		name (""),
		animation_filename(""),
		current_time (0.f),
		duration (0.f),
		loop (false)
	{}

	/** \brief Adds a keyframe for a single frame
	 *
	 * \note Keyframes must be specified in order, i.e. when two frames f1, f2
	 * with f1.timestamp < f2.timestamp are to be added, f1 must be added
	 * before f2.
	 */
	void addFramePose (
			const std::string &frame_name,
			const float time,
			const Vector3f &frame_translation,
			const smQuaternion &frame_rotation_quaternion,
			const Vector3f &frame_scaling
			);
	void updateAnimationFromRawValues ();

	bool loadFromFile (const char* filename, bool strict = true);
	bool loadFromFileAtFrameRate (const char* filename, float frames_per_second, bool strict = true);
	bool saveToFile (const char* filename);

	std::string name;
	std::string animation_filename;

	float current_time;
	float duration;
	bool loop;
	FrameConfig configuration;

	RawValues values;

	std::vector<ColumnInfo> column_infos;
	AnimationTrackMap frame_animation_tracks;
};

typedef boost::shared_ptr<Animation> AnimationPtr;

struct MeshupModel;
typedef boost::shared_ptr<MeshupModel> MeshupModelPtr;

struct Frame;
typedef boost::shared_ptr<Frame> FramePtr;

/** \brief Performs the interpolation by filling frame->pose_<> values */
void InterpolateModelFramePose (
		FramePtr frame,
		const PoseInfo &start_pose,
		const PoseInfo &end_pose, const float fraction
		);

/** \brief Searches for the proper animation interpolants and updates the
 * poses */
void InterpolateModelFramesFromAnimation (MeshupModelPtr model, AnimationPtr animation, float time);

/** \brief Updates the transformations within the model for drawing */
void UpdateModelFromAnimation (MeshupModelPtr model, AnimationPtr animation, float time);
#endif
