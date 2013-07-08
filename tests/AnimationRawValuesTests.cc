#include <UnitTest++.h>

#include "Model.h"
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

	CHECK (raw.haveKeyValue(0., 10));
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

TEST (RawValuesGetPrevKeyFrameTime) {
	RawValues raw;

	raw.addKeyValue (1., 10, 1.11f);

	CHECK_EQUAL (1., raw.getPrevKeyFrameTime (1.1));
}

TEST (RawValuesGetPrevKeyFrameTimeBeforeFirstKeyFrame) {
	RawValues raw;

	raw.addKeyValue (1., 10, 1.11f);
	raw.addKeyValue (4., 10, 99.f);

	CHECK_EQUAL (1., raw.getPrevKeyFrameTime (0.1));
}

TEST (RawValuesGetNextKeyFrameTime) {
	RawValues raw;

	raw.addKeyValue (1., 10, 1.11f);
	raw.addKeyValue (4., 10, 99.f);

	CHECK_EQUAL (4., raw.getNextKeyFrameTime (1.1f));
	CHECK_EQUAL (1., raw.getNextKeyFrameTime (0.1f));
}

TEST (RawValuesGetNextKeyFrameTimeAfterLastKeyFrame) {
	RawValues raw;

	raw.addKeyValue (1., 10, 1.11f);
	raw.addKeyValue (4., 10, 99.f);

	CHECK_EQUAL (4., raw.getNextKeyFrameTime (4.1f));
}

TEST (RawValuesGetNextKeyValue) {
	RawValues raw;

	raw.addKeyValue (1., 10, 1.11f);
	raw.addKeyValue (4., 10, 99.f);

	float time; float value;
	
	value = raw.getNextKeyValue (0.1f, 10, &time);
	CHECK_EQUAL (1.11f, value);
	CHECK_EQUAL (1.f, time);

	value = raw.getNextKeyValue (1.1f, 10, &time);
	CHECK_EQUAL (99.f, value);
	CHECK_EQUAL (4.f, time);

	value = raw.getNextKeyValue (5.1f, 10, &time);
	CHECK_EQUAL (99.f, value);
	CHECK_EQUAL (4.f, time);

	value = raw.getNextKeyValue (5.1f, 11, &time);
	CHECK_EQUAL (0.f, value);
	CHECK_EQUAL (0.f, time);
}

TEST (RawValuesGetPrevKeyValue) {
	RawValues raw;

	raw.addKeyValue (1., 10, 1.11f);
	raw.addKeyValue (4., 10, 99.f);

	float time; float value;
	
	value = raw.getPrevKeyValue (0.1f, 10, &time);
	CHECK_EQUAL (1.11f, value);
	CHECK_EQUAL (1.f, time);

 	value = raw.getPrevKeyValue (1.1f, 10, &time);
 	CHECK_EQUAL (1.11f, value);
 	CHECK_EQUAL (1.f, time);
 
 	value = raw.getPrevKeyValue (5.1f, 10, &time);
 	CHECK_EQUAL (99.f, value);
 	CHECK_EQUAL (4.f, time);

	value = raw.getPrevKeyValue (5.1f, 11 , &time);
 	CHECK_EQUAL (0.f, value);
 	CHECK_EQUAL (0.f, time);
}

TEST (RawValuesGetPrevNextKeyValueSingleFrame) {
	RawValues raw;

	raw.addKeyValue (4., 10, 99.f);

	float time; float value;

	value = raw.getPrevKeyValue (1.1f, 10, &time);
 	CHECK_EQUAL (99.f, value);
 	CHECK_EQUAL (4.f, time);
	
 	value = raw.getPrevKeyValue (5.1f, 10, &time);
 	CHECK_EQUAL (99.f, value);
 	CHECK_EQUAL (4.f, time);
	
	value = raw.getNextKeyValue (1.1f, 10, &time);
 	CHECK_EQUAL (99.f, value);
 	CHECK_EQUAL (4.f, time);
	
 	value = raw.getNextKeyValue (5.1f, 10, &time);
 	CHECK_EQUAL (99.f, value);
 	CHECK_EQUAL (4.f, time);
}

TEST (RawValuesGetInterpolatedValue) {
	RawValues raw;

	raw.addKeyValue (0., 10, 1.f);
	raw.addKeyValue (2., 10, 3.f);

	CHECK_EQUAL (1.f, raw.getInterpolatedValue (0., 10));
	CHECK_EQUAL (2.f, raw.getInterpolatedValue (1., 10));
	CHECK_EQUAL (3.f, raw.getInterpolatedValue (2., 10));
}
