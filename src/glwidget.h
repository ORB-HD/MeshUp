#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QGLWidget>
#include <QVector3D>

class GLWidget : public QGLWidget
{
	Q_OBJECT

	public:
		GLWidget(QWidget *parent = 0);
		~GLWidget();

		QSize minimumSizeHint() const;
		QSize sizeHint() const;

	protected:
		void update_timer();
		void drawGrid();

		void initializeGL();
		void paintGL();
		void resizeGL(int width, int height);

		void keyPressEvent (QKeyEvent* event);
		void mousePressEvent(QMouseEvent *event);
		void mouseMoveEvent(QMouseEvent *event);

	private:
		void updateSphericalCoordinates();
		void updateCamera();

		QPoint lastMousePos;
		// azimuth
		float phi;
		// the elevation angle
		float theta;
		// radius
		float r;

		QVector3D poi;
		QVector3D eye;
		QVector3D up;

		unsigned int application_time_msec;
		unsigned int first_frame_msec;
		double delta_time_sec;
};

#endif
