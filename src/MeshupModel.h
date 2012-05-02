#ifndef _MESHUPMODEL_H
#define _MESHUPMODEL_H

#include <vector>
#include <list>
#include <iostream>
#include <map>
#include <boost/shared_ptr.hpp>
#include <limits>

#include "SimpleMath/SimpleMath.h"
#include "SimpleMath/SimpleMathGL.h"

#include "OBJMesh.h"

/** \brief Searches in various locations for the model. */
std::string find_model_file_by_name (const std::string &model_name);

struct FrameConfig {
	FrameConfig() :
		axis_front(1.f, 0.f, 0.f),
		axis_up(0.f, 1.f, 0.f),
		axis_right (0.f, 0.f, 1.f),
		axes_rotation (Matrix33f::Identity())
	{ 
		rotation_order[0] = 2;
		rotation_order[1] = 1;
		rotation_order[2] = 0;
	}
	Vector3f axis_front;
	Vector3f axis_up;
	Vector3f axis_right;
	Matrix33f axes_rotation;

	int rotation_order[3];

	void init() {
		axes_rotation(0,0) = axis_front[0];
		axes_rotation(1,0) = axis_front[1];
		axes_rotation(2,0) = axis_front[2];

		axes_rotation(0,1) = axis_up[0];
		axes_rotation(1,1) = axis_up[1];
		axes_rotation(2,1) = axis_up[2];

		axes_rotation(0,2) = axis_right[0];
		axes_rotation(1,2) = axis_right[1];
		axes_rotation(2,2) = axis_right[2];
	}

	Matrix44f convertAnglesToMatrix (const Vector3f &rotation_angles) const {
	return	smRotate (rotation_angles[0], 1.f, 0.f, 0.f)
		* smRotate (rotation_angles[1], 0.f, 1.f, 0.f)
		* smRotate (rotation_angles[2], 0.f, 0.f, 1.f);
	};

	smQuaternion convertAnglesToQuaternion (const Vector3f &rotation_angles) const {
	int a0 = rotation_order[2];
	int a1 = rotation_order[1];
	int a2 = rotation_order[0];

	Vector3f axis_0 (
			axes_rotation(a0, 0),
			axes_rotation(a0, 1),
			axes_rotation(a0, 2)
				);

	Vector3f axis_1 (
			axes_rotation(a1, 0),
			axes_rotation(a1, 1),
			axes_rotation(a1, 2)
				);

	Vector3f axis_2 (
			axes_rotation(a2, 0),
			axes_rotation(a2, 1),
			axes_rotation(a2, 2)
				);

	return smQuaternion::fromGLRotate (
				rotation_angles[a0],
				axis_0[0], axis_0[1], axis_0[2]
				)
			* smQuaternion::fromGLRotate (
				rotation_angles[a1],
				axis_1[0], axis_1[1], axis_1[2]
				)
			* smQuaternion::fromGLRotate (
				rotation_angles[a2],
				axis_2[0], axis_2[1], axis_2[2]
				);
	}

};

typedef boost::shared_ptr<OBJMesh> MeshPtr;

struct Frame;
typedef boost::shared_ptr<Frame> FramePtr;

struct Frame {
	Frame() :
		name (""),
		parent_translation (0.f, 0.f, 0.f),
		parent_rotation (0.f, 0.f, 0.f),
		pose_translation (0.f, 0.f, 0.f),
		pose_rotation (0.f, 0.f, 0.f),
		pose_rotation_quaternion (0.f, 0.f, 0.f, 1.f),
		pose_scaling (1.f, 1.f, 1.f),
		frame_transform (Matrix44f::Identity ()),
		pose_transform (Matrix44f::Identity ())
	{}

	std::string name;
	Vector3f parent_translation;
	Vector3f parent_rotation;

	Vector3f pose_translation;
	Vector3f pose_rotation;
	smQuaternion pose_rotation_quaternion;
	Vector3f pose_scaling;

	/** Transformation from base to pose */
	Matrix44f frame_transform;
	Matrix44f pose_transform;

	std::vector<FramePtr> children;

	void updatePoseTransform(const Matrix44f &parent_pose_transform, const FrameConfig &config);
	void initFrameTransform(const Matrix44f &parent_pose_transform, const FrameConfig &config);

	Vector3f getFrameTransformTranslation() {
		return Vector3f (frame_transform(3,0), frame_transform(3,1), frame_transform (3,2));
	}
};

struct Segment {
	Segment () :
		name ("unnamed"),
		dimensions (-1.f, -1.f, -1.f),
		scale (-1.f, -1.f, -1.f),
		meshcenter (1/0.0, 0.f, 0.f),
		translate (0.f, 0.f, 0.f),
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
	Matrix44f gl_matrix;
	FramePtr frame;
	std::string mesh_filename;
};

struct FramePose {
	FramePose() :
		timestamp (-1.f),
		translation (0.f, 0.f, 0.f),
		rotation (0.f, 0.f, 0.f),
		rotation_quaternion (0.f, 0.f, 0.f, 1.f),
		scaling (1.f, 1.f, 1.f),
		endpoint (0.f, 1.f, 0.f)
	{}

	float timestamp;
	Vector3f translation;
	Vector3f rotation;
	smQuaternion rotation_quaternion;
	Vector3f scaling;
	Vector3f endpoint;
};

struct FrameAnimationTrack {
	typedef std::list<FramePose> FramePoseList;
	FramePoseList poses;

	FramePose interpolatePose (float time);
};

struct Animation {
	Animation() :
		current_time (0.f),
		duration (0.f),
		loop (false)
	{}
	std::string name;
	typedef std::map<FramePtr, FrameAnimationTrack> FrameAnimationTrackMap;
	FrameAnimationTrackMap frametracks;

	float current_time;
	float duration;
	bool loop;
};
typedef boost::shared_ptr<Animation> AnimationPtr;

struct MeshupModel {
	MeshupModel():
		model_filename (""),
		animation_filename (""),
		frames_initialized(false)
	{
		// create the BASE frame
		FramePtr base_frame (new (Frame));
		base_frame->name = "BASE";
		base_frame->parent_translation.setZero();
		base_frame->parent_rotation.setZero();

		frames.push_back (base_frame);
		framemap["BASE"] = base_frame;
	}

	MeshupModel& operator= (const MeshupModel& other) {
		if (&other != this) {
			model_filename = other.model_filename;
			animation_filename = other.animation_filename;

			segments = other.segments;
			meshmap = other.meshmap;

			frames = other.frames;
			framemap = other.framemap;

			configuration = other.configuration;
			frames_initialized = other.frames_initialized;
			animation = other.animation;

		}
		return *this;
	}

	std::string model_filename;
	std::string animation_filename;

	typedef std::list<Segment> SegmentList;
	SegmentList segments;
	typedef std::map<std::string, MeshPtr> MeshMap;
	MeshMap meshmap;
	typedef std::vector<FramePtr> FrameVector;
	FrameVector frames;
	typedef std::map<std::string, FramePtr> FrameMap;
	FrameMap framemap;
	typedef std::map<FramePtr, FrameAnimationTrack> FrameAnimationTrackMap;

	/// Configuration how transformations are defined
	FrameConfig configuration;

	/// Marks whether the frame transformations have to be initialized
	bool frames_initialized;
	
	Animation animation;

	void addFrame (
			const std::string &parent_frame_name,
			const std::string &frame_name,
			const Vector3f &parent_translation,
			const Vector3f &parent_rotation);

	void addSegment (
			const std::string &frame_name,
			const std::string &segment_name,
			const Vector3f &dimensions,
			const Vector3f &scale,
			const Vector3f &color,
			const std::string &mesh_name,
			const Vector3f &translate,
			const Vector3f &mesh_center);

	void addFramePose (
			const std::string &frame_name,
			float time,
			const Vector3f &frame_translation,
			const Vector3f &frame_rotation,
			const Vector3f &frame_scaling
			);

	FramePtr findFrame (const char* frame_name) {
		FrameMap::iterator frame_iter = framemap.find (frame_name);

		if (frame_iter == framemap.end()) {
			std::cerr << "Error: Could not find frame '" << frame_name << "'!" << std::endl;
			return FramePtr();
		}

		return frame_iter->second;
	}

	void clear() {
		segments.clear();
		frames.clear();
		framemap.clear();
		animation.frametracks.clear();

		*this = MeshupModel();
	}

	void resetAnimation() {
		animation.current_time = 0.f;
	}
	void setAnimationLoop(bool do_loop) {
		animation.loop = do_loop;
	}
	float getAnimationDuration() {
		return animation.duration;
	}
	void setAnimationTime (float time_sec) {
		animation.current_time = time_sec;
	}

	/// Initializes the fixed frame transformations and sets frames_initialized to true
	void initFrameTransform();

	/** Updates the animation state.
	 *
	 * Updates the pose information of the frames by interpolating the
	 * keyframes defined in Animation.
	 */
	void updatePose();
	/** \brief Updates the Frame transformations.
	 *
	 * Updates the full pose transformations recursively such that
	 * Frame::pose_transformation contains the full Base->Pose
	 * transformation.
	 */
	void updateFrames();
	/** \brief Updates the segment transformations (scale, translation to mesh
	 * center, etc).
	 *
	 * This function prepares all segments for the OpenGL drawer. The
	 * drawer itself then only has to loop over all segments, query the
	 * color and use Segment::gl_matrix to setup the OpenGL
	 * transformation.
	 */
	void updateSegments();

	void draw();
	void drawFrameAxes();
	void drawBaseFrameAxes();

	void saveModelToFile (const char* filename);
	bool loadModelFromFile (const char* filename, bool strict = true);
	bool loadAnimationFromFile (const char* filename, bool strict = true);
};

#endif
