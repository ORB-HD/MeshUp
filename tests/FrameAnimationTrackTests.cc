#include <UnitTest++.h>

#include "Model.h"
#include "Animation.h"
#include "SimpleMath/SimpleMathGL.h"

#include <iostream>

using namespace std;

TEST ( FrameAnimationTrackInterpolationSimple ) {
	PoseInfo pose_start;
	PoseInfo pose_end;
	
	pose_start.timestamp = 0.f;

	pose_end.timestamp = 1.f;
	pose_end.translation.set (1.f, 2.f, 3.f);
	pose_end.scaling.set (2.f, 0.f, 3.f);

	FramePtr frame (new Frame());

	InterpolateModelFramePose (frame, pose_start, pose_end, 0.f);
	// first frame
	CHECK_EQUAL (Vector3f (0.f, 0.f, 0.f), frame->pose_translation);
	CHECK_EQUAL (Vector3f (1.f, 1.f, 1.f), frame->pose_scaling);

	// interpolated intermedite frame
	InterpolateModelFramePose (frame, pose_start, pose_end, 0.5f);
	CHECK_EQUAL (Vector3f (0.5f, 1.f, 1.5f), frame->pose_translation);
	CHECK_EQUAL (Vector3f (1.5f, 0.5f, 2.f), frame->pose_scaling);

	// last frame
	InterpolateModelFramePose (frame, pose_start, pose_end, 1.f);
	CHECK_EQUAL (Vector3f (1.f, 2.f, 3.f), frame->pose_translation);
	CHECK_EQUAL (Vector3f (2.f, 0.f, 3.f), frame->pose_scaling);
}
