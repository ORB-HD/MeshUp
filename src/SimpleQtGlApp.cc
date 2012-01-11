#include <QtGui> 

#include "glwidget.h" 
#include "SimpleQtGlApp.h"

#include <assert.h>
#include <iostream>

using namespace std;

SimpleQtGlApp::SimpleQtGlApp(QWidget *parent)
{
	timer = new QTimer (this);
	setupUi(this); // this sets up GUI

	timer->setSingleShot(false);
	timer->start(20);

	timeLine = new QTimeLine (1000., this);
	timeLine->setCurveShape(QTimeLine::LinearCurve);

	if (checkBoxLoopAnimation->isChecked())
		timeLine->setLoopCount(0);
	else
		timeLine->setLoopCount(1);

	timeLine->setUpdateInterval(20);
	timeLine->setFrameRange(0, 1000);

	horizontalSliderTime->setMinimum(0);
	horizontalSliderTime->setMaximum(1000.);
	horizontalSliderTime->setSingleStep(1);

	// player is paused on startup
	playerPaused = true;

	// dockPlayerControls->setVisible(false);
	dockViewSettings->setVisible(false);

	// the timer is used to continously redraw the OpenGL widget
	connect (timer, SIGNAL(timeout()), glWidget, SLOT(updateGL()));

	// view stettings
	connect (checkBoxDrawGrid, SIGNAL (toggled(bool)), glWidget, SLOT (toggle_draw_grid(bool)));
	connect (checkBoxDrawBones, SIGNAL (toggled(bool)), glWidget, SLOT (toggle_draw_bones(bool)));
	connect (checkBoxDrawAxes, SIGNAL (toggled(bool)), glWidget, SLOT (toggle_draw_axes(bool)));

	// timeline & timeSlider
	connect (timeLine, SIGNAL(frameChanged(int)), this, SLOT(timeline_frame_changed(int)));
	connect (horizontalSliderTime, SIGNAL(sliderMoved(int)), this, SLOT(timeline_set_frame(int)));
	connect (horizontalSliderTime, SIGNAL(valueChanged(int)), this, SLOT(timeslider_value_changed(int)));
	connect (timeLine, SIGNAL(finished()), toolButtonPlay, SLOT (click()));

	// pausing and playing button
	connect (toolButtonPlay, SIGNAL (clicked(bool)), this, SLOT (toggle_play_animation(bool)));
	connect (checkBoxLoopAnimation, SIGNAL (toggled(bool)), this, SLOT (toggle_loop_animation(bool)));

	// quit when we want to quit
	connect (actionQuit, SIGNAL( triggered() ), qApp, SLOT( quit() ));
}

void SimpleQtGlApp::toggle_play_animation (bool status) {
	playerPaused = status;

	if (status) {
		// if we are at the end of the time, we have to restart
		if (timeLine->currentFrame() == timeLine->endFrame()) {
			timeLine->setCurrentTime(0);
		}
		timeLine->resume();
	} else {
		timeLine->stop();
	}

	toolButtonPlay->setText("Play");
}

void SimpleQtGlApp::toggle_loop_animation (bool status) {
	if (status) {
		timeLine->setLoopCount(0);
	} else {
		timeLine->setLoopCount(1);
	}
}


void SimpleQtGlApp::timeline_frame_changed (int frame_index) {
//	qDebug () << __func__ << " frame_index = " << frame_index;

	horizontalSliderTime->setValue (frame_index);

	timeLine->setDuration (glWidget->getAnimationDuration() * 1000.f);
	glWidget->setAnimationTime (static_cast<float>(frame_index) / 1000.);
}

void SimpleQtGlApp::timeline_set_frame (int frame_index) {
//	qDebug () << __func__ << " frame_index = " << frame_index;

	static bool repeat_gate = false;

	if (!repeat_gate) {
		repeat_gate = true;
		timeLine->setCurrentTime (frame_index * glWidget->getAnimationDuration());
		repeat_gate = false;
	}
	glWidget->setAnimationTime (static_cast<float>(frame_index) / 1000.);
}

void SimpleQtGlApp::timeslider_value_changed (int frame_index) {
	glWidget->setAnimationTime (static_cast<float>(frame_index) / 1000.);
}

