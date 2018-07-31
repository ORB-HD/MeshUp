// CameraAnimate.cc
// implementation of camera animation

#include "CameraOperator.h"
#include "MeshVBO.h"
#include "string_utils.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <boost/filesystem.hpp>


using namespace std;

bool CameraOperator::loadFromFile (const char* filename, bool strict) {
	ifstream file_in (filename);

	if (!file_in) {
		cerr << "Error opening cam file " << filename << "!";

		if (strict)
			exit (1);

		return false;
	}

	// configuration = frame_config;
	width = current_cam->width;
	height = current_cam->height;

	double force_fps_previous_frame = 0.;
	int force_fps_frame_count = 0;
	bool last_line = false;

	//reset variables
	current_cam = mobile_cam;
	fixed = true;
	for (int i=0; i<cam_pos.size(); i++) {
		delete cam_pos[i]->cam;
		delete cam_pos[i];
	}
	cam_pos.clear();
	camera_display->clear();

	string filename_str (filename);

	cout << "Loading camera " << filename << endl;

	string previous_line;
	string line;

	int line_number = 0;

	while (!file_in.eof()) {
		previous_line = line;
		getline (file_in, line);

		line_number++;
		line = strip_comments (strip_whitespaces( (line)));

		// skip lines with no information
		if (line.size() == 0)
			continue;

		std::vector<string> data_columns;
		data_columns = tokenize_csv_strip_whitespaces (line);

		float state_time;
		float value;

		// convert the data to raw values
		VectorNd state_values (VectorNd::Zero (data_columns.size()));
		for (int ci = 0; ci < data_columns.size(); ci++) {
			istringstream value_stream (data_columns[ci]);
			if (!(value_stream >> value)) {
				cerr << "Error: could not convert value string '"
					 << value_stream.str() << "' into a number in "
					 << filename << ":" << line_number << "." << endl;
				abort();
			}
			state_values[ci] = value;
		}
		state_time = state_values[0];
		addCameraPos(state_values);

		force_fps_previous_frame = state_time;
		force_fps_frame_count++;

		if (state_time > duration)
			duration = state_time;

		continue;
	}

	camera_filename = filename;

	return true;
}

void CameraOperator::addCameraPos(VectorNd data) {
	float time = data[0];
	Vector3f poi(data[1], data[2], data[3]);
	Vector3f eye(data[4], data[5], data[6]);

	Camera* cam = new Camera();
	cam->poi = poi;
	cam->eye = eye;
	cam->width = this->width;
	cam->height = this->height;
	cam->updateSphericalCoordinates();

	addCamera(time, cam, true);
}

QListWidgetItem* CameraOperator::addCamera(float time, Camera* cam, bool moving) {
	CameraPosition* camdata = new CameraPosition();
	camdata->cam = cam;
	camdata->time = time;
	camdata->moving = moving;
	int position = 0;
	std::vector<CameraPosition*>::iterator it;
	it = cam_pos.begin();
	for (;position<cam_pos.size(); position++) {
		if ( cam_pos[position]->time > time) {
			break;
		}
		it++;
	}
	cam_pos.insert(it, camdata);

	if (position == 0) {
		camdata->moving = false;
	}

	CameraListItem* display = new CameraListItem(camdata, camera_display);
	camera_display->insertItem(position, display);

	if (cam_pos.size() > 1) {
		fixed = false;
	}

	return camera_display->item(position);
}

void CameraOperator::setCamHeight(int height) {
	this->height = height; 
	for ( int i=0; i<cam_pos.size(); i++) {
		cam_pos[i]->cam->height = height;
	}
	mobile_cam->height = height;
}

void CameraOperator::setCamWidth(int width) {
	this->width = width;
	for ( int i=0; i<cam_pos.size(); i++) {
		cam_pos[i]->cam->width = width;
	}
	mobile_cam->width = width;
}

bool CameraOperator::updateCamera(float current_time) {
	Camera* old = current_cam;
	//if camera is fixed do not update camera
	if (!fixed) {
		int frame_index = 0;
		// Determine the right camera entry
		for(int i=0; i<cam_pos.size(); i++) {
			frame_index = i;
			if ( (i+1) < cam_pos.size() && current_time < cam_pos[i+1]->time) {
				break;
			}
		}
		if (frame_index == cam_pos.size() - 1) {
			current_cam = cam_pos[frame_index]->cam;
		// check if smooth transition to next camera position is to be done 
		// if not just update camera otherwise calculate poisition of transitioning camera
		} else if ( !cam_pos[frame_index+1]->moving ) {
			current_cam = cam_pos[frame_index]->cam;
		} else {
			float time_curr_cam = cam_pos[frame_index]->time;
			float time_next_cam = cam_pos[frame_index + 1]->time;
			float fraq = (current_time - time_curr_cam) / (time_next_cam - time_curr_cam);
			Vector3f poi = (cam_pos[frame_index]->cam->poi * (1-fraq)) + (cam_pos[frame_index+1]->cam->poi * fraq);
			Vector3f eye = (cam_pos[frame_index]->cam->eye * (1-fraq)) + (cam_pos[frame_index+1]->cam->eye * fraq);
			mobile_cam->poi = poi;
			mobile_cam->eye = eye;
			mobile_cam->updateSphericalCoordinates();
			current_cam = mobile_cam;
			if (old->poi != current_cam->poi || old->eye != current_cam->eye) {
				return true;
			} else {
				return false;
			}
		}
	}
	if (old == current_cam) {
		return false;
	}
	return true;
}

void CameraOperator::setFixAtCam(Camera* cam) {
	fixed = true;
	current_cam = cam;
}

void CameraOperator::deleteCameraPos(CameraPosition* pos) {
	if (cam_pos.size() > 1) {
		int item_pos = 0;
		vector<CameraPosition*>::iterator it = cam_pos.begin();
		// Determine the right camera entry
		for(;item_pos<cam_pos.size(); item_pos++) {
			if (pos == cam_pos[item_pos] ) {
				break;
			}
			it++;
		}
		camera_display->takeItem(item_pos);
		cam_pos.erase(it);
		if (item_pos > 0) {
			item_pos--;
		}
		current_cam = cam_pos[item_pos]->cam;
		delete pos->cam;
		delete pos;
	}
}

int CameraOperator::setCameraPosTime(CameraPosition* pos, float time) {
	pos->time = time;
	int item_pos = 0;
	vector<CameraPosition*>::iterator it = cam_pos.begin();
	// Determine the right camera entry
	for(;it != cam_pos.end(); it++) {
		if (pos == *it ) {
			break;
		}
		item_pos++;
	}
	bool position_corrected = false;
	if (cam_pos.size() != 2) {
		while ( !position_corrected ) {
			position_corrected = true;
			if ( it == cam_pos.begin() ) {
				if ( (it+1) != cam_pos.end() && (*it)->time > (*(it+1))->time ) {
					cam_pos.insert(it+2, pos);
					cam_pos.erase(it);
					it++;
					item_pos++;
					position_corrected = false;
				}
			} else if ( it == cam_pos.end()-1 ) {
				if ( it != cam_pos.begin() && (*it)->time < (*(it-1))->time ){
					cam_pos.insert(it-1, pos);
					cam_pos.erase(it+1);
					it--;
					item_pos--;
					position_corrected = false;
				}
			} else if ( (it+1) != cam_pos.end() && (*it)->time > (*(it+1))->time ) {
				cam_pos.insert(it+2, pos);
				cam_pos.erase(it);
				it++;
				item_pos++;
				position_corrected = false;
			} else if ( it != cam_pos.begin() && (*it)->time < (*(it-1))->time ){
				cam_pos.insert(it-1, pos);
				cam_pos.erase(it+1);
				it--;
				item_pos--;
				position_corrected = false;
			}
		}
	} else {
		if ( it == cam_pos.begin() ) {
			if ( (*it)->time > (*(it+1))->time ) {
				*it = *(it+1);
				*(it+1) = pos;
				item_pos++;
			}
		} else if ( it == cam_pos.end()-1 ) {
			if ( (*it)->time < (*(it-1))->time ) {
				*it = *(it-1);
				*(it-1) = pos;
				item_pos--;
			}
		}
	}

	for(int i=0; i<cam_pos.size(); i++) {
		CameraListItem* item = (CameraListItem*)camera_display->item(i);
		item->camera_data = cam_pos[i];
		item->data_changed();
	}
	return item_pos;
}

void CameraOperator::setFixed(bool status) {
	if (fixed != status) {
		if (status) {
			mobile_cam->poi = current_cam->poi;
			mobile_cam->eye = current_cam->eye;
			mobile_cam->updateSphericalCoordinates();
			current_cam = mobile_cam;
		}
		fixed = status;
	}
}


void CameraListItem::data_changed() {
	std::ostringstream display;
	std::string mov;
	if (camera_data->moving) {
		mov = "moving";
	} else {
		mov = "static";
	}
	display << "Camera" << "\t" << std::setprecision(3) << camera_data->time << "\t" << mov;
	setText(display.str().c_str());
}

