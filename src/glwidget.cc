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

#include "glprimitives.h"
#include "ModelData.h"
#include "timer.h"

using namespace std;

static bool update_simulation = false;

ModelData model_data;
TimerInfo timer_info;
double draw_time = 0.;
int draw_count = 0;

GLWidget::GLWidget(QWidget *parent)
    : QGLWidget(parent)
{
	poi.setX(0.);
	poi.setY(1.0);
	poi.setZ(0.);

	eye.setX(6.);
	eye.setY(3.);
	eye.setZ(6.);

	updateSphericalCoordinates();

	/*
	qDebug () << r;
	qDebug () << theta;
	qDebug () << phi;
	*/

	delta_time_sec = -1.;

	setFocusPolicy(Qt::StrongFocus);
	setMouseTracking(true);
}

GLWidget::~GLWidget() {
	qDebug() << "DESTRUCTOR: drawing time: " << draw_time << "(s) count: " << draw_count << " ~" << draw_time / draw_count << "(s) per draw";

	makeCurrent();
	glprimitives_destroy();
}

/****************
 * Slots
 ****************/
void GLWidget::setAnimationTime (float fraction) {
	model_data.setAnimationTime(fraction * model_data.getAnimationDuration());
}

float GLWidget::getAnimationDuration() {
	return model_data.getAnimationDuration();
}

void GLWidget::toggle_draw_grid (bool status) {
	qDebug() << __func__ << " status = " << status << ": not yet implemented!";
}

void GLWidget::toggle_draw_bones (bool status) {
	qDebug() << __func__ << " status = " << status << ": not yet implemented!";
}

void GLWidget::toggle_draw_axes (bool status) {
	qDebug() << __func__ << " status = " << status << ": not yet implemented!";
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

	glClearColor (0.3, 0.3, 0.3, 1.);
	glClearDepth (1.);
	glEnable (GL_DEPTH_TEST);
	glDepthFunc (GL_LESS);
	glEnable (GL_CULL_FACE);

	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();

	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity();

	// initialize the glprimitives (cubes, etc.)
	glprimitives_init();

	// initialize lights
	GLfloat light_ka[] = { 0.2f, 0.2f, 0.2f, 1.0f};
	GLfloat light_kd[] = { 0.7f, 0.7f, 0.7f, 1.0f};
	GLfloat light_ks[] = { 1.0f, 1.0f, 1.0f, 1.0f};

	glLightfv(GL_LIGHT0, GL_AMBIENT,  light_ka);
	glLightfv(GL_LIGHT0, GL_DIFFUSE,  light_kd);
	glLightfv(GL_LIGHT0, GL_SPECULAR, light_ks);

	GLfloat light_pos[4] = {20.0f, 60.0f, 30.0f, 1.0f};
	glLightfv (GL_LIGHT0, GL_POSITION, light_pos);

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	/*
	// Fog stuff
	glEnable (GL_FOG);

	glFogi (GL_FOG_MODE, GL_LINEAR);
	glFogfv (GL_FOG_COLOR, Vector4f (0.3f, 0.3f, 0.3f, 1.f).data());
	glFogf (GL_FOG_DENSITY, 1.f);
	glHint (GL_FOG_HINT, GL_DONT_CARE);

	glFogf (GL_FOG_START, 20.0f);
	glFogf (GL_FOG_END, 25.0f);
	
	glFogi (GL_FOG_COORD_SRC, GL_FRAGMENT_DEPTH);
	*/

//	model_data.saveToFile("testmodel_backup.json");
	model_data.loadFromFile("models/samplemodel.json");
	model_data.saveToFile("testmodel.json");
	model_data.setAnimationLoop(true);

	model_data.addFramePose (
			"HIP",
			0.,
			Vector3f (0.f, 0.9f, 0.f),
			Vector3f (0.f, 0.f, 0.f),
			Vector3f (1.f, 1.f, 1.f));

	model_data.addFramePose (
			"UPPERARM_R",
			0.,
			Vector3f (0.f, 0.f, 0.f),
			Vector3f (0.f, 0.f, 0.f),
			Vector3f (1.f, 1.f, 1.f)
			);

	model_data.addFramePose (
			"UPPERARM_R",
			2.,
			Vector3f (0.f, 0.f, 0.f),
			Vector3f (0.f, 0.f, 90.f),
			Vector3f (1.f, 1.f, 1.f)
			);

	model_data.addFramePose (
			"LOWERARM_R",
			0.,
			Vector3f (0.f, 0.f, 0.f),
			Vector3f (0.f, 0.f, 0.f),
			Vector3f (1.f, 1.f, 1.f)
			);

	model_data.addFramePose (
			"LOWERARM_R",
			1.,
			Vector3f (0.f, 0.f, 0.f),
			Vector3f (0.f, 0.f, 90.f),
			Vector3f (1.f, 1.f, 1.f)
			);


}

void GLWidget::updateSphericalCoordinates() {
	QVector3D los = poi - eye;
	r = los.length();
	theta = acos (-los.y() / r);
	phi = atan (los.z() / los.x());
}

void GLWidget::updateCamera() {
	// update the camera
	float s_theta, c_theta, s_phi, c_phi;
	s_theta = sin (theta);
	c_theta = cos (theta);
	s_phi = sin (phi);
	c_phi = cos (phi);

	eye.setX(r * s_theta * c_phi);
	eye.setY(r * c_theta);
	eye.setZ(r * s_theta * s_phi);

	eye += poi;

	QVector3D right (-s_phi, 0., c_phi);

	QVector3D eye_normalized (eye);
	eye_normalized.normalize();

	up = QVector3D::crossProduct (right, eye_normalized);

	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity();

	gluLookAt (eye.x(), eye.y(), eye.z(),
			poi.x(), poi.y(), poi.z(),
			up.x(), up.y(), up.z());
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
	Vector3f bg_color (0.3f, 0.3f, 0.3f);
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
			Vector3f color (0.6f, 0.6f, 0.6f);

			color = (1.f - alpha) * bg_color + color * alpha;

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

	glColor3f (1.0, 0.6, 0.6);
	glBegin (GL_LINES);
	for (i = 0; i <= count; i++) {
		glVertex3f (i * xstep + xmin, 0., zmin);
		glVertex3f (i * xstep + xmin, 0., zmax);
		glVertex3f (xmin, 0, i * zstep + zmin);
		glVertex3f (xmax, 0, i * zstep + zmin);
	}
	glEnd ();
}

void GLWidget::paintGL() {
	update_timer();
	glClearColor (0.3, 0.3, 0.3, 1.);

	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity();

	updateCamera();

	GLfloat light_pos[4] = {20.0f, 60.0f, 30.0f, 1.0f};
	glLightfv (GL_LIGHT0, GL_POSITION, light_pos);

	glEnable (GL_COLOR_MATERIAL);
	glDisable(GL_LIGHTING);

//	drawGrid();
	draw_checkers_board_shaded();

	glEnable (GL_COLOR_MATERIAL);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	timer_start (&timer_info);

	model_data.updatePose ();
	model_data.draw();

	draw_time += timer_stop(&timer_info);
	draw_count++;

	/*
	if (draw_count % 100 == 0) {
		qDebug() << "drawing time: " << draw_time << "(s) count: " << draw_count << " ~" << draw_time / draw_count << "(s) per draw";
	}
*/

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

	float fov = 45;
	gluPerspective (fov, (GLfloat) width / (GLfloat) height, 0.005, 200);

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
	int dx = event->x() - lastMousePos.x();
	int dy = event->y() - lastMousePos.y();

	if (event->buttons().testFlag(Qt::LeftButton)) {
		// rotate
		phi += 0.01 * dx;
		theta -= 0.01 * dy;

		theta = std::max(theta, 0.01f);
		theta = std::min(theta, static_cast<float>(M_PI * 0.99));
	} else if (event->buttons().testFlag(Qt::MiddleButton)) {
		// move
		QVector3D eye_normalized (poi - eye);
		eye_normalized.normalize();

		QVector3D global_y (0., 1., 0.);
		QVector3D right = QVector3D::crossProduct (up, eye_normalized);
		poi += right * dx * 0.01 + global_y * dy * 0.01;
		eye += right * dx * 0.01 + global_y * dy * 0.01;
	} else if (event->buttons().testFlag(Qt::RightButton)) {
		// zoom
		r += 0.05 * dy;
		r = std::max (0.01f, r);
	}

	lastMousePos = event->pos();

	updateGL();
}

