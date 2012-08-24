#include "GL/glew.h"

#include <QtGui>
#include <QtOpenGL>
#include <QDebug>

#include <algorithm>
#include <iostream>
#include <cmath>

#include <sys/time.h>
#include <ctime>

#include <assert.h>
#include "glwidget.h"
#include <GL/glu.h>

#include "timer.h"
#include "Animation.h"

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
Vector4f light_position (0.f, 0.f, 0.f, 1.f);

GLWidget::GLWidget(QWidget *parent)
    : QGLWidget(parent),
		opengl_initialized (false),
		draw_base_axes (false),
		draw_frame_axes (false),
		draw_grid (false),
		draw_floor (true),
		draw_meshes (true),
		draw_shadows (false),
		draw_curves (false)
{
	poi.set (0.f, 1.f, 0.f);
	eye.set (6.f, 3.f, 6.f);

	updateSphericalCoordinates();

	/*
	qDebug () << r;
	qDebug () << theta;
	qDebug () << phi;
	*/

	delta_time_sec = -1.;

	setFocusPolicy(Qt::StrongFocus);
	setMouseTracking(true);

	model_data = MeshupModelPtr (new MeshupModel());
	animation_data = AnimationPtr (new Animation());
}

GLWidget::~GLWidget() {
	cerr << "DESTRUCTOR: drawing time: " << draw_time << "(s) count: " << draw_count << " ~" << draw_time / draw_count << "(s) per draw" << endl;

	makeCurrent();
}

void GLWidget::loadModel(const char* filename) {
	if (opengl_initialized) {
		model_data->loadModelFromFile (filename);
		emit model_loaded();
	} else {
		// mark file for later loading
		model_filename = filename;
	}
}

void GLWidget::loadAnimation(const char* filename) {
	if (opengl_initialized) {
		animation_data->loadFromFileAtFrameRate(filename, 60.f);
		emit animation_loaded();
	} else {
		// mark file for later loading
		animation_filename = filename;
	}
}

void GLWidget::setAnimationTime (float fraction) {
	animation_data->current_time = fraction * animation_data->duration;
}

void GLWidget::actionRenderImage () {
}

void GLWidget::actionRenderSeriesImage () {
}

float GLWidget::getAnimationDuration() {
	return animation_data->duration;
}

QImage GLWidget::renderContentOffscreen (int image_width, int image_height, bool use_alpha) {
	makeCurrent();

	// set up the actual format (alpha channel, depth buffer)
	QGLFramebufferObjectFormat buffer_format;
	if (use_alpha)
		buffer_format.setInternalTextureFormat(GL_RGBA);
	else
		buffer_format.setInternalTextureFormat(GL_RGB);
	buffer_format.setAttachment (QGLFramebufferObject::Depth );

	// create the buffer object
	QGLFramebufferObject *fb = new QGLFramebufferObject(image_width, image_height, buffer_format);

	// future drawing shall be performed into this buffer
	fb->bind();

	// resize to the desired size, draw, release, and resize again to the
	// previous size

	int old_width = width();
	int old_height = height(); 

	resizeGL(image_width, image_height);
	paintGL();

	fb->release();

	resizeGL (old_width, old_height);

	// now grab the buffer
	QImage result = fb->toImage();

	delete fb;

	return result;
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

	light_position.set (3.f, 6.f, 3.f, 1.f);
	glLightfv (GL_LIGHT0, GL_POSITION, light_position.data());

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	opengl_initialized = true;

	animation_data->loop = true;
}

void GLWidget::updateSphericalCoordinates() {
	Vector3f los = poi - eye;
	r = los.norm();
	theta = acos (-los[1] / r);
	phi = atan (los[2] / los[0]);
}

void GLWidget::updateCamera() {
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

	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity();

	gluLookAt (eye[0], eye[1], eye[2],
			poi[0], poi[1], poi[2],
			up[0], up[1], up[2]);
}

void GLWidget::updateLightingMatrices () {
	//Calculate & save matrices
	glPushMatrix();

	glLoadIdentity();
	gluPerspective(fov, (float)windowWidth/windowHeight, 1.0f, 100.0f);
	glGetFloatv(GL_MODELVIEW_MATRIX, camera_projection_matrix.data());

	glLoadIdentity();
	gluLookAt(eye[0], eye[1], eye[2],
			poi[0], poi[1], poi[2],
			up[0], up[1], up[2]);
	glGetFloatv(GL_MODELVIEW_MATRIX, camera_view_matrix.data());

	glLoadIdentity();
	gluPerspective(fov, 1.0f, 1.0f, 20.0f);
	glGetFloatv(GL_MODELVIEW_MATRIX, light_projection_matrix.data());

	glLoadIdentity();
	gluLookAt( light_position[0], light_position[1], light_position[2],
			0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f);
	glGetFloatv(GL_MODELVIEW_MATRIX, light_view_matrix.data());

	glPopMatrix();
}

void draw_checkers_board_shaded() {
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
	glBegin (GL_QUADS);

	for (int i = 0; i < count; i++) {
		float x_shift = (i % 2) * xstep;
		for (int j = 0; j < count; j = j+2) {
			Vector3f v0 (j * xstep + xmin + x_shift, 0., i * zstep + zmin);
			Vector3f v1 (j * xstep + xmin + x_shift, 0., (i + 1) * zstep + zmin);
			Vector3f v2 ((j + 1) * xstep + xmin + x_shift, 0., (i + 1) * zstep + zmin);
			Vector3f v3 ((j + 1) * xstep + xmin + x_shift, 0., i * zstep + zmin);

			float distance = v0.norm();
			float alpha = 1.;

			if (distance > shade_start) {
				alpha = 1. - m * (distance - shade_start);
				if (alpha < 0.f)
					alpha = 0.f;
			}

			assert (alpha >= 0.f &&  alpha <= 1.f);

			// upper left
			Vector4f color (0.6f, 0.6f, 0.6f, 1.f);

			color = (1.f - alpha) * clear_color + color * alpha;

			if (color[0] <= 0.f
					&& color[1] <= 0.f
					&& color[2] <= 0.f
				 )
				continue;

			glColor3fv (color.data());
			glVertex3fv (v0.data());
			glVertex3fv (v1.data());
			glVertex3fv (v2.data());
			glVertex3fv (v3.data());
		}
	}

	glEnd();
	
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

	glColor3f (1.f, 1.f, 1.f);
	glLineWidth(1.f);
	glBegin (GL_LINES);
	for (i = 0; i <= count; i++) {
		glVertex3f (i * xstep + xmin, 0., zmin);
		glVertex3f (i * xstep + xmin, 0., zmax);
		glVertex3f (xmin, 0, i * zstep + zmin);
		glVertex3f (xmax, 0, i * zstep + zmin);
	}
	glEnd ();
}

void GLWidget::drawScene() {
	if (draw_grid)
		drawGrid();

	if (draw_floor)
		draw_checkers_board_shaded();

	/*
	glColor3f (1.f, 1.f, 1.f);
	glBegin (GL_QUADS);
	glNormal3f (0.f, 1.f, 0.f);
	glVertex3f (-20.f, 0.f, -20.f);
	glVertex3f (-20.f, 0.f, 20.f);
	glVertex3f (20.f, 0.f, 20.f);
	glVertex3f (20.f, 0.f, -20.f);
	glEnd();
	*/

	timer_start (&timer_info);

	if (draw_meshes)
		model_data->draw();

	draw_time += timer_stop(&timer_info);
	draw_count++;

	if (draw_base_axes)
		model_data->drawBaseFrameAxes();
	if (draw_frame_axes)
		model_data->drawFrameAxes();
	if (draw_curves)
		model_data->drawCurves();

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
//	glLightfv(GL_LIGHT0, GL_AMBIENT,  (light_ka * 0.1f).data());
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

	// set alpha test to discalrd false comparisons
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
	// check whether we should reload our model
	if (model_filename.size() != 0) {
		loadModel (model_filename.c_str());

		// clear the variable to mark that we do not have to load a model
		// anymore
		model_filename = "";
	}

	// or the animation
	if (animation_filename.size() != 0) {
		loadAnimation (animation_filename.c_str());

		// clear the variable to mark that we do not have to load the animation 
		// anymore.
		animation_filename = "";
	}

	update_timer();

	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity();

	updateCamera();

	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	UpdateModelFromAnimation (model_data, animation_data, animation_data->current_time);

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
//	qDebug() << "resizing to" << width << "x" << height;

	if (height == 0)
		height = 1;

	if (width == 0)
		width = 1;

	glViewport (0, 0, width, height);

	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();

	fov = 45;
	gluPerspective (fov, (GLfloat) width / (GLfloat) height, 0.005, 200);

	windowWidth = width;
	windowHeight = height;

	glMatrixMode (GL_MODELVIEW);
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

	if (event->buttons().testFlag(Qt::LeftButton)) {
		// rotate
		phi += 0.01 * dx;
		theta -= 0.01 * dy;

		theta = std::max(theta, 0.01f);
		theta = std::min(theta, static_cast<float>(M_PI * 0.99));
#if QT_VERSION <= 0x040700
	} else if (event->buttons().testFlag(Qt::MidButton)) {
#else
	} else if (event->buttons().testFlag(Qt::MiddleButton)) {
#endif
		// move
		Vector3f eye_normalized (poi - eye);
		eye_normalized.normalize();

		Vector3f global_y (0.f, 1.f, 0.f);
		Vector3f right = up.cross (eye_normalized);
		Vector3f local_up = eye_normalized.cross(right);
		poi += right * (float)dx * 0.01f + local_up* dy * (float)0.01f;
		eye += right * (float)dx * 0.01f + local_up* dy * (float)0.01f;
	} else if (event->buttons().testFlag(Qt::RightButton)) {
		// zoom
		r += 0.05 * dy;
		r = std::max (0.01f, r);
	}

	lastMousePos = event->pos();

	updateGL();
}

