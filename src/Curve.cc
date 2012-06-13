#include "Curve.h"

#include "GL/glew.h"

#include <iostream>
#include <cstdlib>

using namespace std;

void Curve::generate_vbo() {
	if (points.size() != colors.size()) {
		cerr << "Error creating Curve VBO: points and colors do not have equal size!" << endl;
		abort();
	}

	if (points.size() < 2) {
		cerr << "Error creating Curve VBO: not enough points!" << endl;
		abort();
	}

	meshVBO.begin();	

	for (unsigned int i = 0; i < points.size() - 1; i++) {
		meshVBO.addVerticefv (points[i].data());
		meshVBO.addColorfv (colors[i].data());
	}

	meshVBO.end();

	meshVBO.generate_vbo();
}

void Curve::draw() {
	glLineWidth (width);
	meshVBO.draw(GL_LINE_STRIP);
}
