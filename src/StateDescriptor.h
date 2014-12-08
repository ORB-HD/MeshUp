/*
 * MeshUp - A visualization tool for multi-body systems based on skeletal
 * animation and magic.
 *
 * Copyright (c) 2012 Martin Felis <martin.felis@iwr.uni-heidelberg.de>
 *
 * Licensed under the MIT license. See LICENSE for more details.
 */

#ifndef STATE_DESCRIPTOR_H
#define STATE_DESCRIPTOR_H

#include <vector>
#include <list>
#include <iostream>
#include <map>
#include <limits>

#include "SimpleMath/SimpleMath.h"

/** \brief Description of the description of a column section entry of the
 * animation file.
 *
 * This data structure is also used to convert between raw Degree Of
 * Freedom (DOF) vectors to the actual frame transformation information.
 */
struct StateInfo {
	StateInfo() :
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

	std::string toString() {
		if (is_time_column)
			return ("time");

		char result_str[4] = { 0, 0, 0, 0};
		char *o_type = &result_str[0];
		char *o_axis = &result_str[1];
		char *o_neg = &result_str[2];

		switch (type) {
			case TransformTypeUnknown: *o_type = 'U'; break;
			case TransformTypeRotation: *o_type = 'R'; break;
			case TransformTypeTranslation: *o_type = 'T'; break;
			case TransformTypeScale: *o_type = 'S'; break;
			default: *o_type = 'E';
		}
		switch (axis) {
			case AxisTypeUnknown: *o_axis = 'U'; break;
			case AxisTypeX: *o_axis = 'X'; break;
			case AxisTypeY: *o_axis = 'Y'; break;
			case AxisTypeZ: *o_axis = 'Z'; break;
			case AxisTypeNegativeX: *o_axis = 'X'; *o_neg = '-'; break;
			case AxisTypeNegativeY: *o_axis = 'Y'; *o_neg = '-'; break;
			case AxisTypeNegativeZ: *o_axis = 'Z'; *o_neg = '-'; break;
		}

		return std::string (result_str);
	}

	std::string frame_name;
	TransformType type;
	AxisType axis;

	bool is_time_column;
	bool is_empty;
	bool is_radian;
};

struct StateDescriptor {
	std::vector<StateInfo> states;

	void clear() {
		states.clear();
	}
};

#endif
