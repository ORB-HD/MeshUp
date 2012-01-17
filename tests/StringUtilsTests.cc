#include <UnitTest++.h>

#include "string_utils.h"

#include <iostream>
#include <string>
#include <vector>


using namespace std;

const float TEST_PREC = 1.0e-6;

TEST ( StringUtilsTokenize ) {
	string line ("2.      0.      0.      0.      30.	30.	30.");
	
	vector<string> tokens = tokenize(line);

	CHECK_EQUAL (7, tokens.size());
}

TEST ( StringUtilsCountChar ) {
	string line ("1//2");

	CHECK_EQUAL (2, count_char(line, "/"));
}
