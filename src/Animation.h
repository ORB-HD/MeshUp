/*
 * MeshUp - A visualization tool for multi-body systems based on skeletal
 * animation and magic.
 *
 * Copyright (c) 2012-2018 Martin Felis <martin.felis@iwr.uni-heidelberg.de>
 *
 * Licensed under the MIT license. See LICENSE for more details.
 */

#ifndef _ANIMATION_H
#define _ANIMATION_H

#include <vector>
#include <list>
#include <iostream>
#include <map>
#include <limits>

#include "SimpleMath/SimpleMath.h"
#include "SimpleMath/SimpleMathGL.h"

#include "StateDescriptor.h"
#include "FrameConfig.h"
#include "Curve.h"

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
	SimpleMath::GL::Quaternion rotation_quaternion;
	Vector3f scaling;

	void applyStateValue (const StateInfo &column_info, float value, const FrameConfig &frame_config);
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
		loop (false),
		raw_values (std::vector<VectorNd>())
	{}

	bool loadFromFile (const char* filename, const FrameConfig &frame_config, bool strict = true);

	void getInterpolatingIndices (float time, int *frame_prev, int *frame_next, float *time_fraction);

	KeyFrame getKeyFrameAtFrameIndex (int frame_index);
	KeyFrame getKeyFrameAtTime (float time);

	std::string animation_filename;

	float current_time;
	float duration;
	bool loop;
	FrameConfig configuration;

	StateDescriptor state_descriptor;
	std::vector<VectorNd> raw_values;
};

typedef Animation* AnimationPtr;

struct MeshupModel;
typedef MeshupModel* MeshupModelPtr;

struct Frame;
typedef Frame* FramePtr;

/** \brief Updates the transformations within the model for drawing */
void UpdateModelFromAnimation (MeshupModelPtr model, AnimationPtr animation, float time);
#endif
