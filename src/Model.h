/*
 * MeshUp - A visualization tool for multi-body systems based on skeletal
 * animation and magic.
 *
 * Copyright (c) 2012-2018 Martin Felis <martin.felis@iwr.uni-heidelberg.de>
 *
 * Licensed under the MIT license. See LICENSE for more details.
 */

#ifndef _MESHUPMODEL_H
#define _MESHUPMODEL_H

#include <vector>
#include <list>
#include <iostream>
#include <map>
#include <limits>

#include "SimpleMath/SimpleMath.h"
#include "SimpleMath/SimpleMathGL.h"

#include "StateDescriptor.h"
#include "FrameConfig.h"
#include "MeshVBO.h"
#include "Curve.h"

typedef MeshVBO* MeshPtr;
typedef Curve* CurvePtr;

struct Frame;
typedef Frame* FramePtr;

/** \brief Searches in various locations for the model. */
std::string find_model_file_by_name (const std::string &model_name);

struct Frame {
	Frame() :
		name (""),
		pose_translation (0.f, 0.f, 0.f),
		pose_rotation (0.f, 0.f, 0.f),
		pose_rotation_quaternion (0.f, 0.f, 0.f, 1.f),
		pose_scaling (1.f, 1.f, 1.f),
		frame_transform (Matrix44f::Identity ()),
		parent_transform (Matrix44f::Identity ()),
		pose_transform (Matrix44f::Identity ())
	{}

	std::string name;

	Vector3f pose_translation;
	Vector3f pose_rotation;
	SimpleMath::GL::Quaternion pose_rotation_quaternion;
	Vector3f pose_scaling;

	/** Transformation from base to pose */
	Matrix44f frame_transform;
	Matrix44f parent_transform;
	Matrix44f pose_transform;

	std::vector<FramePtr> children;

	/// \brief Recursively updates the pose of the Frame and its children
	void updatePoseTransform(const Matrix44f &parent_pose_transform, const FrameConfig &config);
	/** \brief Recursively updates all frames in neutral pose.
	 *
	 * As the pose information is superimposed onto the default pose we have
	 * to compute the default transformations first. This is done in this function.
	 * */
	void initDefaultFrameTransform(const Matrix44f &parent_pose_transform, const FrameConfig &config);
	void resetPoseTransform ();

	Matrix33f getFrameTransformRotation() {
		return Matrix33f (
				frame_transform(0,0), frame_transform(1,0), frame_transform(2,0),
				frame_transform(0,1), frame_transform(1,1), frame_transform(2,1),
				frame_transform(0,2), frame_transform(1,2), frame_transform(2,2)
				);
	}

	Vector3f getFrameTransformTranslation() {
		return Vector3f (frame_transform(3,0), frame_transform(3,1), frame_transform (3,2));
	}

	Matrix33f getPoseTransformRotation() {
		return Matrix33f (
				pose_transform(0,0), pose_transform(1,0), pose_transform(2,0),
				pose_transform(0,1), pose_transform(1,1), pose_transform(2,1),
				pose_transform(0,2), pose_transform(1,2), pose_transform(2,2)
				);
	}

	Vector3f getPoseTransformTranslation() {
		return Vector3f (pose_transform(3,0), pose_transform(3,1), pose_transform (3,2));
	}};

struct Segment {
	Segment () :
		name ("unnamed"),
		dimensions (-1.f, -1.f, -1.f),
		scale (-1.f, -1.f, -1.f),
		meshcenter (1/0.0, 0.f, 0.f),
		translate (0.f, 0.f, 0.f),
		rotate (SimpleMath::GL::Quaternion::fromGLRotate (0.f, 1.f, 0.f, 0.f)),
		gl_matrix (Matrix44f::Identity(4,4)),
		frame (FramePtr()),
		mesh_filename("")
	{}

	std::string name;

	Vector3f dimensions;
	Vector3f scale;
	Vector3f color;
	MeshPtr mesh;
	Vector3f meshcenter;
	Vector3f translate;
	SimpleMath::GL::Quaternion rotate;
	Matrix44f gl_matrix;
	FramePtr frame;
	std::string mesh_filename;
};

struct Point {
	Point() :
		pointIndex(0),
		frameId(0),
		name ("unnamed"),
		parentBody ("unnamed"),
		frame (FramePtr()),
		coordinates (0.f, 0.f, 0.f),
		color (1.f, 0.f, 0.f),
		draw_line (false),
		line_width (1.f)
	{}

	int pointIndex;
	std::string name;
	std::string parentBody;
	int frameId;
	FramePtr frame;
	Vector3f coordinates;
	Vector3f color;
	bool draw_line;
	float line_width;
};

struct MeshupModel {
	MeshupModel():
		model_filename (""),
		frames_initialized(false),
		skip_vbo_generation(false)
	{
		// create the BASE frame
		FramePtr base_frame (new (Frame));
		base_frame->name = "ROOT";
		base_frame->parent_transform = Matrix44f::Identity();

		frames.push_back (base_frame);
		framemap["ROOT"] = base_frame;
	}
	MeshupModel (const MeshupModel& other) {
		model_filename = other.model_filename;

		segments = other.segments;
		meshmap = other.meshmap;

		frames = other.frames;
		framemap = other.framemap;

		curvemap = other.curvemap;
		points = other.points;

		configuration = other.configuration;
		frames_initialized = other.frames_initialized;

		state_descriptor = other.state_descriptor;
	}

	MeshupModel& operator= (const MeshupModel& other) {
		if (&other != this) {
			model_filename = other.model_filename;

			segments = other.segments;
			meshmap = other.meshmap;

			frames = other.frames;
			framemap = other.framemap;

			curvemap = other.curvemap;
			points = other.points;

			configuration = other.configuration;
			frames_initialized = other.frames_initialized;
	
			state_descriptor = other.state_descriptor;
		}
		return *this;
	}

	std::string model_filename;

	typedef std::list<Segment> SegmentList;
	SegmentList segments;
	typedef std::map<std::string, MeshPtr> MeshMap;
	MeshMap meshmap;
	typedef std::vector<FramePtr> FrameVector;
	FrameVector frames;
	typedef std::map<std::string, FramePtr> FrameMap;
	FrameMap framemap;
	typedef std::map<std::string, CurvePtr> CurveMap;
	CurveMap curvemap;
	typedef std::vector<Point> PointVector;
	PointVector points;

	/// Configuration how transformations are defined
	FrameConfig configuration;
	/// Maps individual dofs to transformations
	StateDescriptor state_descriptor;

	/// Marks whether the frame transformations have to be initialized
	bool frames_initialized;

	/// Skips vbo generation when adding segments (useful when no OpenGL
	// available)
	bool skip_vbo_generation;
	
	void addFrame (
			const std::string &parent_frame_name,
			const std::string &frame_name,
			const Matrix44f &parent_transform);

	void addSegment (
			const std::string &frame_name,
			const MeshPtr mesh,
			const Vector3f &dimensions,
			const Vector3f &color,
			const Vector3f &translate,
			const SimpleMath::GL::Quaternion &rotate,
			const Vector3f &scale,
			const Vector3f &mesh_center);

	void addCurvePoint (
			const std::string &curve_name,
			const Vector3f &coords,
			const Vector3f &color
			);

	void addPoint (
			const std::string &name,
			const std::string &frame_name,
			const Vector3f &coords,
			const Vector3f &color,
			const bool draw_line,
			const float line_width = 1.f
			);

	// resets all poses to identity, i.e. the neutral pose
	void resetPoses();
	// applies pose transformations to all frames
	void updateFrames();
	// applies frame transformations to the segments
	void updateSegments();

	FramePtr findFrame (const char* frame_name) {
		FrameMap::iterator frame_iter = framemap.find (frame_name);

		if (frame_iter == framemap.end()) {
			std::cerr << "Error: Could not find frame '" << frame_name << "'!" << std::endl;
			return FramePtr();
		}

		return frame_iter->second;
	}

	bool frameExists (const char* frame_name) {
		FrameMap::iterator frame_iter = framemap.find (frame_name);

		if (frame_iter == framemap.end())
			return false;

		return true;
	}

	unsigned int getPointIndex (const char* point_name) {
		for (unsigned int i = 0; i < points.size(); i++) {
			if (points[i].name == point_name)
				return i;
		}

		return std::numeric_limits<unsigned int>::max();
	}

	bool pointExists (const char* point_name) {
		for (unsigned int i = 0; i < points.size(); i++) {
			if (points[i].name == point_name)
				return true;
		}

		return false;
	}

	void clearCurves() {
		curvemap.clear();
	}

	void clear() {
		segments.clear();
		frames.clear();
		framemap.clear();
		meshmap.clear();
		clearCurves();
		state_descriptor.clear();
	
		*this = MeshupModel();
	}

	/// Initializes the fixed frame transformations and sets frames_initialized to true
	void initDefaultFrameTransform();

	void draw();
	void drawFrameAxes();
	void drawBaseFrameAxes();
	void drawCurves();
	void drawPoints();

	bool loadModelFromFile (const char* filename, bool strict = true);
	void saveModelToFile (const char* filename);

	bool loadModelFromLuaFile (const char* filename, bool strict = true);
	
	void saveModelToLuaFile (const char* filename);
};

typedef MeshupModel* MeshupModelPtr;

#endif
