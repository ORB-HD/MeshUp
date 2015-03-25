/*
 * QtGLAppBase - Simple Qt Application to get started with OpenGL stuff.
 *
 * Copyright (c) 2011-2012 Martin Felis <martin.felis@iwr.uni-heidelberg.de>
 *
 * Licensed under the MIT license. See LICENSE for more details.
 */

#ifndef _CAMERA_H
#define _CAMERA_H

#include <SimpleMath/SimpleMath.h>

/**
 * \warning This class is hacked such that it uses the following
 * coordinate system: X forward, Y left, Z up. However the vectors in poi,
 * eye, up use the standard OpenGL coordinate system: X forward, Y up, Z
 * right. (Note: left and right are here seen from a person standing at
 * the origin).
 */
struct Camera {
	Vector3f poi;
	Vector3f eye;
	Vector3f up;

	int width;
	int height;

	float phi;
	float theta;
	float r;
	float fov;
	float near;
	float far;

	bool orthographic;

	Camera();
	void update();
	void updateSphericalCoordinates();

	void setFrontView();
	void setSideView();
	void setTopView();
	void setSize (int width_, int height_);

	void move (float screen_x, float screen_y);
	void zoom (float screen_y);
	void rotate (float screen_x, float screen_y);

	Matrix44f getProjectionMatrix() const;
	Matrix44f getViewMatrix() const;
};

/* _CAMERA_H */
#endif
