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

/** \brief Searches in various locations for the model. */
std::string find_model_file_by_name (const std::string &model_name);

/** \brief A single pose of a frame at a given time */
struct FramePoseInfo {
	FramePoseInfo() :
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
typedef std::list<FramePoseInfo> KeyFrameList;

/** \brief Map key is the model frame name, value is the animation track */
struct AnimationTrack {
	KeyFrameList keyframes;

	void findInterpolationPoses (float time, FramePoseInfo &pose_start, FramePoseInfo &pose_end, float &fraction);
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
		base_frame->name = "ROOT";
		framemap["ROOT"] = base_frame;

	/** \brief Adds a keyframe for a single frame
	 *
	 * \note Keyframes must be specified in order, i.e. when two frames f1, f2
	 * with f1.timestamp < f2.timestamp are to be added, f1 must be added
	 * before f2.
	 */
	void addFramePose (
			const std::string &frame_name,
			float time,
			const Vector3f &frame_translation,
			const Vector3f &frame_rotation_angles,
			const Vector3f &frame_scaling
			);

	bool loadFromFile (const char* filename, bool strict = true);

	std::string name;
	std::string animation_filename;

	float current_time;
	float duration;
	bool loop;
	FrameConfig configuration;

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
		const FramePoseInfo &start_pose,
		const FramePoseInfo &end_pose, const float fraction
		);

/** \brief Searches for the proper animation interpolants and updates the
 * poses */
void InterpolateModelFramesFromAnimation (MeshupModelPtr model, AnimationPtr animation, float time);

/** \brief Updates the transformations within the model for drawing */
void UpdateModelFromAnimation (MeshupModelPtr model, AnimationPtr animation, float time);
#endif
