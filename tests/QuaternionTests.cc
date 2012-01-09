#include <UnitTest++.h>

#include "ModelData.h"
#include "SimpleMathGL.h"

#include <iostream>

using namespace std;

TEST ( QuaternionFromGLRotateTests ) {
	// X rotation
	smQuaternion quat_rot_quat (smQuaternion::fromGLRotate(44.f, 1.f, 0.f, 0.f));
	Matrix44f quat_rot_matrix = quat_rot_quat.toGLMatrix();
	CHECK_ARRAY_CLOSE(smRotate (44.f, 1.f, 0.f, 0.f).data(), quat_rot_matrix.data(), 16, 1.0e-6);

	// Y rotation
	quat_rot_quat = smQuaternion::fromGLRotate(33.f, 0.f, 1.f, 0.f);
	quat_rot_matrix = quat_rot_quat.toGLMatrix();
	CHECK_ARRAY_CLOSE(smRotate (33.f, 0.f, 1.f, 0.f).data(), quat_rot_matrix.data(), 16, 1.0e-6);

	// Z rotation
	quat_rot_quat = smQuaternion::fromGLRotate(55.f, 0.f, 0.f, 1.f);
	quat_rot_matrix = quat_rot_quat.toGLMatrix();
	CHECK_ARRAY_CLOSE(smRotate (55.f, 0.f, 0.f, 1.f).data(), quat_rot_matrix.data(), 16, 1.0e-6);
}

TEST ( QuaternionMultiplicationTest ) {
	smQuaternion quat_rot_quat;
	Matrix44f quat_rot_matrix;

	// concatenation
	quat_rot_quat = smQuaternion::fromGLRotate (10.f, 1.f, 0.f, 0.f) 
		* smQuaternion::fromGLRotate (-30.f, 0.f, 1.f, 0.f) 
		* smQuaternion::fromGLRotate (200.f, 0.f, 0.f, 1.f);
	quat_rot_matrix = quat_rot_quat.toGLMatrix();

	Matrix44f rot_matrix = smRotate (10.f, 1.f, 0.f, 0.f)
		* smRotate (-30.f, 0.f, 1.f, 0.f)
		* smRotate (200.f, 0.f, 0.f, 1.f);

	CHECK_ARRAY_CLOSE (rot_matrix.data(), quat_rot_matrix.data(), 16, 1.0e-6);
}

TEST ( QuaternionSlerpTest ) {
	smQuaternion q0 (0.f, 0.f, 0.f, 1.f);
	smQuaternion q1 = smQuaternion::fromGLRotate (100.f, 1.f, 0.f, 0.f);

	smQuaternion q_slerped = q0.slerp (0.35f, q1);

	Matrix44f rot_matrix = smRotate (35.f, 1.f, 0.f, 0.f);
	CHECK_ARRAY_CLOSE (rot_matrix.data(), q_slerped.toGLMatrix().data(), 16, 1.0e-6);
}
