/*
 * MeshUp - A visualization tool for multi-body systems based on skeletal
 * animation and magic.
 *
 * Copyright (c) 2012-2018 Martin Felis <martin.felis@iwr.uni-heidelberg.de>
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
#include <algorithm>

#include "meshup_config.h"

#include "glwidget.h" 
#include "MeshupApp.h"
#include "Animation.h"
#include "ForcesTorques.h"
#include "Scene.h"
#include "Scripting.h"

#include <assert.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <fstream>
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>

#include "json/json.h"
#include "colorscale.h"

#include "QVideoEncoder.h"

#include "Model.h"

using namespace std;

Json::Value settings_json;

const double TimeLineDuration = 1000.;

MeshupApp::MeshupApp(QWidget *parent)
{
	setupUi(this); // this sets up GUI

	selected_cam = NULL;
	scene = new Scene;

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
	renderVideoDialog = new RenderVideoDialog(this);

	// this is NOT the default value,
	//  its just an initialization of the memory with something
	//  that makes sense
	glRefreshTime=20; 

	sceneRefreshTimer = new QTimer (this);
	sceneRefreshTimer->setSingleShot(false);
	updateTime.start();

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
	checkBoxDrawForces->setChecked (glWidget->draw_forces);
	checkBoxDrawTorques->setChecked (glWidget->draw_torques);

	//Setup Camera Operator
	cam_operator = new CameraOperator(listWidgetCameraList);
	cam_operator->addCamera(0.0, glWidget->cam, false);
	cam_operator->current_cam = glWidget->cam;
	glWidget->camera = &cam_operator->current_cam;

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

	// the sceneRefreshTimer is used to continously redraw the OpenGL widget
	connect (sceneRefreshTimer, SIGNAL(timeout()), this , SLOT(drawScene()));

	//camera interaction
	connect (listWidgetCameraList, SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)), this, SLOT(select_camera(QListWidgetItem*, QListWidgetItem*)));
	connect (listWidgetCameraList, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(camera_clicked(QListWidgetItem*)));

	// render dialogs
	connect (actionRenderImage, SIGNAL (triggered()), this, SLOT (actionRenderAndSaveToFile()));
	connect (actionRenderSeriesImage, SIGNAL (triggered()), this, SLOT (actionRenderSeriesAndSaveToFile()));
	connect (actionRenderVideo, SIGNAL(triggered()), this, SLOT(actionRenderVideoAndSaveToFile()));

	// view stettings
	connect (checkBoxDrawBaseAxes, SIGNAL (toggled(bool)), glWidget, SLOT (toggle_draw_base_axes(bool)));
	connect (checkBoxDrawFloor, SIGNAL (toggled(bool)), glWidget, SLOT (toggle_draw_floor(bool)));
	connect (checkBoxDrawFrameAxes, SIGNAL (toggled(bool)), glWidget, SLOT (toggle_draw_frame_axes(bool)));
	connect (checkBoxDrawGrid, SIGNAL (toggled(bool)), glWidget, SLOT (toggle_draw_grid(bool)));
	connect (checkBoxDrawMeshes, SIGNAL (toggled(bool)), glWidget, SLOT (toggle_draw_meshes(bool)));
	connect (checkBoxDrawShadows, SIGNAL (toggled(bool)), glWidget, SLOT (toggle_draw_shadows(bool)));
	connect (checkBoxDrawCurves, SIGNAL (toggled(bool)), glWidget, SLOT (toggle_draw_curves(bool)));
	connect (checkBoxDrawPoints, SIGNAL (toggled(bool)), glWidget, SLOT (toggle_draw_points(bool)));
	connect (checkBoxDrawForces, SIGNAL (toggled(bool)), glWidget, SLOT (toggle_draw_forces(bool)));	
	connect (checkBoxDrawTorques, SIGNAL (toggled(bool)), glWidget, SLOT (toggle_draw_torques(bool)));	
	connect (checkBoxCameraFixed, SIGNAL (toggled(bool)), this, SLOT (toggle_camera_fix(bool)));	
	connect (actionToggleWhiteBackground, SIGNAL (toggled(bool)), glWidget, SLOT (toggle_white_mode(bool)));

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
	connect (actionLoadForcesAndTorques, SIGNAL ( triggered() ), this, SLOT(action_load_forces()));
	connect (pushButtonLoadCameraFile, SIGNAL (clicked()), this, SLOT (action_load_camera()));

	connect (actionReloadFiles, SIGNAL ( triggered() ), this, SLOT(action_reload_files()));

	connect (glWidget, SIGNAL (camera_changed()), this, SLOT (camera_changed()));	
	connect (glWidget, SIGNAL (toggle_camera_fix(bool)), this, SLOT (toggle_camera_fix(bool)));	
	connect (glWidget, SIGNAL (start_draw()), this, SLOT (update_camera()));
	connect (lineEditCameraEye, SIGNAL (editingFinished()), this, SLOT (set_camera_pos()));
	connect (lineEditCameraCenter, SIGNAL (editingFinished()), this, SLOT (set_camera_pos()));

	connect (pushButtonUpdateCamera, SIGNAL (clicked()), this, SLOT (set_camera_pos()));

	connect (glWidget, SIGNAL (opengl_initialized()), this, SLOT (opengl_initialized()));

	loadSettings();
	
	sceneRefreshTimer->start(glRefreshTime);
}

void MeshupApp::opengl_initialized () {
	glWidget->scene = scene;

	parseArguments (main_argc, main_argv);
}

void MeshupApp::drawScene () {
	if (L)
		scripting_update (L, 1.0e-3f * static_cast<float>(updateTime.restart()) );

	scene->setCurrentTime(scene->current_time);
	glWidget->updateGL();

	if (L)
		scripting_draw (L);
}

void MeshupApp::loadModel(const char* filename) {
	if (glWidget->scene == NULL) {
		model_files_queue.push_back (filename);
		return;
	}

	MeshupModel* model = new MeshupModel;
	// TODO: gracefully ignore erroneous files
	model->loadModelFromFile (filename);
	model->resetPoses();
	model->updateSegments();
	
	scene->models.push_back (model);
}

void MeshupApp::loadAnimation(const char* filename) {
	if (glWidget->scene == NULL) {
		animation_files_queue.push_back (filename);
		return;
	}

	if (scene->models.size() == 0) {
		std::cerr << "Error: could not load Animation without a model!" << std::endl;
		abort();
	}

	if (scene->models.size() == scene->animations.size()) {
		// no model given for this animation therefore copy the previous model
		// for this animation
		loadModel(scene->models[scene->models.size() - 1]->model_filename.c_str());
	}

	Animation* animation = new Animation();
	// TODO: gracefully ignore erroneous files
	animation->loadFromFile (filename, scene->models[scene->models.size() - 1]->configuration);
	scene->animations.push_back (animation);
	scene->longest_animation = std::max (scene->longest_animation, animation->duration);

	unsigned int i = scene->animations.size() - 1;
	UpdateModelFromAnimation (scene->models[i], scene->animations[i], scene->current_time);

 	initialize_curves(); 
}

void MeshupApp::loadForcesAndTorques(const char* filename) {
	 if (glWidget->scene == NULL) {
		force_files_queue.push_back (filename);
		return;
	 }

	 if (scene->models.size() == 0 || scene-> animations.size() == 0) {
		std::cerr << "Error: could not load Forces and Torques without a model and animation!" << std::endl;
		abort();
	 }

	 if (scene->forcesTorquesQueue.size() == scene->animations.size()) {
		std::cerr << "Error: There has to be an animation for every force file. Old animations cant be used" << std::endl;
		abort();
	 }

	unsigned int model_num = std::min(scene->models.size()-1, scene->forcesTorquesQueue.size());

	 ForcesTorques* forcesTorques = new ForcesTorques(scene->models[model_num]);

	 forcesTorques->loadFromFile (filename);
	 if( (!forcesTorques->times.empty()) != (glWidget->draw_forces || glWidget->draw_torques)){
		glWidget->draw_forces = true;
		glWidget->draw_torques = true;
		checkBoxDrawForces->setChecked(glWidget->draw_forces);
		checkBoxDrawTorques->setChecked(glWidget->draw_torques);
	 }
	 scene->forcesTorquesQueue.push_back (forcesTorques);
}

void MeshupApp::loadCamera(const char* filename) {
	if (glWidget->scene != NULL) {
		cam_operator->loadFromFile(filename); 
	}
}

void MeshupApp::setAnimationFraction (float fraction) {
	float set_time = fraction * scene->longest_animation;
	scene->setCurrentTime(set_time);
	if (selected_cam != NULL && !playerPaused) {
		selected_cam->camera_data->time = set_time;
		selected_cam->data_changed();
	}
}

void print_usage() {
	cout << "Usage: meshup [model_file(s)] [animation_file(s)] [-s script.lua [args] ] " << endl
		<< "Visualization tool for multi-body systems based on skeletal animation and magic." << endl
		<< endl
		<< "-s, --script FILE	 use the provided FILE as script. See doc/scripting/" << endl
		<< "				 for examples and documentation. Note that any re-" << endl
		<< "				 maining arguments will be sent to the meshup.load(args)" << endl
		<< "				 script function." << endl
		<< endl
		<< "Report bugs to <martin.felis@iwr.uni-heidelberg.de>" << endl;
}

void MeshupApp::parseArguments (int argc, char* argv[]) {
	string scripting_file = "";

	for (int i = 1; i < argc; i++) {

		// check if diplaying help was part of input
		if (string(argv[i]) == "--help"
				|| string(argv[i]) == "-h") {
			print_usage();
			exit(1);
		}

		string arg = argv[i];
		string arg_extension = "";

		if (arg.find (".") != std::string::npos) 
			arg_extension = arg.substr (arg.rfind(".") + 1);

		// check if there is a scripting file included
		if (arg == "-s" || arg == "--script") {
			i++;
			if (i == argc) {
				cerr << "Error: no scripting file provided!" << endl;
				abort();
			}

			arg = argv[i];
			if (arg.size() < 3 || arg.substr (arg.size() - 3) != "lua") {
				cerr << "Error: invalid scripting file! Must be a .lua file." << endl;
				abort();
			}

			scripting_file = arg;

		// In case arg is model file
		} else if (arg.size() >= 3 && arg.substr (arg.size() - 3) == "lua") {
			string model_filename = find_model_file_by_name (arg.c_str());
			if (model_filename.size() != 0) {
				loadModel(model_filename.c_str());
			}

		// In case arg is animation file
		} else if (arg.size() >= 3 && (( arg_extension == "csv") || (arg_extension == "txt"))) {
			loadAnimation (arg.c_str());
		
		// In case arg is force file
		} else if (arg.size() >= 3 && ( arg_extension == "ff")) {
			loadForcesAndTorques (arg.c_str());	
		}
	}

	if (scripting_file != "") {
		cout << "Initialize scripting file " << scripting_file << endl;
		scripting_init (this, scripting_file.c_str());
	} else {
		scripting_init (this, NULL);
	}

	if (L) {
		int script_args_start = argc;
		for (int i = 0; i < argc; i++) {
			if (i < argc -2 && (string("-s") == argv[i] || string("--script") == argv[i])) {
				script_args_start = i + 2;
				break;
			}
		}
		scripting_load (L, argc - script_args_start, &argv[script_args_start]);
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
	settings_json["configuration"]["view"]["draw_forces"] = checkBoxDrawForces->isChecked();
	settings_json["configuration"]["view"]["draw_torques"] = checkBoxDrawTorques->isChecked();
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
	settings_json["configuration"]["render"]["fps"]	 = renderImageSeriesDialog->FpsSpinBox->value();

	settings_json["configuration"]["render"]["fps_mode"]		 = renderImageSeriesDialog->fpsModeRadioButton->isChecked();
	settings_json["configuration"]["render"]["frame_count_mode"] = renderImageSeriesDialog->frameCountModeRadioButton->isChecked();
	settings_json["configuration"]["render"]["composite"]		= renderImageSeriesDialog->compositeBox->isChecked();
	settings_json["configuration"]["render"]["transparent"]	 = renderImageSeriesDialog->transparentBackgroundCheckBox->isChecked();

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
	checkBoxDrawForces->setChecked(settings_json["configuration"]["view"].get("draw_forces", glWidget->draw_forces).asBool());
	checkBoxDrawTorques->setChecked(settings_json["configuration"]["view"].get("draw_torques", glWidget->draw_torques).asBool());
	glWidget->toggle_draw_orthographic(settings_json["configuration"]["view"].get("draw_orthographic", (*glWidget->camera)->orthographic).asBool());

	dockViewSettings->setVisible(settings_json["configuration"]["docks"]["view_settings"].get("visible", false).asBool());
	dockCameraControls->setVisible(settings_json["configuration"]["docks"]["camera_controls"].get("visible", false).asBool());
	dockPlayerControls->setVisible(settings_json["configuration"]["docks"]["player_controls"].get("visible", true).asBool());
	checkBoxLoopAnimation->setChecked(settings_json["configuration"]["docks"]["player_controls"].get("repeat", true).asBool());

	renderImageSeriesDialog->WidthSpinBox->setValue(settings_json["configuration"]["render"].get("width", glWidget->width()).asInt());
	renderImageSeriesDialog->HeightSpinBox->setValue(settings_json["configuration"]["render"].get("height", glWidget->height()).asInt());
	renderImageSeriesDialog->FpsSpinBox->setValue(settings_json["configuration"]["render"].get("fps", 25).asInt());

	renderImageSeriesDialog->fpsModeRadioButton->setChecked(settings_json["configuration"]["render"].get("fps_mode", true).asBool());
	renderImageSeriesDialog->frameCountModeRadioButton->setChecked(settings_json["configuration"]["render"].get("frame_count_mode", false).asBool());
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
	bool fixed = cam_operator->fixed;
	cam_operator->setCamHeight((*glWidget->camera)->height);
	cam_operator->setCamWidth((*glWidget->camera)->width);

	unsigned int digits = 2;

	stringstream center_stream ("");
	center_stream << std::fixed << std::setprecision(digits) << center[0] << ", " << center[1] << ", " << center[2];

	stringstream eye_stream ("");
	eye_stream << std::fixed << std::setprecision(digits) << eye[0] << ", " << eye[1] << ", " << eye[2];

	lineEditCameraEye->setText (eye_stream.str().c_str());
	lineEditCameraCenter->setText (center_stream.str().c_str());
	checkBoxCameraFixed->setChecked(fixed);
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

void MeshupApp::set_camera_pos() {
	string center_string = lineEditCameraCenter->text().toStdString();
	Vector3f poi = parse_vec3_string (center_string);

	string eye_string = lineEditCameraEye->text().toStdString();
	Vector3f eye = parse_vec3_string (eye_string);

	glWidget->setCameraPoi(poi);
	glWidget->setCameraEye(eye);
}

void MeshupApp::update_camera() {
	cam_operator->updateCamera(scene->current_time);
}

void MeshupApp::toggle_camera_fix(bool status) {
	cam_operator->setFixed(status);
}

void MeshupApp::select_camera(QListWidgetItem* current, QListWidgetItem* previous){
	selected_cam = (CameraListItem*) current;
	CameraPosition* cam_data = selected_cam->camera_data;
	cam_operator->setFixAtCam(cam_data->cam);
	camera_changed();
	scene->setCurrentTime(cam_data->time);
	toggle_play_animation(false);
	update_time_widgets();
}

void MeshupApp::camera_clicked(QListWidgetItem* item) {
	CameraListItem* clicked_item = (CameraListItem*) item;
	if ( clicked_item == selected_cam) {
		selected_cam = NULL;
		item->setSelected(false);
	}
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
		loadModel (file_dialog.selectedFiles().at(0).toStdString().c_str());
	}	
}

void MeshupApp::action_load_animation() {
	QFileDialog file_dialog (this, "Select Animation File");

	file_dialog.setNameFilter(tr("MeshupAnimation (*.txt *.csv)"));
	file_dialog.setFileMode(QFileDialog::ExistingFile);

	if (file_dialog.exec()) {
		loadAnimation (file_dialog.selectedFiles().at(0).toStdString().c_str());
	}	
}

void MeshupApp::action_load_forces() {
	QFileDialog file_dialog (this, "Select Force/Torque File");

	file_dialog.setNameFilter(tr("MeshupForces (*.ff)"));
	file_dialog.setFileMode(QFileDialog::ExistingFile);

	if (file_dialog.exec()) {
		loadForcesAndTorques(file_dialog.selectedFiles().at(0).toStdString().c_str());
	}
}

void MeshupApp::action_load_camera() {
	QFileDialog file_dialog (this, "Select Camera File");

	file_dialog.setNameFilter(tr("MeshupCamera (*.cam)"));
	file_dialog.setFileMode(QFileDialog::ExistingFile);

	if (file_dialog.exec()) {
		loadCamera(file_dialog.selectedFiles().at(0).toStdString().c_str());
	}
}

void MeshupApp::action_reload_files() {
	for (unsigned int i = 0; i < scene->models.size(); i++) {
		string filename = scene->models[i]->model_filename;
		MeshupModel* model = scene->models[i];
		model->clear();

		if (model->loadModelFromFile (filename.c_str())) {
			model->resetPoses();
			model->updateSegments();
		} else {
			cerr << "Error loading model " << scene->models[i]->model_filename << endl;
		}
	}

	for (unsigned int i = 0; i < scene->animations.size(); i++) {
		Animation* animation = scene->animations[i];

		if (!animation->loadFromFile (animation->animation_filename.c_str(), scene->models[i]->configuration)) {
			cerr << "Error loading animation " << scene->animations[i]->animation_filename << endl;
		}
	}

	for (unsigned int i = 0; i < scene->forcesTorquesQueue.size(); i++){
		ForcesTorques* forcesTorques = scene->forcesTorquesQueue[i];

		if (!forcesTorques->loadFromFile(forcesTorques->forces_filename.c_str())){
			cerr << "Error loading forces " << scene->forcesTorquesQueue[i]->forces_filename << endl;
		}
	}

 	initialize_curves(); 

	emit (animation_loaded());
	
	return;
}

void MeshupApp::action_quit () {
	saveSettings();
	qApp->quit();
}

void MeshupApp::animation_loaded() {
	qDebug() << __func__;

	for (unsigned int i = 0; i < scene->models.size(); i++) {
		scene->models[i]->resetPoses();
		scene->models[i]->updateFrames();
	}

	for (unsigned int i = 0; i < scene->animations.size(); i++) {
		UpdateModelFromAnimation (scene->models[i], scene->animations[i], scene->current_time);
	}
}

void MeshupApp::initialize_curves() {
	float curve_frame_rate = 100.f;

	float old_time = scene->current_time;

	// qDebug() << "duration = " << scientific << duration << endl;
	// cout << "time_step = " << scientific << time_step << endl;

	for (unsigned int i = 0; i < scene->models.size(); i++) {
		scene->models[i]->clearCurves();
	}

	for (unsigned int i = 0; i < scene->animations.size(); i++) {
		float current_time = 0.f;
		float duration = scene->animations[i]->duration;
		float time_step = duration / curve_frame_rate;

		while (1) {
			float fraction = current_time / duration * 2.f - 1.f;

			UpdateModelFromAnimation (scene->models[i], scene->animations[i], current_time);
			scene->models[i]->updateFrames();
			MeshupModel::FrameMap::iterator frame_iter = scene->models[i]->framemap.begin();

			for (frame_iter; frame_iter != scene->models[i]->framemap.end(); frame_iter++) {
				Matrix44f pose_matrix = frame_iter->second->pose_transform;
				Vector3f pose_translation (
						pose_matrix (3,0),
						pose_matrix (3,1),
						pose_matrix (3,2)
						);

				scene->models[i]->addCurvePoint (frame_iter->first,
						pose_translation,
						Vector3f (
							colorscale::red(fraction),
							colorscale::green(fraction),
							colorscale::blue(fraction))
						);
			}

			if (current_time == duration)
				break;

			current_time += time_step;
			if (current_time > duration)
				current_time = duration;
		}
	}

	scene->current_time = old_time;
	// qDebug() << "initializing curves done";
}

/** \brief Modifies the widgets to show the current time
 */
void MeshupApp::timeline_frame_changed (int frame_index) {
//	qDebug () << __func__ << " frame_index = " << frame_index;

	static bool repeat_gate = false;

	if (!repeat_gate) {
		repeat_gate = true;

		setAnimationFraction (static_cast<float>(frame_index) / TimeLineDuration);
		
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
		timeLine->setCurrentTime (frame_index * scene->longest_animation);

		repeat_gate = false;
	}
	setAnimationFraction (static_cast<float>(frame_index) / TimeLineDuration);
}

void MeshupApp::timeslider_value_changed (int frame_index) {
	float current_time = static_cast<float>(frame_index) / TimeLineDuration * scene->longest_animation;
	
	int num_seconds = static_cast<int>(floor(current_time));
	int num_milliseconds = static_cast<int>(round((current_time - num_seconds) * 1000.f));

	stringstream time_string("");
	time_string << num_seconds << "." << setw(3) << setfill('0') << num_milliseconds;
	timeLabel->setText(time_string.str().c_str());

	setAnimationFraction (static_cast<float>(frame_index) / TimeLineDuration);
}

void MeshupApp::update_time_widgets () {
//	qDebug() << __func__;
	if (scene->animations.size() > 0 && scene->longest_animation > 0.) {
		double time_fraction = scene->current_time / scene->longest_animation;
		int frame_index = static_cast<int>(round(time_fraction * TimeLineDuration));

		horizontalSliderTime->setValue (frame_index);
		timeLine->setDuration (scene->longest_animation * TimeLineDuration / (spinBoxSpeed->value() / 100.0));
		setAnimationFraction (static_cast<float>(frame_index) / TimeLineDuration);
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

	glWidget->saveScreenshot (filename_stream.str().c_str(), w, h, renderImageSeriesDialog->transparentBackgroundCheckBox->isChecked());
}

void MeshupApp::actionRenderSeriesAndSaveToFile () {
	int fps;
	bool fps_mode;
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

	float duration = scene->longest_animation;
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
	pbar.setWindowModality(Qt::WindowModal);
	pbar.setMinimumDuration(0);

	stringstream overlayFilename;
	overlayFilename << figure_name << "_" << setw(3) << setfill('0') << series_nr << "-overlay.png";

	for(int i = 0; i < image_count; i++) {
		pbar.setValue(i);

		float current_time = (float) i * timestep;

		filename_stream.str("");
		filename_stream << figure_name << "_" << setw(3) << setfill('0') << series_nr << "-" << setw(4) << setfill('0') << i << ".png";
		scene->setCurrentTime (current_time);
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
		if (pbar.wasCanceled()) {
			qDebug() << "canceled!";
			return;
		}
	}
	pbar.setValue(image_count);
}

void MeshupApp::actionRenderVideoAndSaveToFile () {
	unsigned width;
	unsigned height;
	unsigned length;
	unsigned fps = 25;
	QString filename;

	int result = renderVideoDialog->exec();

	if (result == QDialog::Rejected)
		return;

	width = renderVideoDialog->WidthSpinBox->value();
	height = renderVideoDialog->HeightSpinBox->value();
	length = renderVideoDialog->TimeSpinBox->value(); 
	filename = renderVideoDialog->videoName->text();

	float duration = scene->longest_animation;
	unsigned frame_count = fps * length + 27;
	float timestep = duration / frame_count;

	static_cast<int>(floor(duration / timestep));

	QProgressDialog pbar("Rendering offscreen", "Abort Render", 0, frame_count, this);
	pbar.setWindowModality(Qt::WindowModal);
	pbar.setMinimumDuration(0);
	QVideoEncoder encoder;
	encoder.createFile(filename, width, height, fps);

	for(int i = 0; i < frame_count; i++) {
		pbar.setValue(i);

		float current_time = (float) i * timestep;
		scene->setCurrentTime (current_time);

		QImage image = glWidget->renderContentOffscreen (width, height, false);
		encoder.encodeImage(image);

		if (pbar.wasCanceled()) {
			qDebug() << "canceled!";
			return;
		}
	}
	pbar.setValue(frame_count);

	encoder.close();
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
