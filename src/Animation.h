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
	std::string frame_name;
	TransformType type;
	AxisType axis;

	bool is_time_column;
	bool is_empty;
	bool is_radian;
};

/** \brief Value that can be either interpolated or not.
 */
struct AnimationValue {
	AnimationValue() :
		value (0.f),
		keyed (false) 
	{}

	float value;
	bool keyed;
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

/** \brief Raw keyframe data in form of a value vector.
 *
 * This format is used to store the animation keyframes as Degree of
 * Freedom (DOF) values that allow editing of keyframe data on a per joint
 * level.
 */
struct AnimationRawKeyframe {
	float timestamp;
	std::vector<AnimationValue> values;
};
typedef std::list<AnimationRawKeyframe> AnimationRawKeyframeList;

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

	float getPrevKeyFrameTime () const;
	float getNextKeyFrameTime () const;

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

	void getRawDataInterpolants (
			const float time,
			AnimationRawKeyframeList::const_iterator &prev_iter, 
			AnimationRawKeyframeList::const_iterator &next_iter,
			float &fraction
			) const;

	bool loadFromFile (const char* filename, bool strict = true);

	std::string name;
	std::string animation_filename;

	float current_time;
	float duration;
	bool loop;
	FrameConfig configuration;

	RawValues values;

	std::vector<ColumnInfo> column_infos;
	AnimationTrackMap frame_animation_tracks;
	AnimationRawKeyframeList animation_raw_keyframes;
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
