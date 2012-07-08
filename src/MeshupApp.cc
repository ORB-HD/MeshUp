#include <QtGui> 
#include <QFile>
#include <QDir>
#include <QFileDialog>
#include <QProgressDialog>

#include "glwidget.h" 
#include "MeshupApp.h"
#include "Animation.h"
#include "AnimationEditModel.h"

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
	renderImageSeriesDialog = new RenderImageSeriesDialog(this);

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
	
	spinBoxSpeed->setMinimum(1);
	spinBoxSpeed->setMaximum(1000);
	spinBoxSpeed->setValue(100);
	spinBoxSpeed->setSingleStep(5);

	horizontalSliderTime->setMinimum(0);
	horizontalSliderTime->setMaximum(1000.);
	horizontalSliderTime->setSingleStep(1);

	checkBoxDrawBaseAxes->setChecked (glWidget->draw_base_axes);
	checkBoxDrawFloor->setChecked (glWidget->draw_floor);
	checkBoxDrawFrameAxes->setChecked (glWidget->draw_frame_axes);
	checkBoxDrawGrid->setChecked (glWidget->draw_grid);
	checkBoxDrawMeshes->setChecked (glWidget->draw_meshes);
	checkBoxDrawShadows->setChecked (glWidget->draw_shadows);
	checkBoxDrawCurves->setChecked (glWidget->draw_curves);

	// animation editor
	animationValuesTableView->setModel (new AnimationEditModel(NULL));

	// player is paused on startup
	playerPaused = true;

	dockPlayerControls->setVisible(true);
	dockViewSettings->setVisible(false);
	dockAnimationEditor->setVisible(false);

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

	// animation editor
	connect (toolButtonKeyFrameNext, SIGNAL( pressed() ), this, SLOT( action_next_keyframe() ));
	connect (toolButtonKeyFramePrev, SIGNAL( pressed() ), this, SLOT( action_prev_keyframe() ));

	// keyboard shortcuts
	connect (actionLoadModel, SIGNAL ( triggered() ), this, SLOT(action_load_model()));
	connect (actionLoadAnimation, SIGNAL ( triggered() ), this, SLOT(action_load_animation()));
	connect (actionReloadFiles, SIGNAL ( triggered() ), this, SLOT(action_reload_files()));

	loadSettings();
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

	settings_json["configuration"]["docks"]["view_settings"]["visible"] = dockViewSettings->isVisible();
	settings_json["configuration"]["docks"]["player_controls"]["visible"] = dockPlayerControls->isVisible();
	settings_json["configuration"]["docks"]["player_controls"]["repeat"] = checkBoxLoopAnimation->isChecked();
	settings_json["configuration"]["docks"]["animationeditor_settings"]["visible"] = dockAnimationEditor->isVisible();

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

	dockViewSettings->setVisible(settings_json["configuration"]["docks"]["view_settings"].get("visible", false).asBool());
	dockPlayerControls->setVisible(settings_json["configuration"]["docks"]["player_controls"].get("visible", true).asBool());
	checkBoxLoopAnimation->setChecked(settings_json["configuration"]["docks"]["player_controls"].get("repeat", true).asBool());
	dockAnimationEditor->setVisible(settings_json["configuration"]["docks"]["animationeditor_settings"].get("visible", false).asBool());

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

	status = test_animation->loadFromFile(animation_filename.c_str(), false);
	if (!status) {
		cerr << "Reloading of animation '" << animation_filename.c_str() << "' failed!";
		return;
	}

	// everything worked fine -> replace the current model
	glWidget->animation_data = test_animation;

	return;
}

void MeshupApp::action_quit () {
	saveSettings();
	qApp->quit();
}

void MeshupApp::action_next_keyframe() {
	glWidget->animation_data->current_time = glWidget->animation_data->getNextKeyFrameTime();
}

void MeshupApp::action_prev_keyframe() {
	glWidget->animation_data->current_time = glWidget->animation_data->getPrevKeyFrameTime();
}

void MeshupApp::timeline_frame_changed (int frame_index) {
//	qDebug () << __func__ << " frame_index = " << frame_index;

	horizontalSliderTime->setValue (frame_index);

	timeLine->setDuration (glWidget->getAnimationDuration() * 1000.f /(spinBoxSpeed->value()/100.0));
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
	static int fps=25;
	static bool doMencoder=true;
	static bool doComposite=true;
	static bool render_transparent=false;

	renderImageSeriesDialog->WidthSpinBox->setValue(glWidget->width());
	renderImageSeriesDialog->HeightSpinBox->setValue(glWidget->height());
	renderImageSeriesDialog->FpsSpinBox->setValue(fps);
	renderImageSeriesDialog->mencoderBox->setChecked(doMencoder);

	int result = renderImageSeriesDialog->exec();

	if (result == QDialog::Rejected)
		return;

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


	//~ cout << "starting offscreen rendering (this may take a while)..." << endl;
	int w = renderImageSeriesDialog->WidthSpinBox->value();
	int h = renderImageSeriesDialog->HeightSpinBox->value();
	fps = renderImageSeriesDialog->FpsSpinBox->value();
	
	QProgressDialog pbar("Rendering offscreen", "Abort Render", 0, fps*glWidget->animation_data->duration* 100.0 / spinBoxSpeed->value(), this);
	pbar.setMinimumDuration(0);
	pbar.show();
	stringstream overlayFilename;
	overlayFilename << figure_name << "_" << setw(3) << setfill('0') << series_nr << "-overlay.png";
	
	for(int i = 0; i < (float) fps*glWidget->animation_data->duration* 100.0 / spinBoxSpeed->value(); i++) {
		pbar.setValue(i);
		pbar.show();
		filename_stream.str("");
		filename_stream << figure_name << "_" << setw(3) << setfill('0') << series_nr << "-" << setw(4) << setfill('0') << i << ".png";
		glWidget->animation_data->current_time = (float) i / fps*  spinBoxSpeed->value() / 100.0;
		QImage image = glWidget->renderContentOffscreen (w,h,render_transparent);
		image.save (filename_stream.str().c_str(), 0, -1);
		//not used:
		//if (pbar.wasCanceled())
		//	return;
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
		mencoder << "-mf w=" << w << ":h="<< h << ":fps=" << fps << ":type=png -ovc lavc -lavcopts vcodec=mpeg4:mbd=2:trell -oac copy -o ";
		mencoder << figure_name << "_" << setw(3) << setfill('0') << series_nr << ".avi";
		
		cout << mencoder.str() << endl;
		
		if (system(mencoder.str().c_str()) == -1) {
			cerr << "Error occured when running command:" << endl;
			cerr << "  " << mencoder.str()<< endl;
			abort();
		}
	}
	
}
