#include <UnitTest++.h>

#include "MeshupModel.h"
#include "SimpleMath/SimpleMathGL.h"

#include <iostream>

using namespace std;

const float TEST_PREC = 1.0e-6;

TEST ( FrameTestSimple ) {
	// Test whether the frame is at the desired position
	MeshupModel model;

	Matrix44f parent_transform = model.configuration.convertAnglesToMatrix (Vector3f( 0., 0., 0.)) 
		* smTranslate (1., 2., 3.);

	model.addFrame (
			"BASE",
			"TEST_FRAME",
			parent_transform
			);

	model.updateFrames();

	FramePtr test_frame = model.findFrame ("TEST_FRAME");

	CHECK_ARRAY_CLOSE (Vector3f (1.f, 2.f, 3.f).data(),
			(test_frame->getFrameTransformTranslation()).data(),
			3,
			TEST_PREC);
}
