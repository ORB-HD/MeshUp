#include <UnitTest++.h>

#include "ModelData.h"

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
