#include <UnitTest++.h>

#include "MeshupModel.h"
#include "Animation.h"
#include "SimpleMath/SimpleMathGL.h"

#include <iostream>

using namespace std;

const float TEST_PREC = 1.0e-6;

TEST ( RawValuesAddKeyValue ) {
	RawValues raw;

	raw.addKeyValue (0., 10, 1.23f);

	CHECK (raw.haveKeyValue(0, 10));
	CHECK_EQUAL (1.23f, raw.getKeyValue(0, 10));
}

TEST ( RawValuesAddKeyValueTwice ) {
	RawValues raw;

	raw.addKeyValue (0., 10, 1.23f);
	raw.addKeyValue (0., 10, 1.23f);

	CHECK (raw.haveKeyValue(0, 10));
	CHECK_EQUAL (1.23f, raw.getKeyValue(0,10));
}

TEST (RawValuesDeleteKeyValue) {
	RawValues raw;

	raw.addKeyValue (0., 10, 1.23);
	raw.deleteKeyValue (0., 10);

	CHECK_EQUAL (false, raw.haveKeyValue(0., 10));
}

TEST (RawValuesHaveKeyFrame) {
	RawValues raw;

	raw.addKeyValue (0., 10, 1.23);
	raw.addKeyValue (4., 11, 1.23);

	CHECK_EQUAL (true, raw.haveKeyFrame(0.));
	CHECK_EQUAL (true, raw.haveKeyFrame(4.));
	CHECK_EQUAL (false, raw.haveKeyFrame(4.001));
}

TEST (RawValuesMoveKeyFrame) {
	RawValues raw;

	raw.addKeyValue (0., 10, 1.23);
	raw.moveKeyFrame (0., 4.);

	CHECK_EQUAL (true, raw.haveKeyFrame(4.));
	CHECK_EQUAL (false, raw.haveKeyFrame(0.));
}

TEST (RawValuesMoveKeyFrameOntoExisting) {
	RawValues raw;

	raw.addKeyValue (0., 10, 1.23);
	raw.addKeyValue (4., 11, 1.25);

	raw.moveKeyFrame (0., 4.);

	CHECK_EQUAL (true, raw.haveKeyValue (4., 10));
	CHECK_EQUAL (true, raw.haveKeyValue (4., 11));
}

TEST (RawValuesMoveKeyFrameOverwriteExisting) {
	RawValues raw;

	raw.addKeyValue (0., 10, 1.11f);
	raw.addKeyValue (4., 10, 99.f);
	raw.addKeyValue (4., 11, 99.f);

	CHECK_EQUAL (99.f, raw.getKeyValue (4., 10));

	raw.moveKeyFrame (0., 4.);

	CHECK_EQUAL (1.11f, raw.getKeyValue (4., 10));
	CHECK_EQUAL (99.f, raw.getKeyValue (4., 11));
}
