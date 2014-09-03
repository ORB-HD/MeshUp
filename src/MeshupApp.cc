/*
 * MeshUp - A visualization tool for multi-body systems based on skeletal
 * animation and magic.
 *
 * Copyright (c) 2012 Martin Felis <martin.felis@iwr.uni-heidelberg.de>
 *
 * Licensed under the MIT license. See LICENSE for more details.
 */

#include <QtGui> 
#include <QFile>
#include <QDir>
#include <QFileDialog>
#include <QProgressDialog>
#include <QRegExp>
#include <QRegExpValidator>

#include "meshup_config.h"

#include "glwidget.h" 
#include "MeshupApp.h"
#include "Animation.h"

#include <assert.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <fstream>
#include <signal.h>
#include <sys/socket.h>

#include "json/json.h"
#include "colorscale.h"

using namespace std;

Json::Value settings_json;

const double TimeLineDuration = 1000.;

MeshupApp::MeshupApp(QWidget *parent)
{
	timer = new QTimer (this);
	setupUi(this); // this sets up GUI

	//setting up the socket pair for signal handling
	if (!::socketpair(AF_UNIX, SOCK_STREAM,0,sigusr1Fd)) {
		snUSR1 = new QSocketNotifier(sigusr1Fd[1], QSocketNotifier::Read, this);
		connect(snUSR1, SIGNAL(activated(int)), this, SLOT(handleSIGUSR1()));
	}
	

	// version label
	string version_str = string("v") + MESHUP_VERSION_STRING;
	versionLabel = new QLabel(version_str.c_str(), this);
	menubar->setCornerWidget (versionLabel);

	renderImageDialog = new RenderImageDialog(this);
	renderImageSeriesDialog = new RenderImageSeriesDialog(this);

	// this is NOT the default value,
	//  its just an initialization of the memory with something
	//  that makes sense
	glRefreshTime=20; 

	timer->setSingleShot(false);
	// Start of timer is now at the end of this function
	//  to allow the settings being used

	timeLine = new QTimeLine (TimeLineDuration, this);
	timeLine->setCurveShape(QTimeLine::LinearCurve);

	if (checkBoxLoopAnimation->isChecked())
		timeLine->setLoopCount(0);
	else
		timeLine->setLoopCount(1);

	timeLine->setUpdateInterval(20);
	timeLine->setFrameRange(0, 1000);
	
	spinBoxSpeed->setMinimum(1);
	spinBoxSpeed->setMaximum(1000);
	spinBoxSpeed->setValue(100);
	spinBoxSpeed->setSingleStep(5);

	horizontalSliderTime->setMinimum(0);
	horizontalSliderTime->setMaximum(TimeLineDuration);
	horizontalSliderTime->setSingleStep(1);

	checkBoxDrawBaseAxes->setChecked (glWidget->draw_base_axes);
	checkBoxDrawFloor->setChecked (glWidget->draw_floor);
	checkBoxDrawFrameAxes->setChecked (glWidget->draw_frame_axes);
	checkBoxDrawGrid->setChecked (glWidget->draw_grid);
	checkBoxDrawMeshes->setChecked (glWidget->draw_meshes);
	checkBoxDrawShadows->setChecked (glWidget->draw_shadows);
	checkBoxDrawCurves->setChecked (glWidget->draw_curves);
	checkBoxDrawPoints->setChecked (glWidget->draw_points);

	// camera controls
	QRegExp	coord_expr ("^\\s*-?\\d*(\\.|\\.\\d+)?\\s*,\\s*-?\\d*(\\.|\\.\\d+)?\\s*,\\s*-?\\d*(\\.|\\.\\d+)?\\s*$");
	QRegExpValidator *coord_validator_eye = new QRegExpValidator (coord_expr, lineEditCameraEye);
	QRegExpValidator *coord_validator_center = new QRegExpValidator (coord_expr, lineEditCameraCenter);
	lineEditCameraEye->setValidator (coord_validator_eye);
	lineEditCameraCenter->setValidator (coord_validator_center);

	// player is paused on startup
	playerPaused = true;

	dockCameraControls->setVisible(false);
	dockPlayerControls->setVisible(true);
	dockViewSettings->setVisible(false);

	// the timer is used to continously redraw the OpenGL widget
	connect (timer, SIGNAL(timeout()), glWidget, SLOT(updateGL()));

	// render dialogs
	connect (actionRenderImage, SIGNAL (triggered()), this, SLOT (actionRenderAndSaveToFile()));
	connect (actionRenderSeriesImage, SIGNAL (triggered()), this, SLOT (actionRenderSeriesAndSaveToFile()));

	// view stettings
	connect (checkBoxDrawBaseAxes, SIGNAL (toggled(bool)), glWidget, SLOT (toggle_draw_base_axes(bool)));
	connect (checkBoxDrawFloor, SIGNAL (toggled(bool)), glWidget, SLOT (toggle_draw_floor(bool)));
	connect (checkBoxDrawFrameAxes, SIGNAL (toggled(bool)), glWidget, SLOT (toggle_draw_frame_axes(bool)));
	connect (checkBoxDrawGrid, SIGNAL (toggled(bool)), glWidget, SLOT (toggle_draw_grid(bool)));
	connect (checkBoxDrawMeshes, SIGNAL (toggled(bool)), glWidget, SLOT (toggle_draw_meshes(bool)));
	connect (checkBoxDrawShadows, SIGNAL (toggled(bool)), glWidget, SLOT (toggle_draw_shadows(bool)));
	connect (checkBoxDrawCurves, SIGNAL (toggled(bool)), glWidget, SLOT (toggle_draw_curves(bool)));
	connect (checkBoxDrawPoints, SIGNAL (toggled(bool)), glWidget, SLOT (toggle_draw_points(bool)));

	connect (actionFrontView, SIGNAL (triggered()), glWidget, SLOT (set_front_view()));
	connect (actionSideView, SIGNAL (triggered()), glWidget, SLOT (set_side_view()));
	connect (actionTopView, SIGNAL (triggered()), glWidget, SLOT (set_top_view()));
	connect (actionToggleOrthographic, SIGNAL (toggled(bool)), glWidget, SLOT (toggle_draw_orthographic(bool)));

	// timeline & timeSlider
	connect (timeLine, SIGNAL(frameChanged(int)), this, SLOT(timeline_frame_changed(int)));
	connect (horizontalSliderTime, SIGNAL(sliderMoved(int)), this, SLOT(timeline_set_frame(int)));
	connect (horizontalSliderTime, SIGNAL(valueChanged(int)), this, SLOT(timeslider_value_changed(int)));
	connect (timeLine, SIGNAL(finished()), toolButtonPlay, SLOT (click()));

	// pausing and playing button
	connect (toolButtonPlay, SIGNAL (clicked(bool)), this, SLOT (toggle_play_animation(bool)));
	connect (checkBoxLoopAnimation, SIGNAL (toggled(bool)), this, SLOT (toggle_loop_animation(bool)));

	// action_quit() makes sure to set the settings before we quit
	connect (actionQuit, SIGNAL( triggered() ), this, SLOT( action_quit() ));

	// keyboard shortcuts
	connect (actionLoadModel, SIGNAL ( triggered() ), this, SLOT(action_load_model()));
	connect (actionLoadAnimation, SIGNAL ( triggered() ), this, SLOT(action_load_animation()));

	connect (actionReloadFiles, SIGNAL ( triggered() ), this, SLOT(action_reload_files()));

	connect (glWidget, SIGNAL (animation_loaded()), this, SLOT (animation_loaded()));
	connect (glWidget, SIGNAL (model_loaded()), this, SLOT (camera_changed()));
	connect (glWidget, SIGNAL (camera_changed()), this, SLOT (camera_changed()));
	connect (lineEditCameraEye, SIGNAL (editingFinished()), this, SLOT (update_camera()));
	connect (lineEditCameraCenter, SIGNAL (editingFinished()), this, SLOT (update_camera()));

	connect (pushButtonUpdateCamera, SIGNAL (clicked()), this, SLOT (update_camera()));

	loadSettings();
	
	timer->start(glRefreshTime);
}

void print_usage() {
	cout << "Usage: meshup [model_name] [animation_file] " << endl
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

		if (!model_loaded) {
			string model_filename = find_model_file_by_name (argv[i]);
			if (model_filename.size() != 0) {
				glWidget->loadModel(model_filename.c_str());
				model_loaded = true;
			} else {
				cerr << "Model '" << argv[i] << "' not found! First parameter must be a model!" << endl;
				exit(1);
			}
		} else if (!animation_loaded) {
			glWidget->loadAnimation(argv[i]);
			animation_loaded = true;
		}
	}
}

void MeshupApp::closeEvent (QCloseEvent *event) {
	saveSettings();
}

void MeshupApp::focusChanged(QFocusEvent *event) {
	cerr << "focus changed!" << endl;
}

void MeshupApp::focusInEvent(QFocusEvent *event) {
	cerr << "focus in!" << endl;
}

void MeshupApp::saveSettings () {
	settings_json["configuration"]["view"]["draw_base_axes"] = checkBoxDrawBaseAxes->isChecked();
	settings_json["configuration"]["view"]["draw_floor"] = checkBoxDrawFloor->isChecked();
	settings_json["configuration"]["view"]["draw_frame_axes"] = checkBoxDrawFrameAxes->isChecked();
	settings_json["configuration"]["view"]["draw_grid"] = checkBoxDrawGrid->isChecked();
	settings_json["configuration"]["view"]["draw_meshes"] = checkBoxDrawMeshes->isChecked();
	settings_json["configuration"]["view"]["draw_shadows"] = checkBoxDrawShadows->isChecked();
	settings_json["configuration"]["view"]["draw_curves"] = checkBoxDrawCurves->isChecked();
	settings_json["configuration"]["view"]["draw_points"] = checkBoxDrawPoints->isChecked();
	settings_json["configuration"]["view"]["draw_orthographic"] = actionToggleOrthographic->isChecked();

	settings_json["configuration"]["docks"]["camera_controls"]["visible"] = dockCameraControls->isVisible();
	settings_json["configuration"]["docks"]["view_settings"]["visible"] = dockViewSettings->isVisible();
	settings_json["configuration"]["docks"]["player_controls"]["visible"] = dockPlayerControls->isVisible();
	settings_json["configuration"]["docks"]["player_controls"]["repeat"] = checkBoxLoopAnimation->isChecked();

	settings_json["configuration"]["window"]["width"] = width();
	settings_json["configuration"]["window"]["height"] = height();
	settings_json["configuration"]["window"]["xpos"] = x();
	settings_json["configuration"]["window"]["ypos"] = y();
	settings_json["configuration"]["window"]["glRefreshTime"] = glRefreshTime;

	settings_json["configuration"]["render"]["width"]  = renderImageSeriesDialog->WidthSpinBox->value();
	settings_json["configuration"]["render"]["height"] = renderImageSeriesDialog->HeightSpinBox->value();
	settings_json["configuration"]["render"]["fps"]    = renderImageSeriesDialog->FpsSpinBox->value();

	settings_json["configuration"]["render"]["fps_mode"]         = renderImageSeriesDialog->fpsModeRadioButton->isChecked();
	settings_json["configuration"]["render"]["frame_count_mode"] = renderImageSeriesDialog->frameCountModeRadioButton->isChecked();
	settings_json["configuration"]["render"]["mencoder"]         = renderImageSeriesDialog->mencoderBox->isChecked();
	settings_json["configuration"]["render"]["composite"]        = renderImageSeriesDialog->compositeBox->isChecked();
	settings_json["configuration"]["render"]["transparent"]      = renderImageSeriesDialog->transparentBackgroundCheckBox->isChecked();

	string home_dir = getenv("HOME");

	// create the path if it does not yet exist
	QDir settings_dir ((home_dir + string("/.meshup")).c_str());
	if (!settings_dir.exists()) {
		settings_dir.mkdir((home_dir + string ("/.meshup")).c_str());
	}

	cout << "Saving MeshUp settings to " << home_dir << "/.meshup/settings.json" << endl;

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

	checkBoxDrawBaseAxes->setChecked(settings_json["configuration"]["view"].get("draw_base_axes", glWidget->draw_base_axes).asBool());
	checkBoxDrawFloor->setChecked(settings_json["configuration"]["view"].get("draw_floor", glWidget->draw_floor).asBool());
	checkBoxDrawFrameAxes->setChecked(settings_json["configuration"]["view"].get("draw_frame_axes", glWidget->draw_frame_axes).asBool());
	checkBoxDrawGrid->setChecked(settings_json["configuration"]["view"].get("draw_grid", glWidget->draw_grid).asBool());
	checkBoxDrawMeshes->setChecked(settings_json["configuration"]["view"].get("draw_meshes", glWidget->draw_meshes).asBool());
	checkBoxDrawShadows->setChecked(settings_json["configuration"]["view"].get("draw_shadows", glWidget->draw_shadows).asBool());
	checkBoxDrawCurves->setChecked(settings_json["configuration"]["view"].get("draw_curves", glWidget->draw_curves).asBool());
	checkBoxDrawPoints->setChecked(settings_json["configuration"]["view"].get("draw_points", glWidget->draw_points).asBool());
	glWidget->toggle_draw_orthographic(settings_json["configuration"]["view"].get("draw_orthographic", glWidget->draw_orthographic).asBool());

	dockViewSettings->setVisible(settings_json["configuration"]["docks"]["view_settings"].get("visible", false).asBool());
	dockCameraControls->setVisible(settings_json["configuration"]["docks"]["camera_controls"].get("visible", false).asBool());
	dockPlayerControls->setVisible(settings_json["configuration"]["docks"]["player_controls"].get("visible", true).asBool());
	checkBoxLoopAnimation->setChecked(settings_json["configuration"]["docks"]["player_controls"].get("repeat", true).asBool());

	renderImageSeriesDialog->WidthSpinBox->setValue(settings_json["configuration"]["render"].get("width", glWidget->width()).asInt());
	renderImageSeriesDialog->HeightSpinBox->setValue(settings_json["configuration"]["render"].get("height", glWidget->height()).asInt());
	renderImageSeriesDialog->FpsSpinBox->setValue(settings_json["configuration"]["render"].get("fps", 25).asInt());

	renderImageSeriesDialog->fpsModeRadioButton->setChecked(settings_json["configuration"]["render"].get("fps_mode", true).asBool());
	renderImageSeriesDialog->frameCountModeRadioButton->setChecked(settings_json["configuration"]["render"].get("frame_count_mode", false).asBool());
	renderImageSeriesDialog->mencoderBox->setChecked(settings_json["configuration"]["render"].get("mencoder", false).asBool());
	renderImageSeriesDialog->compositeBox->setChecked(settings_json["configuration"]["render"].get("composite", false).asBool());
	renderImageSeriesDialog->transparentBackgroundCheckBox->setChecked(settings_json["configuration"]["render"].get("transparent", true).asBool());

	int x, y, w, h;

	x = settings_json["configuration"]["window"].get("xpos", 100).asInt();
	y = settings_json["configuration"]["window"].get("xpos", 50).asInt();
	w = settings_json["configuration"]["window"].get("width", 650).asInt();
	h = settings_json["configuration"]["window"].get("height", 650).asInt();
	glRefreshTime = settings_json["configuration"]["window"].get("glRefreshTime", 20).asInt();

	setGeometry (x, y, w, h);
	camera_changed();
}

void MeshupApp::camera_changed() {
	Vector3f center = glWidget->getCameraPoi();	
	Vector3f eye = glWidget->getCameraEye();	

	unsigned int digits = 2;

	stringstream center_stream ("");
	center_stream << std::fixed << std::setprecision(digits) << center[0] << ", " << center[1] << ", " << center[2];

	stringstream eye_stream ("");
	eye_stream << std::fixed << std::setprecision(digits) << eye[0] << ", " << eye[1] << ", " << eye[2];

	lineEditCameraEye->setText (eye_stream.str().c_str());
	lineEditCameraCenter->setText (center_stream.str().c_str());
}

Vector3f parse_vec3_string (const std::string vec3_string) {
	Vector3f result;

	unsigned int token_start = 0;
	unsigned int token_end = vec3_string.find(",");
	for (unsigned int i = 0; i < 3; i++) {
		string token = vec3_string.substr (token_start, token_end - token_start);

		result[i] = static_cast<float>(atof(token.c_str()));

		token_start = token_end + 1;
		token_end = vec3_string.find (", ", token_start);
	}

//	cout << "Parsed '" << vec3_string << "' to " << result.transpose() << endl;

	return result;
}

void MeshupApp::update_camera() {
	string center_string = lineEditCameraCenter->text().toStdString();
	Vector3f poi = parse_vec3_string (center_string);

	string eye_string = lineEditCameraEye->text().toStdString();
	Vector3f eye = parse_vec3_string (eye_string);

	glWidget->setCameraPoi(poi);
	glWidget->setCameraEye(eye);
	glWidget->updateSphericalCoordinates();
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

void MeshupApp::action_load_model() {
	QFileDialog file_dialog (this, "Select Model File");

	file_dialog.setNameFilter(tr("MeshupModels (*.json *lua)"));
	file_dialog.setFileMode(QFileDialog::ExistingFile);

	if (file_dialog.exec()) {
		glWidget->loadModel (file_dialog.selectedFiles().at(0).toStdString().c_str());
	}	
}

void MeshupApp::action_load_animation() {
	QFileDialog file_dialog (this, "Select Animation File");

	file_dialog.setNameFilter(tr("MeshupAnimation (*.txt *.csv)"));
	file_dialog.setFileMode(QFileDialog::ExistingFile);

	if (file_dialog.exec()) {
		glWidget->loadAnimation (file_dialog.selectedFiles().at(0).toStdString().c_str());
	}	
}

void MeshupApp::action_reload_files() {
	MeshupModelPtr test_model (new MeshupModel());
	AnimationPtr test_animation (new Animation());

	string model_filename = glWidget->model_data->model_filename;
	string animation_filename = glWidget->animation_data->animation_filename;

	// no model to reload
	if (model_filename.size() == 0) 
		return;

	bool status;
	status = test_model->loadModelFromFile(model_filename.c_str(), false);

	if (!status) {
		cerr << "Reloading of model '" << model_filename.c_str() << "' failed!";
		return;
	}

	glWidget->model_data = test_model;

	// no animation to reload
	if (animation_filename.size() == 0)
		return;

	status = test_animation->loadFromFile(animation_filename.c_str(), glWidget->model_data->configuration, false);
	if (!status) {
		cerr << "Reloading of animation '" << animation_filename.c_str() << "' failed!";
		return;
	}

	// try to set old animation time.
	if (glWidget->animation_data &&  glWidget->animation_data->current_time < test_animation->duration) {
		test_animation->current_time=glWidget->animation_data->current_time;
	}

	// everything worked fine -> replace the current model
	glWidget->animation_data = test_animation;

	emit (animation_loaded());
	
	return;
}

void MeshupApp::action_quit () {
	saveSettings();
	qApp->quit();
}

void MeshupApp::animation_loaded() {
	qDebug() << __func__;

	glWidget->model_data->resetPoses();

	UpdateModelFromAnimation (glWidget->model_data, glWidget->animation_data, glWidget->animation_data->current_time);

	initialize_curves();
}

void MeshupApp::initialize_curves() {
	// qDebug() << "initializing curves";

	float curve_frame_rate = 60.f;
	float duration = glWidget->animation_data->duration;
	
	float time_step = duration / curve_frame_rate;
	unsigned int step_count = duration * curve_frame_rate;
	float old_time = glWidget->animation_data->current_time;
	float current_time = 0.f;

	// qDebug() << "duration = " << scientific << duration << endl;
	// cout << "time_step = " << scientific << time_step << endl;
	// cout << "step_count = " << scientific << step_count << endl;

	glWidget->model_data->clearCurves();
		
	while (current_time < duration) {
		current_time += time_step;
		if (current_time > duration)
			current_time = duration;

		float fraction = current_time / duration * 2.f - 1.f;

		UpdateModelFromAnimation (glWidget->model_data, glWidget->animation_data, current_time);

		MeshupModel::FrameMap::iterator frame_iter = glWidget->model_data->framemap.begin();

		for (frame_iter; frame_iter != glWidget->model_data->framemap.end(); frame_iter++) {
			Matrix44f pose_matrix = frame_iter->second->pose_transform;
			Vector3f pose_translation (
					pose_matrix (3,0),
					pose_matrix (3,1),
					pose_matrix (3,2)
					);

			glWidget->model_data->addCurvePoint (frame_iter->first,
					pose_translation,
					Vector3f (
						colorscale::red(fraction),
						colorscale::green(fraction),
						colorscale::blue(fraction))
					);
		}
	}

	glWidget->animation_data->current_time = old_time;
	// qDebug() << "initializing curves done";
}

/** \brief Modifies the widgets to show the current time
 */
void MeshupApp::timeline_frame_changed (int frame_index) {
//	qDebug () << __func__ << " frame_index = " << frame_index;

	static bool repeat_gate = false;

	if (!repeat_gate) {
		repeat_gate = true;

		glWidget->setAnimationTime (static_cast<float>(frame_index) / TimeLineDuration);
		
		update_time_widgets();

		repeat_gate = false;
	}
}

/** \brief Modifies timeLine so that it reflects the value from the
 * horizontalSliderTime
 */
void MeshupApp::timeline_set_frame (int frame_index) {
//	qDebug () << __func__ << " frame_index = " << frame_index;
	static bool repeat_gate = false;

	if (!repeat_gate) {
		repeat_gate = true;

		// this automatically calls timeline_frame_changed and thus updates
		// the horizontal slider
		timeLine->setCurrentTime (frame_index * glWidget->getAnimationDuration());

		repeat_gate = false;
	}
	glWidget->setAnimationTime (static_cast<float>(frame_index) / TimeLineDuration);
}

void MeshupApp::timeslider_value_changed (int frame_index) {
	float current_time = static_cast<float>(frame_index) / TimeLineDuration * glWidget->getAnimationDuration();
	
	int num_seconds = static_cast<int>(floor(current_time));
	int num_milliseconds = static_cast<int>(round((current_time - num_seconds) * 1000.f));

	stringstream time_string("");
	time_string << num_seconds << "." << setw(3) << setfill('0') << num_milliseconds;
	timeLabel->setText(time_string.str().c_str());

	glWidget->setAnimationTime (static_cast<float>(frame_index) / TimeLineDuration);
}

void MeshupApp::update_time_widgets () {
//	qDebug() << __func__;
	if (glWidget->animation_data && glWidget->animation_data->duration > 0.) {
		double time_fraction = glWidget->animation_data->current_time / glWidget->animation_data->duration;
		int frame_index = static_cast<int>(round(time_fraction * TimeLineDuration));

		horizontalSliderTime->setValue (frame_index);
		timeLine->setDuration (glWidget->getAnimationDuration() * TimeLineDuration / (spinBoxSpeed->value() / 100.0));
		glWidget->setAnimationTime (static_cast<float>(frame_index) / TimeLineDuration);
	}
}

void MeshupApp::actionRenderAndSaveToFile () {
	renderImageDialog->WidthSpinBox->setValue(glWidget->width());
	renderImageDialog->HeightSpinBox->setValue(glWidget->height());

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

void MeshupApp::actionRenderSeriesAndSaveToFile () {
	int fps;
	bool fps_mode;
	bool doMencoder;
	bool doComposite;
	bool render_transparent;
	int width;
	int height;

	int result = renderImageSeriesDialog->exec();

	if (result == QDialog::Rejected)
		return;

	width = renderImageSeriesDialog->WidthSpinBox->value();
	height = renderImageSeriesDialog->HeightSpinBox->value();
	fps = renderImageSeriesDialog->FpsSpinBox->value();
	
	if (renderImageSeriesDialog->fpsModeRadioButton->isChecked())
		fps_mode = true;
	else
		fps_mode = false;

	doMencoder = renderImageSeriesDialog->mencoderBox->isChecked();
	doComposite = renderImageSeriesDialog->compositeBox->isChecked();
	render_transparent = renderImageSeriesDialog->transparentBackgroundCheckBox->isChecked();
	
	string figure_name = string("./image-series") ;
	stringstream filename_stream;
	
	int series_nr=0;
	while (true) {
		filename_stream.str("");
		filename_stream << figure_name << "_" << setw(3) << setfill('0') << series_nr << "-0000.png";
		if (!QFile (filename_stream.str().c_str()).exists()) 
			break;
		series_nr++;
	}

	float duration = glWidget->animation_data->duration;
	float speedup = 100.f / static_cast<float>(spinBoxSpeed->value());
	float timestep;
	int image_count;

	if (fps_mode) {
		timestep = 1.f / fps / speedup;
		image_count = static_cast<int>(floor(duration / timestep));
	} else {
		timestep = duration / (fps - 1.f);
		image_count = static_cast<int>(roundf(fps));
	}

	QProgressDialog pbar("Rendering offscreen", "Abort Render", 0, image_count, this);
	pbar.setMinimumDuration(0);
	pbar.show();
	stringstream overlayFilename;
	overlayFilename << figure_name << "_" << setw(3) << setfill('0') << series_nr << "-overlay.png";

	for(int i = 0; i < image_count; i++) {
		pbar.setValue(i);
		pbar.show();

		float current_time = (float) i * timestep;

		filename_stream.str("");
		filename_stream << figure_name << "_" << setw(3) << setfill('0') << series_nr << "-" << setw(4) << setfill('0') << i << ".png";
		glWidget->animation_data->current_time = current_time;
		QImage image = glWidget->renderContentOffscreen (width, height, render_transparent);
		image.save (filename_stream.str().c_str(), 0, -1);

		if (doComposite) {
			string cmd("composite -compose plus ");
			if (i==0) {
				cmd="cp "+filename_stream.str()+" "+overlayFilename.str();
			} else {
				cmd="composite -compose plus "+filename_stream.str()+" "+overlayFilename.str()+" "+overlayFilename.str();
			}
			if (system(cmd.c_str()) == -1) {
				cerr << "Error occured when running command:" << endl;
				cerr << "  " << cmd << endl;
				abort();
			}
		}
	}
	if (doMencoder) {
		cout << "running mencoder to produce a movie" << endl;
		stringstream mencoder;
		mencoder << "mencoder mf://"  << figure_name << "_" << setw(3) << setfill('0') << series_nr << "-"<< "*.png ";
		mencoder << "-mf w=" << width << ":h="<< height << ":fps=" << fps << ":type=png -ovc lavc -lavcopts vcodec=mpeg4:mbd=2:trell -oac copy -o ";
		mencoder << figure_name << "_" << setw(3) << setfill('0') << series_nr << ".avi";
		
		cout << mencoder.str() << endl;
		
		if (system(mencoder.str().c_str()) == -1) {
			cerr << "Error occured when running command:" << endl;
			cerr << "  " << mencoder.str()<< endl;
			abort();
		}
	}
	
}
//Signal handling stuff

void MeshupApp::SIGUSR1Handler(int)
 {
     char a = 1;
     size_t length = ::write(sigusr1Fd[0], &a, sizeof(a));
 }

int setup_unix_signal_handlers()
 {
     struct sigaction usr1;

     usr1.sa_handler = MeshupApp::SIGUSR1Handler;
     sigemptyset(&usr1.sa_mask);
     usr1.sa_flags = 0;
     usr1.sa_flags |= SA_RESTART;

     if (sigaction(SIGUSR1, &usr1, 0) > 0)
        return 1;

  
     return 0;
 }

 void MeshupApp::handleSIGUSR1() {
	snUSR1->setEnabled(false);
	char tmp;

	size_t length = ::read(sigusr1Fd[1], &tmp, sizeof(tmp));

	action_reload_files();

	snUSR1->setEnabled(true);
}
int MeshupApp::sigusr1Fd[2];
