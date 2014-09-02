/*
 * MeshUp - A visualization tool for multi-body systems based on skeletal
 * animation and magic.
 *
 * Copyright (c) 2011-2012 Martin Felis <martin.felis@iwr.uni-heidelberg.de>
 *
 * Licensed under the MIT license. See LICENSE for more details.
 */

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
		meshVBO.addVertex3fv (points[i].data());
		meshVBO.addColor3fv (colors[i].data());
	}

	meshVBO.end();

	meshVBO.generate_vbo();
}

void Curve::draw() {
	// Lazy compile the VBO if not yet done
	if (meshVBO.vbo_id == 0) {
		generate_vbo();
	}

	glLineWidth (width);
	glEnable (GL_LINE_SMOOTH);
	glEnable (GL_BLEND);
	glDepthMask (GL_FALSE);
	glHint (GL_LINE_SMOOTH_HINT, GL_NICEST);

	meshVBO.draw(GL_LINE_STRIP);

	glDisable (GL_BLEND);
	glDepthMask (GL_TRUE);
}
