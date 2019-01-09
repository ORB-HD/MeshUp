/*
 * MeshUp - A visualization tool for multi-body systems based on skeletal
 * animation and magic.
 *
 * Copyright (c) 2011-2018 Martin Felis <martin.felis@iwr.uni-heidelberg.de>
 *
 * Licensed under the MIT license. See LICENSE for more details.
 */


#include "GL/glew.h"
#include <QtGui>
#include "MeshupApp.h"

#ifdef __APPLE__
#include <QtOpenGL/QGLFrameBufferObjectFormat>
#endif

#include <QDebug>

#include <algorithm>
#include <iostream>
#include <cmath>

#include <sys/time.h>
#include <ctime>

#include <assert.h>
#include "glwidget.h"

#ifdef __APPLE__
#include <OpenGL/glu.h>
#include <OpenGL/glext.h>
#else
#include <GL/glu.h>
#endif

#include "timer.h"
#include "Animation.h"
#include "Scene.h"

using namespace std;

static bool update_simulation = false;

TimerInfo timer_info;
double draw_time = 0.;
int draw_count = 0;

Matrix44f camera_projection_matrix (Matrix44f::Identity());
Matrix44f camera_view_matrix (Matrix44f::Identity());
Matrix44f light_projection_matrix (Matrix44f::Identity());
Matrix44f light_view_matrix (Matrix44f::Identity());
const int shadow_map_size = 512;
GLuint shadow_map_texture_id = 0;

Vector4f light_ka (0.2f, 0.2f, 0.2f, 1.0f);
Vector4f light_kd (0.7f, 0.7f, 0.7f, 1.0f);
Vector4f light_ks (1.0f, 1.0f, 1.0f, 1.0f);

GLWidget::GLWidget(QWidget *parent)
    : QGLWidget(parent),
		scene (NULL),
		draw_base_axes (false),
		draw_frame_axes (false),
		draw_grid (false),
		draw_floor (true),
		draw_meshes (true),
		draw_shadows (false),
		draw_curves (false),
		draw_points (true),
		draw_forces(true),
		draw_torques(true),
		white_mode (true)
{
	cam = new Camera();
	cam->width = width();
	cam->height = height();
	camera = &cam;
	(*camera)->poi.set (0.f, 1.f, 0.f);
	(*camera)->eye.set (6.f, 3.f, 6.f);

	(*camera)->updateSphericalCoordinates();

	delta_time_sec = -1.;
	white_mode = false;

	setFocusPolicy(Qt::StrongFocus);
	setMouseTracking(true);
}

GLWidget::~GLWidget() {
	cerr << "DESTRUCTOR: drawing time: " << draw_time << "(s) count: " << draw_count << " ~" << draw_time / draw_count << "(s) per draw" << endl;

	makeCurrent();
}

void GLWidget::actionRenderImage () {
}

void GLWidget::actionRenderSeriesImage () {
}

QImage GLWidget::renderContentOffscreen (int image_width, int image_height, bool use_alpha) {
	makeCurrent();

	// set up the actual format (alpha channel, depth buffer)
	QOpenGLFramebufferObjectFormat buffer_format;
	if (use_alpha) {
		buffer_format.setInternalTextureFormat(GL_RGBA);
		if (white_mode) {
			glClearColor (1.f, 1.f, 1.f, 0.f);
		} else {
			glClearColor (0.f, 0.f, 0.f, 0.f);
		}
	}
	else
		buffer_format.setInternalTextureFormat(GL_RGB);
	buffer_format.setAttachment (QOpenGLFramebufferObject::Depth );

	// create the buffer object
	QOpenGLFramebufferObject *fb = new QOpenGLFramebufferObject(image_width, image_height, buffer_format);

	// future drawing shall be performed into this buffer
	fb->bind();

	// resize to the desired size, draw, release, and resize again to the
	// previous size

	int old_width = width();
	int old_height = height(); 

	resizeGL(image_width, image_height);
	paintGL();

	fb->release();

	//reset render parameters
	resizeGL (old_width, old_height);
	if (white_mode) {
		glClearColor (1.f, 1.f, 1.f, 1.f);
	} else {
		glClearColor (0.f, 0.f, 0.f, 1.f);
	}

	// now grab the buffer
	QImage result = fb->toImage();

	delete fb;

	return result;
}

Vector3f GLWidget::getCameraPoi() {
	return (*camera)->poi;
}

Vector3f GLWidget::getCameraEye() {
	return (*camera)->eye;
}

void GLWidget::setCameraPoi (const Vector3f values) {
	(*camera)->poi = values;
	(*camera)->updateSphericalCoordinates();
	emit camera_changed();
}

void GLWidget::setCameraEye (const Vector3f values) {
	(*camera)->eye = values;
	(*camera)->updateSphericalCoordinates();
	emit camera_changed();
}

void GLWidget::saveScreenshot (const char* filename, int width, int height, bool transparency) {
	QImage image = renderContentOffscreen (width, height, transparency);
	image.save (filename, 0, -1);
}

/****************
 * Slots
 ****************/
void GLWidget::toggle_draw_grid (bool status) {
	draw_grid = status;
}

void GLWidget::toggle_draw_base_axes (bool status) {
	draw_base_axes = status;
}

void GLWidget::toggle_draw_frame_axes (bool status) {
	draw_frame_axes = status;
}

void GLWidget::toggle_draw_floor (bool status) {
	draw_floor = status;
}

void GLWidget::toggle_draw_meshes (bool status) {
	draw_meshes = status;
}

void GLWidget::toggle_draw_shadows (bool status) {
	draw_shadows = status;
}

void GLWidget::toggle_draw_curves (bool status) {
	draw_curves = status;
}

void GLWidget::toggle_draw_points (bool status) {
	draw_points = status;
}

void GLWidget::toggle_draw_forces(bool status) {
	draw_forces = status;
}

void GLWidget::toggle_draw_torques(bool status) {
	draw_torques = status;
}

void GLWidget::toggle_draw_orthographic (bool status) {
	(*camera)->orthographic = status;

	resizeGL (windowWidth, windowHeight);
}

void GLWidget::toggle_white_mode (bool status) {
	white_mode = status;

	//qDebug() << "white mode is " << white_mode;

	if (white_mode) {
		glClearColor (1.f, 1.f, 1.f, 1.f);
	} else {
		glClearColor (0.f, 0.f, 0.f, 1.f);
	}
}

void GLWidget::set_front_view () {
	(*camera)->setFrontView();
	emit camera_changed();
}

void GLWidget::set_side_view () {
	(*camera)->setSideView();
	emit camera_changed();
}

void GLWidget::set_top_view () {
	(*camera)->setTopView();
	emit camera_changed();
}

void GLWidget::set_light_source(Vector4f pos) {
	light_position = pos;
	glLightfv (GL_LIGHT0, GL_POSITION, light_position.data());
}

void GLWidget::update_timer() {
	struct timeval clock_value;
	gettimeofday (&clock_value, NULL);

	unsigned int clock_time = clock_value.tv_sec * 1000. + clock_value.tv_usec * 1.0e-3;

	if (delta_time_sec < 0.) {
		delta_time_sec = 0.;
		application_time_msec = 0.;
		first_frame_msec = clock_time;

		return;
	}

	unsigned int last_frame_time_msec = application_time_msec;
	application_time_msec = clock_time - first_frame_msec;

	delta_time_sec = static_cast<double>(application_time_msec - last_frame_time_msec) * 1.0e-3;
}

QSize GLWidget::minimumSizeHint() const
{
    return QSize(50, 50);
}

QSize GLWidget::sizeHint() const
{
    return QSize(400, 400);
}

void GLWidget::initializeGL()
{
	GLenum err = glewInit();
	if (GLEW_OK != err) {
		cerr << "Error initializing GLEW: " << glewGetErrorString(err) << endl;
		exit(1);
	}

	qDebug() << "Using GLEW     : " << (const char*) glewGetString (GLEW_VERSION);
	qDebug() << "OpenGL Version : " << (const char*) glGetString (GL_VERSION);
	qDebug() << "GLSL Version   : " << (const char*) glGetString (GL_SHADING_LANGUAGE_VERSION);

	if (!GLEW_ARB_shadow) {
		qDebug() << "Error: ARB_shadow not supported!";
		exit (1);
	}
  if (!GLEW_ARB_depth_texture) {
		qDebug() << "Error: ARB_depth_texture not supported!";
		exit(1);
	}

	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();

	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity();

	glShadeModel (GL_SMOOTH);
	glClearColor (0.f, 0.f, 0.f, 0.f);
	glColor4f (1.f, 1.f, 1.f, 1.f);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	glClearDepth(1.0f);
	glDepthFunc(GL_LEQUAL);
	glEnable (GL_DEPTH_TEST);
	
	//Disabled, to see the flipped vertices in model
	//glEnable (GL_CULL_FACE);

	glEnable (GL_NORMALIZE);

	// initialize shadow map texture
	glGenTextures (1, &shadow_map_texture_id);
	glBindTexture (GL_TEXTURE_2D, shadow_map_texture_id);
	glTexImage2D (GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
			shadow_map_size, shadow_map_size, 
			0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	glColorMaterial (GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
	glEnable (GL_COLOR_MATERIAL);
	glMaterialfv(GL_FRONT, GL_SPECULAR, Vector4f (1.f, 1.f, 1.f, 1.f).data());
	glMaterialf(GL_FRONT, GL_SHININESS, 16.0f);

	// initialize lights
	glLightfv(GL_LIGHT0, GL_AMBIENT,  light_ka.data());
	glLightfv(GL_LIGHT0, GL_DIFFUSE,  light_kd.data());
	glLightfv(GL_LIGHT0, GL_SPECULAR, light_ks.data());

	light_position.set (-0.f, 3.f, 5.f, 1.f);
	glLightfv (GL_LIGHT0, GL_POSITION, light_position.data());

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	glEnable(GL_DEPTH_CLAMP);


	emit opengl_initialized();
}

void GLWidget::updateLightingMatrices () {
	//Calculate & save matrices
	glPushMatrix();

	glLoadIdentity();
	gluPerspective((*camera)->fov, (float)windowWidth/windowHeight, 1.0f, 100.0f);
	glGetFloatv(GL_MODELVIEW_MATRIX, camera_projection_matrix.data());

	glLoadIdentity();
	gluLookAt((*camera)->eye[0], (*camera)->eye[1], (*camera)->eye[2],
			(*camera)->poi[0], (*camera)->poi[1], (*camera)->poi[2],
			(*camera)->up[0], (*camera)->up[1], (*camera)->up[2]);
	glGetFloatv(GL_MODELVIEW_MATRIX, camera_view_matrix.data());

	glLoadIdentity();
	gluPerspective((*camera)->fov, 1.0f, 1.0f, 20.0f);
	glGetFloatv(GL_MODELVIEW_MATRIX, light_projection_matrix.data());

	glLoadIdentity();
	gluLookAt( light_position[0], light_position[1], light_position[2],
			0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f);
	glGetFloatv(GL_MODELVIEW_MATRIX, light_view_matrix.data());

	glPopMatrix();
}

void draw_checkers_board_shaded(bool white_mode) {
	float length = 16.f;
	int count = 32;
	float xmin (-length),
				xmax (length),
				xstep (fabs (xmin - xmax) / float(count)),
				zmin (-length),
				zmax (length),
				zstep (fabs (xmin -xmax) / float (count));

	float shade_start = 3.;
	float shade_width = length - shade_start;
	shade_width = 5.f;
	float m = 1.f / (shade_width);
	Vector4f clear_color;
	glGetFloatv (GL_COLOR_CLEAR_VALUE, clear_color.data());

	if (white_mode)
		clear_color.set (1.f, 1.f, 1.f, 1.f);
	else
		clear_color.set (0.f, 0.f, 0., 1.f);

	Vector4f ground_color (0.5f, 0.5f, 0.5f, 1.f);

	glDisable (GL_LIGHTING);
	glEnable(GL_DEPTH_TEST);

	glBegin (GL_QUADS);

	for (int i = 0; i < count; i++) {
		float x_shift = (i % 2) * xstep;
		for (int j = 0; j < count; j = j+2) {
			Vector3f v0 (j * xstep + xmin + x_shift, 0., i * zstep + zmin);
			Vector3f v1 (j * xstep + xmin + x_shift, 0., (i + 1) * zstep + zmin);
			Vector3f v2 ((j + 1) * xstep + xmin + x_shift, 0., (i + 1) * zstep + zmin);
			Vector3f v3 ((j + 1) * xstep + xmin + x_shift, 0., i * zstep + zmin);

			float distance = (v0 * 0.5 + v2 * 0.5).norm();
			float alpha = 1.;

			if (distance > shade_start) {
				alpha = 1. - m * (distance - shade_start);
				if (alpha < 0.f)
					alpha = 0.f;
			}

			assert (alpha >= 0.f &&  alpha <= 1.f);

			// upper left
			Vector4f color = ground_color;

			color = (1.f - alpha) * clear_color + ground_color * alpha;

		//	if (color[0] <= 0.f
		//			&& color[1] <= 0.f
		//			&& color[2] <= 0.f
		//		 )
		//		continue;

			glColor4fv (color.data());
			glVertex3fv (v0.data());
			glColor4fv (color.data());
			glVertex3fv (v1.data());
			glColor4fv (color.data());
			glVertex3fv (v2.data());
			glColor4fv (color.data());
			glVertex3fv (v3.data());
		}
	}

	glEnd();
	glEnable (GL_LIGHTING);
}

void GLWidget::drawGrid() {
	float xmin, xmax, xstep, zmin, zmax, zstep;
	int i, count;

	xmin = -16;
	xmax = 16;
	zmin = -16;
	zmax = 16;

	count = 32;

	xstep = fabs (xmin - xmax) / (float)count;
	zstep = fabs (zmin - zmax) / (float)count;

	glDisable (GL_LIGHTING);
	glColor3f (0.2f, 0.2f, 0.2f);
	glLineWidth(2.f);
	glBegin (GL_LINES);
	for (i = 0; i <= count; i++) {
		glColor3f (0.2f, 0.2f, 0.2f);
		glVertex3f (i * xstep + xmin, 0., zmin);
		glColor3f (0.2f, 0.2f, 0.2f);
		glVertex3f (i * xstep + xmin, 0., zmax);
		glColor3f (0.2f, 0.2f, 0.2f);
		glVertex3f (xmin, 0, i * zstep + zmin);
		glColor3f (0.2f, 0.2f, 0.2f);
		glVertex3f (xmax, 0, i * zstep + zmin);
	}
	glEnd ();
	glEnable (GL_LIGHTING);
}

void GLWidget::drawScene() {
	if (!scene) {
		return;
	}
	emit start_draw();

	if (draw_grid) {
		drawGrid();
	}

	if (draw_floor) {
		draw_checkers_board_shaded(white_mode);
	}

	if (draw_meshes) {
		scene->drawMeshes();
	}

	if (draw_base_axes) {
		scene->drawBaseFrameAxes();
	}
	if (draw_frame_axes) {
		scene->drawFrameAxes();
	}

	bool depth_test_enabled = glIsEnabled (GL_DEPTH_TEST);
	if (depth_test_enabled) {
		glDisable (GL_DEPTH_TEST);
	}
	glDisable (GL_LIGHTING);

	if (draw_forces) {
		scene->drawForces();
	}
	if (draw_torques) {
		scene->drawTorques();
	}
	if (draw_points) {
		scene->drawPoints();
	}
	if (draw_curves) {
		scene->drawCurves();
	}

	glEnable (GL_LIGHTING);

	if (depth_test_enabled){
		glEnable (GL_DEPTH_TEST);
	}

	/*
	if (draw_count % 100 == 0) {
		qDebug() << "drawing time: " << draw_time << "(s) count: " << draw_count << " ~" << draw_time / draw_count << "(s) per draw";
	}
	*/
}

void GLWidget::shadowMapSetupPass1 () {
	updateLightingMatrices();

	// 1st pass: from light's pont of view
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf (light_projection_matrix.data());

	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(light_view_matrix.data());

	// set viewport to shadow map size
	glViewport (0, 0, shadow_map_size, shadow_map_size);

	// draw the back faces
	glEnable (GL_CULL_FACE);
	glCullFace (GL_FRONT);

	// disable color writes and use cheap flat shading
	glShadeModel (GL_FLAT);
	glColorMask (0, 0, 0, 0);
}

void GLWidget::shadowMapSetupPass2 () {
	// read the depth buffer back into the shadow map
	glBindTexture (GL_TEXTURE_2D, shadow_map_texture_id);
	glCopyTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, 0, 0, shadow_map_size, shadow_map_size);

	// restore previous states
	glCullFace (GL_BACK);
	glShadeModel (GL_SMOOTH);
	glColorMask (1, 1, 1, 1);

	// 2nd pass: draw with dim light
	glClear (GL_DEPTH_BUFFER_BIT);
	glMatrixMode (GL_PROJECTION);
	glLoadMatrixf (camera_projection_matrix.data());

	glMatrixMode (GL_MODELVIEW);
	glLoadMatrixf (camera_view_matrix.data());

	glViewport (0, 0, windowWidth, windowHeight);

	// setup dim light
	glLightfv(GL_LIGHT0, GL_POSITION, light_position.data());
	//glLightfv(GL_LIGHT0, GL_AMBIENT,  (light_ka * 0.1f).data());
	glLightfv(GL_LIGHT0, GL_DIFFUSE,  (light_kd * 0.1f).data());
	glLightfv(GL_LIGHT0, GL_SPECULAR, Vector4f (0.f, 0.f, 0.f, 0.f).data());

	glEnable(GL_LIGHT0);	
	glEnable(GL_LIGHTING);
}

void GLWidget::shadowMapSetupPass3 () {
	// 3rd pass: draw the lighted area
	glLightfv(GL_LIGHT0, GL_DIFFUSE,  light_kd.data());
	glLightfv(GL_LIGHT0, GL_SPECULAR, Vector4f (1.f, 1.f, 1.f, 1.f).data());

	Matrix44f bias_matrix (
			0.5f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.5f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.5f, 0.0f,
			0.5f, 0.5f, 0.5f, 1.0f);
	Matrix44f texture_matrix = bias_matrix.transpose() * light_projection_matrix.transpose() * light_view_matrix.transpose();

	texture_matrix = texture_matrix;

	// setup texture coordinate generaton
	Vector4f row;
	int row_i = 0;
	row.set (
			texture_matrix(row_i,0),
			texture_matrix(row_i,1),
			texture_matrix(row_i,2),
			texture_matrix(row_i,3)
			);
	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
	glTexGenfv(GL_S, GL_EYE_PLANE, row.data());
	glEnable(GL_TEXTURE_GEN_S);

	row_i = 1;
	row.set (
			texture_matrix(row_i,0),
			texture_matrix(row_i,1),
			texture_matrix(row_i,2),
			texture_matrix(row_i,3)
			);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
	glTexGenfv(GL_T, GL_EYE_PLANE, row.data());
	glEnable(GL_TEXTURE_GEN_T);

	row_i = 2;
	row.set (
			texture_matrix(row_i,0),
			texture_matrix(row_i,1),
			texture_matrix(row_i,2),
			texture_matrix(row_i,3)
			);
	glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
	glTexGenfv(GL_R, GL_EYE_PLANE, row.data());
	glEnable(GL_TEXTURE_GEN_R);

	row_i = 3;
	row.set (
			texture_matrix(row_i,0),
			texture_matrix(row_i,1),
			texture_matrix(row_i,2),
			texture_matrix(row_i,3)
			);
	glTexGeni(GL_Q, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
	glTexGenfv(GL_Q, GL_EYE_PLANE, row.data());
	glEnable(GL_TEXTURE_GEN_Q);	

	// bind and enable shadow map texture
	glBindTexture (GL_TEXTURE_2D, shadow_map_texture_id);
	glEnable (GL_TEXTURE_2D);

	// enable shadow comparison
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE);

	// shadow map comparison should be true (i.e. not in shadow) 
	// if r <= texture
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC_ARB, GL_LEQUAL);

	// shadow comparison generates an INTENSITY result
	glTexParameteri (GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE_ARB, GL_INTENSITY);

	// set alpha test to discard false comparisons
	glAlphaFunc (GL_GEQUAL, 0.99f);
	glEnable (GL_ALPHA_TEST);
}

void GLWidget::shadowMapCleanup() {
	// reset the state
	glDisable (GL_TEXTURE_2D);

	glDisable (GL_TEXTURE_GEN_S);
	glDisable (GL_TEXTURE_GEN_T);
	glDisable (GL_TEXTURE_GEN_R);
	glDisable (GL_TEXTURE_GEN_Q);

	glDisable (GL_LIGHTING);
	glDisable (GL_ALPHA_TEST);
}

void GLWidget::paintGL() {
	update_timer();

	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity();

	(*camera)->update();

	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (draw_shadows) {
		// start the shadow mapping magic!
		shadowMapSetupPass1();
		drawScene();

		shadowMapSetupPass2();
		drawScene();

		shadowMapSetupPass3();
		drawScene();

		shadowMapCleanup();
	} else {
		glDisable (GL_CULL_FACE);
	
		glLightfv(GL_LIGHT0, GL_POSITION, light_position.data());
		glLightfv(GL_LIGHT0, GL_DIFFUSE,  light_kd.data());
		glEnable(GL_LIGHT0);	
		glEnable(GL_LIGHTING);

		drawScene();
		glDisable(GL_LIGHTING);
	}

	GLenum gl_error = glGetError();
	if (gl_error != GL_NO_ERROR) {
		cout << "OpenGL Error: " << gluErrorString(gl_error) << endl;
		abort();
	}
}

void GLWidget::resizeGL(int width, int height)
{
	//qDebug() << "resizing to" << width << "x" << height;

	if (height == 0)
		height = 1;

	if (width == 0)
		width = 1;

	glViewport (0, 0, width, height);

	windowWidth = width;
	windowHeight = height;

	(*camera)->width = width;
	(*camera)->height = height;
	emit camera_changed();
}

void GLWidget::keyPressEvent(QKeyEvent* event) {
	if (event->key() == Qt::Key_Return) {
		if (!update_simulation)
			update_simulation = true;
		else 
			update_simulation = false;
	}
}

void GLWidget::mousePressEvent(QMouseEvent *event)
{
 	lastMousePos = event->pos();
}

void GLWidget::mouseMoveEvent(QMouseEvent *event)
{
	float dx = static_cast<float>(event->x() - lastMousePos.x());
	float dy = static_cast<float>(event->y() - lastMousePos.y());

	if (event->buttons().testFlag(Qt::MiddleButton) || ( event->buttons().testFlag(Qt::LeftButton) && event->buttons().testFlag(Qt::RightButton))) {
		emit toggle_camera_fix(true);
		(*camera)->move (dx, dy);
		emit camera_changed();
	} else if (event->buttons().testFlag(Qt::LeftButton)) {
		emit toggle_camera_fix(true);
		(*camera)->rotate (dx, dy);
		emit camera_changed();
	} else if (event->buttons().testFlag(Qt::RightButton)) {
		emit toggle_camera_fix(true);
		(*camera)->zoom (dy);
		emit camera_changed();
	}

	lastMousePos = event->pos();

	updateGL();
}

