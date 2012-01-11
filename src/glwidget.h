#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QGLWidget>
#include <QVector3D>

#include <iostream>

class GLWidget : public QGLWidget
{
	Q_OBJECT

	public:
		GLWidget(QWidget *parent = 0);
		virtual ~GLWidget();

		QSize minimumSizeHint() const;
		QSize sizeHint() const;

		void loadModel (const char *filename);
		float getAnimationDuration();

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

		std::string model_filename;

		bool opengl_initialized;

	public slots:
		void toggle_draw_grid(bool status);
		void toggle_draw_bones(bool status);
		void toggle_draw_axes(bool status);

		void setAnimationTime (float fraction);
};

#endif
