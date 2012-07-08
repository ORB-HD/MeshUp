#include <UnitTest++.h>

#include "MeshupModel.h"
#include "Animation.h"
#include "SimpleMath/SimpleMathGL.h"

#include <iostream>

using namespace std;

const float TEST_PREC = 1.0e-6;

TEST ( AnimationTestGetPrevNextFrameTime ) {
	Animation animation;
	Vector3f zero_vec (Vector3f::Zero());
	smQuaternion zero_quat;

	animation.addFramePose ("frame_A", 0.f, zero_vec, zero_quat, zero_vec);
	animation.addFramePose ("frame_A", 1.f, zero_vec, zero_quat, zero_vec);
	animation.addFramePose ("frame_A", 2.f, zero_vec, zero_quat, zero_vec);

	animation.addFramePose ("frame_B", 1.5f, zero_vec, zero_quat, zero_vec);

	animation.current_time = 0.f;

	CHECK_EQUAL (1.f, animation.getNextKeyFrameTime());
	
	animation.current_time = 1.1f;
	CHECK_EQUAL (1.5f, animation.getNextKeyFrameTime());

	animation.current_time = 1.1f;
	CHECK_EQUAL (1.0f, animation.getPrevKeyFrameTime());

	animation.current_time = 0.f;
	CHECK_EQUAL (0.f, animation.getPrevKeyFrameTime());

	animation.current_time = 2.f;
	CHECK_EQUAL (2.f, animation.getNextKeyFrameTime());
}
