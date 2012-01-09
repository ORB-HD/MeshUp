#include <UnitTest++.h>

#include "ModelData.h"
#include "SimpleMathGL.h"

#include <iostream>

using namespace std;

/*
struct FileParsingFixture {
	FileParsingFixture () {
		string data = sample_header;
		stringstream data_stream (data);

		parser = ParserCreate (data_stream);
	}

	~FileParsingFixture () {
		parser.frame_index_map.clear();
	}

	Parser parser;
};

TEST_FIXTURE ( FileParsingFixture, FixtureTest ) {
	CHECK_EQUAL (true, true);
}

TEST ( TestParseJointNames ) {
}
*/

TEST ( BoneAnimationTrackInterpolationSimple ) {
	BoneAnimationTrack anim_track;

	BonePose pose;
	pose.timestamp = 0.f;

	anim_track.poses.push_back (pose);

	pose.timestamp = 1.f;
	pose.translation.set (1.f, 2.f, 3.f);
	pose.rotation_ZYXeuler.set (2.f, 4.f, 6.f);
	pose.scaling.set (2.f, 0.f, 3.f);

	anim_track.poses.push_back (pose);

	// first frame
	BonePose interpolated = anim_track.interpolatePose(0.f);
	CHECK_EQUAL (0.f, interpolated.timestamp);
	CHECK_EQUAL (Vector3f (0.f, 0.f, 0.f), interpolated.translation);
	CHECK_EQUAL (Vector3f (0.f, 0.f, 0.f), interpolated.rotation_ZYXeuler);
	CHECK_EQUAL (Vector3f (1.f, 1.f, 1.f), interpolated.scaling);

	// interpolated intermedite frame
	interpolated = anim_track.interpolatePose(0.5);
	CHECK_EQUAL (0.5f, interpolated.timestamp);
	CHECK_EQUAL (Vector3f (0.5f, 1.f, 1.5f), interpolated.translation);
	CHECK_EQUAL (Vector3f (1.f, 2.f, 3.f), interpolated.rotation_ZYXeuler);
	CHECK_EQUAL (Vector3f (1.5f, 0.5f, 2.f), interpolated.scaling);

	// last frame
	interpolated = anim_track.interpolatePose(1.);
	CHECK_EQUAL (1.f, interpolated.timestamp);
	CHECK_EQUAL (Vector3f (1.f, 2.f, 3.f), interpolated.translation);
	CHECK_EQUAL (Vector3f (2.f, 4.f, 6.f), interpolated.rotation_ZYXeuler);
	CHECK_EQUAL (Vector3f (2.f, 0.f, 3.f), interpolated.scaling);

	// over shoot (=> take last frame)
	interpolated = anim_track.interpolatePose(1.5);
	CHECK_EQUAL (1.f, interpolated.timestamp);
	CHECK_EQUAL (Vector3f (1.f, 2.f, 3.f), interpolated.translation);
	CHECK_EQUAL (Vector3f (2.f, 4.f, 6.f), interpolated.rotation_ZYXeuler);
	CHECK_EQUAL (Vector3f (2.f, 0.f, 3.f), interpolated.scaling);

	// under shoot (=> take first frame)
	interpolated = anim_track.interpolatePose(-1.);
	CHECK_EQUAL (0.f, interpolated.timestamp);
	CHECK_EQUAL (Vector3f (0.f, 0.f, 0.f), interpolated.translation);
	CHECK_EQUAL (Vector3f (0.f, 0.f, 0.f), interpolated.rotation_ZYXeuler);
	CHECK_EQUAL (Vector3f (1.f, 1.f, 1.f), interpolated.scaling);

}

TEST ( QuaternionMultiplicationTest ) {

	// X rotation
	smQuaternion quat_rot_quat (smQuaternion::fromGLRotate(90.f, 1.f, 0.f, 0.f));
	Matrix44f quat_rot_matrix = quat_rot_quat.toGLMatrix();
	CHECK_ARRAY_CLOSE(smRotate (90.f, 1.f, 0.f, 0.f).data(), quat_rot_matrix.data(), 16, 1.0e-6);

	// Y rotation
	quat_rot_quat = smQuaternion::fromGLRotate(90.f, 0.f, 1.f, 0.f);
	quat_rot_matrix = quat_rot_quat.toGLMatrix();
	CHECK_ARRAY_CLOSE(smRotate (90.f, 0.f, 1.f, 0.f).data(), quat_rot_matrix.data(), 16, 1.0e-6);

	// Z rotation
	quat_rot_quat = smQuaternion::fromGLRotate(90.f, 0.f, 0.f, 1.f);
	quat_rot_matrix = quat_rot_quat.toGLMatrix();
	CHECK_ARRAY_CLOSE(smRotate (90.f, 0.f, 0.f, 1.f).data(), quat_rot_matrix.data(), 16, 1.0e-6);

	// concatenation
	quat_rot_quat = smQuaternion::fromGLRotate (10.f, 1.f, 0.f, 0.f) 
		* smQuaternion::fromGLRotate (-30.f, 0.f, 1.f, 0.f) 
		* smQuaternion::fromGLRotate (200.f, 0.f, 0.f, 1.f);
	quat_rot_matrix = quat_rot_quat.toGLMatrix();

	Matrix44f rot_matrix = smRotate (10.f, 1.f, 0.f, 0.f)
		* smRotate (-30.f, 0.f, 1.f, 0.f)
		* smRotate (200.f, 0.f, 0.f, 1.f);

	cout << "quat: " << endl << (
			smQuaternion::fromGLRotate (10.f, 1.f, 0.f, 0.f) 
			* smQuaternion::fromGLRotate (-30.f, 0.f, 1.f, 0.f)
			).toGLMatrix() << endl;
	
	cout << "matr: " << endl << 
		smRotate (10.f, 1.f, 0.f, 0.f) 
		* smRotate (-30.f, 0.f, 1.f, 0.f) 
		<< endl;

	CHECK_ARRAY_CLOSE (rot_matrix.data(), quat_rot_matrix.data(), 16, 1.0e-5);

	/*
	cout << q3.transpose() << endl;	

	cout << q1.toGLMatrix() << endl;

	cout << qgl.toGLMatrix() << endl;

	cout << smRotate (90.f, 1.f, 0.f, 0.f) << endl;
	*/

}
