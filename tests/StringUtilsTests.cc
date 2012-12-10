#include <UnitTest++.h>

#include "string_utils.h"

#include <iostream>
#include <string>
#include <vector>


using namespace std;

const float TEST_PREC = 1.0e-6;

TEST ( StringUtilsTokenizeStd ) {
	string line ("2.      0.      0.      0.      30.	30.	30.");
	
	vector<string> tokens = tokenize(line);

	CHECK_EQUAL (27, tokens.size());
}

TEST ( StringUtilsTokenizeStripWhitespaces ) {
	string line ("2.,      0.,      0.,     0.,      31.,	32.,	355");
	
	vector<string> tokens = tokenize_strip_whitespaces(line, ",\r\n");

	CHECK_EQUAL (7, tokens.size());
}


TEST ( StringUtilsCountChar ) {
	string line ("1//2");

	CHECK_EQUAL (2, count_char(line, "/"));
}

TEST ( StringUtilsTokenizeCSVStripSimple ) {
	string line ("1,1, 2,4, ");

	vector<string> tokens = tokenize_csv_strip_whitespaces (line);

	CHECK_EQUAL (2, tokens.size());

	CHECK_EQUAL ("1,1", tokens[0]);
	CHECK_EQUAL ("2,4", tokens[1]);
}

TEST ( StringUtilsTokenizeCSVStrip ) {
	string line ("1,1, 2,4,   ,521 ,\t\t923\t, , \t123,321, ");

	vector<string> tokens = tokenize_csv_strip_whitespaces (line);

	CHECK_EQUAL (6, tokens.size());

	CHECK_EQUAL ("1,1", tokens[0]);
	CHECK_EQUAL ("2,4", tokens[1]);
	CHECK_EQUAL (",521", tokens[2]);
	CHECK_EQUAL ("923", tokens[3]);
	CHECK_EQUAL ("", tokens[4]);
	CHECK_EQUAL ("123,321", tokens[5]);
}
