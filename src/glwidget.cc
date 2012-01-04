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

using namespace std;

static bool update_simulation = false;

MeshData cube_mesh;

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
	makeCurrent();
	glprimitives_destroy();
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

	qDebug() << "Using GLEW: " << (const char*) glewGetString (GLEW_VERSION);

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

	// create the cube mesh
	cube_mesh.begin();

		// 1st quad "top +y"
	float normal[] = { 0.f, 1.f, 0.f };

	// 1st triangle
	cube_mesh.addNormalfv (normal);
	cube_mesh.addVertice (-0.5, 0.5f, -0.5);
	
	cube_mesh.addNormalfv (normal);
	cube_mesh.addVertice (-0.5, 0.5f, 0.5);

	cube_mesh.addNormalfv (normal);
	cube_mesh.addVertice (0.5f, 0.5f, 0.5f);

	// 2nd triangle
	cube_mesh.addNormalfv (normal);
	cube_mesh.addVertice (0.5, 0.5f, 0.5);

	cube_mesh.addNormalfv (normal);
	cube_mesh.addVertice (0.5, 0.5f, -0.5);

	cube_mesh.addNormalfv (normal);
	cube_mesh.addVertice (-0.5, 0.5f, -0.5);

	// 2nd quad "right side +x"
	normal[0] = 1.f;
	normal[1] = 0.f;
	normal[2] = 0.f;

	cube_mesh.addNormalfv (normal);
	cube_mesh.addVertice (0.5, 0.5f, 0.5);

	cube_mesh.addNormalfv (normal);
	cube_mesh.addVertice (0.5, -0.5f, 0.5);

	cube_mesh.addNormalfv (normal);
	cube_mesh.addVertice (0.5, -0.5f, -0.5);

	cube_mesh.addNormalfv (normal);
	cube_mesh.addVertice (0.5, -0.5f, -0.5);

	cube_mesh.addNormalfv (normal);
	cube_mesh.addVertice (0.5, 0.5f, -0.5);

	cube_mesh.addNormalfv (normal);
	cube_mesh.addVertice (0.5, 0.5f, 0.5);

	// 3rd quad "bottom -y"
	normal[0] = 0.f;
	normal[1] = -1.f;
	normal[2] = 0.f;

	cube_mesh.addNormalfv (normal);
	cube_mesh.addVertice (0.5, -0.5f, 0.5);

	cube_mesh.addNormalfv (normal);
	cube_mesh.addVertice (-0.5, -0.5f, 0.5);

	cube_mesh.addNormalfv (normal);
	cube_mesh.addVertice (-0.5, -0.5f, -0.5);

	cube_mesh.addNormalfv (normal);
	cube_mesh.addVertice (-0.5, -0.5f, -0.5);

	cube_mesh.addNormalfv (normal);
	cube_mesh.addVertice (0.5, -0.5f, -0.5);

	cube_mesh.addNormalfv (normal);
	cube_mesh.addVertice (0.5, -0.5f, 0.5);

	// 4th quad "left -x"
	normal[0] = -1.f;
	normal[1] = 0.f;
	normal[2] = 0.f;

	cube_mesh.addNormalfv (normal);
	cube_mesh.addVertice (-0.5, 0.5f, 0.5);

	cube_mesh.addNormalfv (normal);
	cube_mesh.addVertice (-0.5, 0.5f, -0.5);

	cube_mesh.addNormalfv (normal);
	cube_mesh.addVertice (-0.5, -0.5f, -0.5);

	cube_mesh.addNormalfv (normal);
	cube_mesh.addVertice (-0.5, -0.5f, -0.5);

	cube_mesh.addNormalfv (normal);
	cube_mesh.addVertice (-0.5, -0.5f, 0.5);

	cube_mesh.addNormalfv (normal);
	cube_mesh.addVertice (-0.5, 0.5f, 0.5);

	// 5th quad "front +z"
	normal[0] = 0.f;
	normal[1] = 0.f;
	normal[2] = 1.f;

	cube_mesh.addNormalfv (normal);
	cube_mesh.addVertice (-0.5, 0.5f, 0.5);

	cube_mesh.addNormalfv (normal);
	cube_mesh.addVertice (-0.5, -0.5f, 0.5);

	cube_mesh.addNormalfv (normal);
	cube_mesh.addVertice (0.5, -0.5f, 0.5);

	cube_mesh.addNormalfv (normal);
	cube_mesh.addVertice (0.5, -0.5f, 0.5);

	cube_mesh.addNormalfv (normal);
	cube_mesh.addVertice (0.5, 0.5f, 0.5);

	cube_mesh.addNormalfv (normal);
	cube_mesh.addVertice (-0.5, 0.5f, 0.5);

	// 6th quad "back -z"
	normal[0] = 0.f;
	normal[1] = 0.f;
	normal[2] = -1.f;

	cube_mesh.addNormalfv (normal);
	cube_mesh.addVertice (-0.5, 0.5f, -0.5);

	cube_mesh.addNormalfv (normal);
	cube_mesh.addVertice (0.5, 0.5f, -0.5);

	cube_mesh.addNormalfv (normal);
	cube_mesh.addVertice (0.5, -0.5f, -0.5);

	cube_mesh.addNormalfv (normal);
	cube_mesh.addVertice (0.5, -0.5f, -0.5);

	cube_mesh.addNormalfv (normal);
	cube_mesh.addVertice (-0.5, -0.5f, -0.5);

	cube_mesh.addNormalfv (normal);
	cube_mesh.addVertice (-0.5, 0.5f, -0.5);

	cube_mesh.end();

	cube_mesh.generate_vbo();
	qDebug() << "cube_mesh vbo_id = " << cube_mesh.vbo_id;
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

	glColor3f (0.6, 0.6, 0.6);
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

	drawGrid();

	glEnable (GL_COLOR_MATERIAL);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	if (cube_mesh.vbo_id != 0) {
		glColor3f (0.8, 0.2, 0.2);
		glBindBuffer (GL_ARRAY_BUFFER, cube_mesh.vbo_id);

		glVertexPointer (3, GL_FLOAT, 0, 0);
		glNormalPointer (GL_FLOAT, 0, (const GLvoid *) (cube_mesh.vertices.size() * sizeof (float) * 3));

		glEnableClientState (GL_VERTEX_ARRAY);
		glEnableClientState (GL_NORMAL_ARRAY);

		glDrawArrays (GL_TRIANGLES, 0, cube_mesh.vertices.size());
		glBindBuffer (GL_ARRAY_BUFFER, 0);
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

