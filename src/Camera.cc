/*
 * QtGLAppBase - Simple Qt Application to get started with OpenGL stuff.
 *
 * Copyright (c) 2011-2012 Martin Felis <martin.felis@iwr.uni-heidelberg.de>
 *
 * Licensed under the MIT license. See LICENSE for more details.
 */

#include "GL/glew.h"
#include <GL/glu.h>

#include "Camera.h"
#include "SimpleMath/SimpleMathGL.h"

#include <iostream>

using namespace std;

Camera::Camera() :
	poi(Vector3f(0.f, 1.f, 0.f)),
	eye(Vector3f(6.f, 3.f, 6.f)),
	up (Vector3f (0.f, 1.f, 0.f)),
	phi (0.0f),
	theta (0.0f),
	r (3.0f),
	fov (45.0f),
	near (0.1f),
	far (30.),
	orthographic(false)
	{
	updateSphericalCoordinates();
}

void Camera::update() {
	// update the camera
	float s_theta, c_theta, s_phi, c_phi;
	s_theta = sin (theta);
	c_theta = cos (theta);
	s_phi = sin (phi);
	c_phi = cos (phi);

	eye[0] = (r * s_theta * c_phi);
	eye[1] = (r * c_theta);
	eye[2] = (r * s_theta * s_phi);

	eye += poi;

	if (eye[1] < 0.)
		eye[1];

	Vector3f right (-s_phi, 0., c_phi);

	Vector3f eye_normalized (eye);
	eye_normalized.normalize();

	up = right.cross (eye_normalized);

	// setup of the projection
	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();

	fov = 45;
	GLfloat aspect = (GLfloat) width / (GLfloat) height;

	if (orthographic) {
		GLfloat w = r * width * 0.001;
		GLfloat h = w / aspect; 

		glOrtho (- w * 0.5, w * 0.5,
				-h * 0.5, h * 0.5,
			0.01, 100);

		// setup of the modelview matrix
		glMatrixMode (GL_MODELVIEW);
		glLoadIdentity();

		Vector3f temp_eye = eye - (poi - eye) * 5.;

		gluLookAt (temp_eye[0], temp_eye[1], temp_eye[2],
				poi[0], poi[1], poi[2],
				up[0], up[1], up[2]);
	} else {
		gluPerspective (fov, aspect, 0.001, 50);
		
		// setup of the modelview matrix
		glMatrixMode (GL_MODELVIEW);
		glLoadIdentity();
		
		gluLookAt (eye[0], eye[1], eye[2],
				poi[0], poi[1], poi[2],
				up[0], up[1], up[2]);
	}
}

void Camera::updateSphericalCoordinates() {
	Vector3f los = poi - eye;
	r = los.norm();
	theta = acos (-los[1] / r);
	phi = atan (los[2] / los[0]);

	// atan only returns +- pi/2 so we have to fix the azimuth here
	if (los[0] > 0.f) {
		if (los[2] >= 0.f)
			phi += M_PI;
		else
			phi -= M_PI;
	}

}

void Camera::setFrontView () {
	if (fabs(phi) > 1.0e-5) {
		// front
		theta = 90. * M_PI / 180.;
		phi = 0.;
	}	else {
		// back
		theta = 90. * M_PI / 180.;
		phi = 180 * M_PI / 180.;
	}
}

void Camera::setSideView () {
	if (fabs(fabs(phi) - 90. * M_PI / 180.) > 1.0e-5 && fabs(fabs(theta) - 90. * M_PI / 180.)) {
		// right
		theta = 90. * M_PI / 180.;
		phi = 90. * M_PI / 180.;
	} else {
		// left
		theta = 90. * M_PI / 180.;
		phi = 270. * M_PI / 180.;
	}
}

void Camera::setTopView () {
	if (fabs(theta) > 1.0e-5) {
		// top
		phi = 180. * M_PI / 180.;
		theta = 0.;
	} else {
		// bottom
		phi = 0. * M_PI / 180.;
		theta = 180. * M_PI / 180.;
	}
}

void Camera::setSize (int width_, int height_) {
	width = width_;
	height = height_;
}

void Camera::move (float screen_x, float screen_y) {
		// move
		Vector3f eye_normalized (poi - eye);
		eye_normalized.normalize();

		Vector3f global_y (0.f, 1.f, 0.f);
		Vector3f right = up.cross (eye_normalized);
		Vector3f local_up = eye_normalized.cross(right);
		poi += right * (float)screen_x * 0.01f + local_up* screen_y * (float)0.01f;
		eye += right * (float)screen_x * 0.01f + local_up* screen_y * (float)0.01f;
}

void Camera::zoom (float screen_y) {
	// zoom
	r += 0.05 * screen_y;
	r = std::max (0.01f, r);
}

void Camera::rotate (float screen_x, float screen_y) {
	// rotate
	phi += 0.01 * screen_x;
	theta -= 0.01 * screen_y;

	theta = std::max(theta, 0.01f);
	theta = std::min(theta, static_cast<float>(M_PI * 0.99));
}

Matrix44f Camera::getProjectionMatrix() const {
	// setup of the projection
	glMatrixMode (GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity ();

	GLfloat aspect = (GLfloat) width / (GLfloat) height;

	if (orthographic) {
		GLfloat distance = (GLfloat) (poi - eye).norm();

		GLfloat w = tan(fov * M_PI / 180.) * 0.01 * width * distance / 2.f;
		GLfloat h = w / aspect; 

		glOrtho (- w * 0.5, w * 0.5,
				-h * 0.5, h * 0.5,
			near, far);
	} else {
		gluPerspective (fov, aspect, near, far);
	}

	Matrix44f result;
	glGetFloatv (GL_PROJECTION_MATRIX, result.data());

	glPopMatrix();
	// setup of the modelview matrix
	glMatrixMode (GL_MODELVIEW);
	
	return result;
}

Matrix44f Camera::getViewMatrix() const {
	glPushMatrix();
	glLoadIdentity();

	gluLookAt (eye[0], eye[1], eye[2],
			poi[0], poi[1], poi[2],
			up[0], up[1], up[2]);

	Matrix44f result;
	glGetFloatv (GL_MODELVIEW_MATRIX, result.data());

	glPopMatrix();

	return result;
}


