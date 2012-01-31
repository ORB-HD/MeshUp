#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QTimer>
#include <QTimeLine>
#include "ui_MainWindow.h"
#include "RenderImageDialog.h"
#include "RenderImageSeriesDialog.h"

class MeshupApp : public QMainWindow, public Ui::MainWindow
{
    Q_OBJECT
 
public:
    MeshupApp(QWidget *parent = 0);

		void parseArguments (int argc, char* argv[]);

protected:
		QTimer *timer;
		QTimeLine *timeLine;

		bool playerPaused;
		RenderImageDialog* renderImageDialog;
		RenderImageSeriesDialog* renderImageSeriesDialog;

public slots:
		virtual void closeEvent(QCloseEvent *event);
		virtual void focusChanged (QFocusEvent *event);
		virtual void focusInEvent (QFocusEvent *event);

		void saveSettings ();
		void loadSettings ();

		void toggle_play_animation (bool status);
		void toggle_loop_animation (bool status);

		void action_reload_files ();
		void action_quit();

		void timeline_frame_changed (int frame_index);
		void timeline_set_frame (int frame_index);
		void timeslider_value_changed (int frame_index);

		void actionRenderAndSaveToFile ();
		void actionRenderSeriesAndSaveToFile ();
};
 
#endif
