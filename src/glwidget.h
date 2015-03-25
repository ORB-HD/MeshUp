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
#include "SimpleMath/SimpleMath.h"

#include "Camera.h"

struct Scene;

class GLWidget : public QGLWidget
{
	Q_OBJECT

	public:
		GLWidget(QWidget *parent = 0);
		virtual ~GLWidget();

		QSize minimumSizeHint() const;
		QSize sizeHint() const;

		Scene *scene;

		QImage renderContentOffscreen (int image_width, int image_height, bool use_alpha);

		Camera camera;

		bool draw_base_axes;
		bool draw_frame_axes;
		bool draw_grid;
		bool draw_floor;
		bool draw_meshes;
		bool draw_shadows;
		bool draw_curves;
		bool draw_points;
		bool white_mode;

		Vector4f light_position;

		Vector3f getCameraPoi();
		Vector3f getCameraEye();

		void setCameraPoi (const Vector3f values);
		void setCameraEye (const Vector3f values);

		void saveScreenshot (const char* filename, int width, int height, bool transparency = true);

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

		unsigned int application_time_msec;
		unsigned int first_frame_msec;
		double delta_time_sec;

		std::string model_filename;
		std::string animation_filename;

		float windowWidth;
		float windowHeight;

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
		void toggle_white_mode(bool status);

		void set_front_view ();
		void set_side_view ();
		void set_top_view ();

		void actionRenderImage();
		void actionRenderSeriesImage();

	signals:
		void camera_changed();
		void opengl_initialized();
};

#endif
