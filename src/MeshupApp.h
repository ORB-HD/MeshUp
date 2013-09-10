/*
 * MeshUp - A visualization tool for multi-body systems based on skeletal
 * animation and magic.
 *
 * Copyright (c) 2012 Martin Felis <martin.felis@iwr.uni-heidelberg.de>
 *
 * Licensed under the MIT license. See LICENSE for more details.
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QTimer>
#include <QTimeLine>
#include <QSocketNotifier>
#include "ui_MainWindow.h"
#include "RenderImageDialog.h"
#include "RenderImageSeriesDialog.h"

class MeshupApp : public QMainWindow, public Ui::MainWindow
{
    Q_OBJECT
 
public:
    MeshupApp(QWidget *parent = 0);

		void parseArguments (int argc, char* argv[]);

		// unix signal handler
		static void SIGUSR1Handler(int unused);
		
protected:
		QTimer *timer;
		QTimeLine *timeLine;
		QLabel *versionLabel;

		int glRefreshTime;

		bool playerPaused;
		RenderImageDialog* renderImageDialog;
		RenderImageSeriesDialog* renderImageSeriesDialog;

public slots:
		virtual void closeEvent(QCloseEvent *event);
		virtual void focusChanged (QFocusEvent *event);
		virtual void focusInEvent (QFocusEvent *event);

		void handleSIGUSR1();

		void saveSettings ();
		void loadSettings ();

		void camera_changed ();
		void update_camera ();

		void toggle_play_animation (bool status);
		void toggle_loop_animation (bool status);

		void action_load_model();
		void action_load_animation();

		void action_reload_files ();
		void action_quit();

		void animation_loaded();
		void initialize_curves();

		void timeline_frame_changed (int frame_index);
		void timeline_set_frame (int frame_index);
		void timeslider_value_changed (int frame_index);

		void update_time_widgets ();

		void actionRenderAndSaveToFile ();
		void actionRenderSeriesAndSaveToFile ();
	private:
		static int sigusr1Fd[2];
		QSocketNotifier *snUSR1;
};

int setup_unix_signal_handlers();
#endif
