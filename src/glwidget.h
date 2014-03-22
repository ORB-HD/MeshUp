/*
 * MeshUp - A visualization tool for multi-body systems based on skeletal
 * animation and magic.
 *
 * Copyright (c) 2012 Martin Felis <martin.felis@iwr.uni-heidelberg.de>
 *
 * Licensed under the MIT license. See LICENSE for more details.
 */

#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QGLWidget>
#include <QImage>

#include <iostream>

#include "Model.h"

struct Animation;
typedef boost::shared_ptr<Animation> AnimationPtr;

class GLWidget : public QGLWidget
{
	Q_OBJECT

	public:
		GLWidget(QWidget *parent = 0);
		virtual ~GLWidget();

		QSize minimumSizeHint() const;
		QSize sizeHint() const;

		void loadModel (const char *filename);
		void loadAnimation (const char *filename);

		float getAnimationDuration();

		QImage renderContentOffscreen (int image_width, int image_height, bool use_alpha);

		Vector3f getCameraPoi();
		Vector3f getCameraEye();
		void setCameraPoi (const Vector3f values);
		void setCameraEye (const Vector3f values);

		void updateSphericalCoordinates();

		MeshupModelPtr model_data;
		AnimationPtr animation_data;

		bool draw_base_axes;
		bool draw_frame_axes;
		bool draw_grid;
		bool draw_floor;
		bool draw_meshes;
		bool draw_shadows;
		bool draw_curves;
		bool draw_points;

		bool draw_orthographic;

	protected:
		void update_timer();
		void drawGrid();

		void initializeGL();
		void drawScene ();
		void paintGL();
		void resizeGL(int width, int height);

		void keyPressEvent (QKeyEvent* event);
		void mousePressEvent(QMouseEvent *event);
		void mouseMoveEvent(QMouseEvent *event);

	private:
		void updateCamera();
		void updateLightingMatrices();

		void shadowMapSetupPass1();
		void shadowMapSetupPass2();
		void shadowMapSetupPass3();
		void shadowMapCleanup();

		QPoint lastMousePos;
		// azimuth
		float phi;
		// the elevation angle
		float theta;
		// radius
		float r;
		float fov;

		Vector3f poi;
		Vector3f eye;
		Vector3f up;

		unsigned int application_time_msec;
		unsigned int first_frame_msec;
		double delta_time_sec;

		std::string model_filename;
		std::string animation_filename;

		float windowWidth;
		float windowHeight;

		bool opengl_initialized;

	public slots:
		void toggle_draw_grid(bool status);
		void toggle_draw_base_axes(bool status);
		void toggle_draw_frame_axes(bool status);
		void toggle_draw_floor(bool status);
		void toggle_draw_meshes(bool status);
		void toggle_draw_shadows(bool status);
		void toggle_draw_curves(bool status);
		void toggle_draw_points(bool status);

		void toggle_draw_orthographic(bool status);

		void set_front_view ();
		void set_side_view ();
		void set_top_view ();

		void actionRenderImage();
		void actionRenderSeriesImage();

		void setAnimationTime (float fraction);
	
	signals:
		void animation_loaded();
		void model_loaded();
		void camera_changed();
};

#endif
