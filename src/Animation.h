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

/** \brief A single pose of a frame at a given time */
struct TransformInfo {
	TransformInfo() :
		timestamp (0.),
		translation (0.f, 0.f, 0.f),
		rotation_quaternion (0.f, 0.f, 0.f, 1.f),
		scaling (1.f, 1.f, 1.f)
	{}

	float timestamp;
	Vector3f translation;
	smQuaternion rotation_quaternion;
	Vector3f scaling;

	void applyColumnValue (const ColumnInfo &column_info, float value, const FrameConfig &frame_config);
};

/** \brief Contains for all frames the transformations at a single keyframe
 */
struct KeyFrame {
	float timestamp;
	std::map<std::string, TransformInfo> transformations;
};

struct Animation {
	Animation() :
		animation_filename(""),
		current_time (0.f),
		duration (0.f),
		loop (false)
	{}

	bool loadFromFile (const char* filename, const FrameConfig &frame_config, bool strict = true);

	std::string animation_filename;

	float current_time;
	float duration;
	bool loop;
	FrameConfig configuration;

	std::vector<ColumnInfo> column_infos;
	std::vector<KeyFrame> keyframes;
};

typedef boost::shared_ptr<Animation> AnimationPtr;

struct MeshupModel;
typedef boost::shared_ptr<MeshupModel> MeshupModelPtr;

struct Frame;
typedef boost::shared_ptr<Frame> FramePtr;

/** \brief Updates the transformations within the model for drawing */
void UpdateModelFromAnimation (MeshupModelPtr model, AnimationPtr animation, float time);
#endif
