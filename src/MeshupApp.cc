#include <QtGui> 
#include <QFile>
#include <QDir>

#include "glwidget.h" 
#include "MeshupApp.h"

#include <assert.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <fstream>

#include "json/json.h"

using namespace std;

Json::Value settings_json;

MeshupApp::MeshupApp(QWidget *parent)
{
	timer = new QTimer (this);
	setupUi(this); // this sets up GUI

	renderImageDialog = new RenderImageDialog(this);

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

	checkBoxDrawBaseAxes->setChecked (glWidget->draw_base_axes);
	checkBoxDrawFloor->setChecked (glWidget->draw_floor);
	checkBoxDrawFrameAxes->setChecked (glWidget->draw_frame_axes);
	checkBoxDrawGrid->setChecked (glWidget->draw_grid);
	checkBoxDrawMeshes->setChecked (glWidget->draw_meshes);
	checkBoxDrawShadows->setChecked (glWidget->draw_shadows);

	// player is paused on startup
	playerPaused = true;

	dockPlayerControls->setVisible(false);
	dockViewSettings->setVisible(false);

	// the timer is used to continously redraw the OpenGL widget
	connect (timer, SIGNAL(timeout()), glWidget, SLOT(updateGL()));

	// render dialogs
	connect (actionRenderImage, SIGNAL (triggered()), this, SLOT (actionRenderAndSaveToFile()));

	// view stettings
	connect (checkBoxDrawBaseAxes, SIGNAL (toggled(bool)), glWidget, SLOT (toggle_draw_base_axes(bool)));
	connect (checkBoxDrawFloor, SIGNAL (toggled(bool)), glWidget, SLOT (toggle_draw_floor(bool)));
	connect (checkBoxDrawFrameAxes, SIGNAL (toggled(bool)), glWidget, SLOT (toggle_draw_frame_axes(bool)));
	connect (checkBoxDrawGrid, SIGNAL (toggled(bool)), glWidget, SLOT (toggle_draw_grid(bool)));
	connect (checkBoxDrawMeshes, SIGNAL (toggled(bool)), glWidget, SLOT (toggle_draw_meshes(bool)));
	connect (checkBoxDrawShadows, SIGNAL (toggled(bool)), glWidget, SLOT (toggle_draw_shadows(bool)));

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

	loadSettings();
	saveSettings();
}

void print_usage() {
	cout << "Usage: meshup [model_file] [animation_file] " << endl
		<< "Visualization tool for multi-body systems based on skeletal animation and magic." << endl
		<< endl
		<< "Report bugs to martin.felis@iwr.uni-heidelberg.de" << endl;
}

void MeshupApp::parseArguments (int argc, char* argv[]) {
	bool model_loaded = false;
	bool animation_loaded = false;
	for (int i = 1; i < argc; i++) {
		if (string(argv[i]) == "--help"
				|| string(argv[i]) == "-h") {
			print_usage();
			exit(1);
		}

		// qDebug () << "Argument " << i << ": " << argv[i];
		QFile test_file (argv[i]);
		if (test_file.exists()) {
			// first file is assumed to be the model file
			if (!model_loaded) {
				glWidget->loadModel(argv[i]);
				model_loaded = true;
			} else if (!animation_loaded) {
				glWidget->loadAnimation(argv[i]);
				animation_loaded = true;
			}
		}
	}
}

void MeshupApp::closeEvent (QCloseEvent *event) {
	saveSettings();
}

void MeshupApp::saveSettings () {
	settings_json["configuration"]["view"]["draw_base_axes"] = checkBoxDrawBaseAxes->isChecked();
	settings_json["configuration"]["view"]["draw_floor"] = checkBoxDrawFloor->isChecked();
	settings_json["configuration"]["view"]["draw_frame_axes"] = checkBoxDrawFrameAxes->isChecked();
	settings_json["configuration"]["view"]["draw_grid"] = checkBoxDrawGrid->isChecked();
	settings_json["configuration"]["view"]["draw_meshes"] = checkBoxDrawMeshes->isChecked();
	settings_json["configuration"]["view"]["draw_shadows"] = checkBoxDrawShadows->isChecked();

	settings_json["configuration"]["docks"]["view_settings"]["visible"] = dockViewSettings->isVisible();
	settings_json["configuration"]["docks"]["player_controls"]["visible"] = dockPlayerControls->isVisible();
	settings_json["configuration"]["docks"]["player_controls"]["repeat"] = checkBoxLoopAnimation->isChecked();

	settings_json["configuration"]["window"]["width"] = width();
	settings_json["configuration"]["window"]["height"] = height();
	settings_json["configuration"]["window"]["xpos"] = x();
	settings_json["configuration"]["window"]["ypos"] = y();

	string home_dir = getenv("HOME");

	// create the path if it does not yet exist
	QDir settings_dir ((home_dir + string("/.meshup")).c_str());
	if (!settings_dir.exists()) {
		settings_dir.mkdir((home_dir + string ("/.meshup")).c_str());
	}

	string settings_filename = home_dir + string ("/.meshup/settings.json");

	ofstream config_file (settings_filename.c_str(), ios::trunc);

	if (!config_file) {
		cerr << "Error: Could not open config file '" << settings_filename << "' for writing!" << endl;
		exit (1);
	}

	config_file << settings_json;

	config_file.close();
}

void MeshupApp::loadSettings () {
	string home_dir = getenv("HOME");

	string settings_filename = home_dir + string ("/.meshup/settings.json");
	ifstream config_file (settings_filename.c_str());

	// only read values if they are existing
	if (config_file) {

		qDebug() << "Reading settings from file: " << settings_filename.c_str() ;
		stringstream buffer;
		buffer << config_file.rdbuf();
		config_file.close();

		Json::Reader reader;
		bool parse_result = reader.parse (buffer.str(), settings_json);
		if (!parse_result) {
			cerr << "Error: Parsing file '" << settings_filename << "': " << reader.getFormatedErrorMessages();

			exit (1);
		}
	}

	checkBoxDrawBaseAxes->setChecked(settings_json["configuration"]["view"].get("draw_base_axes", true).asBool());
	checkBoxDrawFloor->setChecked(settings_json["configuration"]["view"].get("draw_floor", true).asBool());
	checkBoxDrawFrameAxes->setChecked(settings_json["configuration"]["view"].get("draw_frame_axes", false).asBool());
	checkBoxDrawGrid->setChecked(settings_json["configuration"]["view"].get("draw_grid", false).asBool());
	checkBoxDrawMeshes->setChecked(settings_json["configuration"]["view"].get("draw_meshes", true).asBool());
	checkBoxDrawShadows->setChecked(settings_json["configuration"]["view"].get("draw_shadows", false).asBool());

	dockViewSettings->setVisible(settings_json["configuration"]["docks"]["view_settings"].get("visible", false).asBool());
	dockPlayerControls->setVisible(settings_json["configuration"]["docks"]["player_controls"].get("visible", true).asBool());
	checkBoxLoopAnimation->setChecked(settings_json["configuration"]["docks"]["player_controls"].get("repeat", true).asBool());

	int x, y, w, h;

	x = settings_json["configuration"]["window"].get("xpos", 100).asInt();
	y = settings_json["configuration"]["window"].get("xpos", 50).asInt();
	w = settings_json["configuration"]["window"].get("width", 650).asInt();
	h = settings_json["configuration"]["window"].get("height", 650).asInt();

	setGeometry (x, y, w, h);
}

void MeshupApp::toggle_play_animation (bool status) {
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

void MeshupApp::toggle_loop_animation (bool status) {
	if (status) {
		timeLine->setLoopCount(0);
	} else {
		timeLine->setLoopCount(1);
	}
}

void MeshupApp::timeline_frame_changed (int frame_index) {
//	qDebug () << __func__ << " frame_index = " << frame_index;

	horizontalSliderTime->setValue (frame_index);

	timeLine->setDuration (glWidget->getAnimationDuration() * 1000.f);
	glWidget->setAnimationTime (static_cast<float>(frame_index) / 1000.);
}

void MeshupApp::timeline_set_frame (int frame_index) {
//	qDebug () << __func__ << " frame_index = " << frame_index;

	static bool repeat_gate = false;

	if (!repeat_gate) {
		repeat_gate = true;
		timeLine->setCurrentTime (frame_index * glWidget->getAnimationDuration());
		repeat_gate = false;
	}
	glWidget->setAnimationTime (static_cast<float>(frame_index) / 1000.);
}

void MeshupApp::timeslider_value_changed (int frame_index) {
	glWidget->setAnimationTime (static_cast<float>(frame_index) / 1000.);
}

void MeshupApp::actionRenderAndSaveToFile () {
	renderImageDialog->WidthSpinBox->setValue(glWidget->width());
	renderImageDialog->HeightSpinBox->setValue(glWidget->width());

	int result = renderImageDialog->exec();

	if (result == QDialog::Rejected)
		return;

	string figure_name = string("./image") ;

	stringstream filename_stream;
	filename_stream << figure_name << "_" << setw(3) << setfill('0') << 0 << ".png";

	if (QFile (filename_stream.str().c_str()).exists()) {
		int i = 1;
		while (QFile (filename_stream.str().c_str()).exists()) {
			filename_stream.str("");
			filename_stream << figure_name << "_" << setw(3) << setfill('0') << i << ".png";
			i++;
		}
	}

	int w = renderImageDialog->WidthSpinBox->value();
	int h = renderImageDialog->HeightSpinBox->value();

	cout << "Saving screenshot to: " << filename_stream.str() << " (size: " << w << "x" << h << ")" << endl;

	QImage image = glWidget->renderContentOffscreen (w,h, renderImageDialog->TransparentBackgroundCheckBox->isChecked());
	image.save (filename_stream.str().c_str(), 0, -1);
}
