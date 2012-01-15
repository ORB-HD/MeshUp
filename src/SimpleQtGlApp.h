#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QTimer>
#include <QTimeLine>
#include "ui_MainWindow.h"
#include "RenderImageDialog.h"

class SimpleQtGlApp : public QMainWindow, public Ui::MainWindow
{
    Q_OBJECT
 
public:
    SimpleQtGlApp(QWidget *parent = 0);

		void parseArguments (int argc, char* argv[]);

protected:
		QTimer *timer;
		QTimeLine *timeLine;

		bool playerPaused;
		RenderImageDialog* renderImageDialog;

public slots:
		void toggle_play_animation (bool status);
		void toggle_loop_animation (bool status);

		void timeline_frame_changed (int frame_index);
		void timeline_set_frame (int frame_index);
		void timeslider_value_changed (int frame_index);

		void actionRenderAndSaveToFile ();
};
 
#endif
