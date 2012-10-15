#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QTimer>
#include <QTimeLine>
#include "ui_MainWindow.h"
#include "RenderImageDialog.h"
#include "RenderImageSeriesDialog.h"
#include "ui/AnimationEditModel.h"

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

		AnimationEditModel *animation_edit_model;

public slots:
		virtual void closeEvent(QCloseEvent *event);
		virtual void focusChanged (QFocusEvent *event);
		virtual void focusInEvent (QFocusEvent *event);

		void saveSettings ();
		void loadSettings ();

		void toggle_play_animation (bool status);
		void toggle_loop_animation (bool status);

		void action_load_model();
		void action_load_animation();

		void action_save_animation();
		void action_save_animation_to();

		void action_reload_files ();
		void action_quit();

		void animation_loaded();
		void initialize_curves();

		void action_next_keyframe();
		void action_prev_keyframe();

		void timeline_frame_changed (int frame_index);
		void timeline_set_frame (int frame_index);
		void timeslider_value_changed (int frame_index);

		void update_time_widgets ();

		void actionRenderAndSaveToFile ();
		void actionRenderSeriesAndSaveToFile ();
};
 
#endif
