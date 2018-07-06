// CameraAnimate.cc
// implementation of camera animation

#include "CameraOperator.h"
#include "MeshVBO.h"
#include "string_utils.h"
#include <iostream>
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

	double force_fps_previous_frame = 0.;
	int force_fps_frame_count = 0;
	bool last_line = false;

	//reset variables
	current_cam = mobile_cam;
	fixed = true;
	times.clear();
	pois.clear();
	eyes.clear();
	smooth.clear();
	for (int i=0; i<cams.size(); i++) {
		delete cams[i];
	}
	cams.clear();

	string filename_str (filename);

	cout << "Loading camera " << filename << endl;

	string previous_line;
	string line;

	bool found_column_section = false;
	bool found_data_section = false;
	bool column_section = false;
	bool data_section = false;
	bool csv_mode = false;
	int column_number = 0;
	int line_number = 0;

	while (!file_in.eof()) {
		previous_line = line;
		getline (file_in, line);

		// make sure the last line is read no matter what
		if (file_in.eof()) {
			last_line = true;
			line = previous_line;
		} else {
			line_number++;
		}

		line = strip_comments (strip_whitespaces( (line)));

		// skip lines with no information
		if (line.size() == 0)
			continue;

		// check whether we have a CSV file with just the data
		if (!found_column_section && !found_data_section) {
			float value = 0.f;
			istringstream value_stream (line.substr (0,
											line.find_first_of (" \t\r") - 1));
			value_stream >> value;
			if (!value_stream.fail()) {
				data_section = true;
				found_data_section = true;
				column_section = false;
				csv_mode = true;
			}
		}

		if (line.substr (0, string("COLUMNS:").size()) == "COLUMNS:" && !csv_mode) {
			found_column_section = true;
			column_section = true;

			// we set it to 0, because in the end we want
			// the actual number of columns
			column_number = 0;

			line = strip_comments (strip_whitespaces
				(line.substr(string("COLUMNS:").size(), line.size())));
			if (line.size() == 0)
				continue;
		}

			if (line.substr (0, string("DATA:").size()) == "DATA:"&& !csv_mode) {
			found_data_section = true;
			column_section = false;
			data_section = true;
			continue;
		}
		else if (!data_section && line.substr (0,string("DATA_FROM:").size()) == "DATA_FROM:" && !csv_mode) {
			file_in.close();

			boost::filesystem::path data_path (strip_whitespaces
						 (line.substr(string("DATA_FROM:").size(), line.size())));

			// search for the file in the same directory as the original file,
			// unless we have an absolutue path or by starting the file
			// with "./" or ".." to expicilty want the file loaded relative to
			// the current work directory.
			if (data_path.string()[0] != '/' && data_path.string()[0] != '.') {
				boost::filesystem::path file_path (filename);
				boost::filesystem::path data_directory = file_path.parent_path();
				data_path = data_directory /= data_path;
			}

			file_in.open(data_path.string().c_str());
			cout << "Loading camera data from " << data_path.string() << endl;

			if (!file_in) {
				cerr << "Error opening camera file " << data_path.string() <<
									"!" << std::endl;

				if (strict)
					exit (1);

				return false;
			}

			filename_str = data_path.string();
			line_number = 0;

			found_data_section = true;
			column_section = false;
			data_section = true;
			continue;
		}

		if (column_section && !csv_mode) {
			std::vector<string> elements;

			elements = tokenize_csv_strip_whitespaces (line);

			for (int ei = 0; ei < elements.size(); ei++) {
				// skip elements that had multiple spaces in them
				if (elements[ei].size() == 0)
					continue;

				// it's safe to increase the column index here, as we did
				// initialize it with -1
				column_number++;

				string column_name = strip_whitespaces(elements[ei]);

				if (tolower(column_name) == "time") {
					if (column_number != 1) {
						cerr << "Error: first column must be time column (it was column "
							 << ei <<")!" << endl;
						abort();
					}
					continue;
				}

			}
			continue;
		}

		if (data_section) {
		std::vector<string> data_columns;
		if (csv_mode)
			data_columns = tokenize_csv_strip_whitespaces (line);
		else
			data_columns = tokenize (line);

			float state_time;
			float value;
			istringstream value_stream (data_columns[0]);
			if (!(value_stream >> value)) {
				cerr << "Error: could not convert value string '" <<
						value_stream.str() << "' into a number in " <<
						filename << ":" << line_number << "." << endl;
				abort();
			}
			state_time = value;

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
			addCameraPos(state_values);

			force_fps_previous_frame = state_time;
			force_fps_frame_count++;

			if (state_time > duration)
				duration = state_time;

			continue;
		}
	}

	if (!found_data_section) {
		cerr << "Error: did not find DATA: section in camera file!"
			 << endl;
		abort();
	}

	camera_filename = filename;

	return true;
}

void CameraOperator::addCameraPos(VectorNd data) {
	float time = data[0];
	Vector3f poi(data[1], data[2], data[3]);
	Vector3f eye(data[4], data[5], data[6]);

	Camera* cam = new Camera();
	cam->height = height;
	cam->width = width;
	cam->poi = poi;
	cam->eye = eye;
	cam->updateSphericalCoordinates();

	times.push_back(time);
	pois.push_back(poi);
	eyes.push_back(eye);
	smooth.push_back(true);
	cams.push_back(cam);

	if (times.size() > 1) {
		fixed = false;
	}
}

void CameraOperator::setCamHeight(int height) {
	this->height = height; 
	for ( int i=0; i<cams.size(); i++) {
		cams[i]->height = height;
	}
}

void CameraOperator::setCamWidth(int width) {
	this->width = width;
	for ( int i=0; i<cams.size(); i++) {
		cams[i]->width = width;
	}
}

bool CameraOperator::updateCamera(float current_time) {
	Camera* old = current_cam;
	//if camera is fixed do not update camera
	if (!fixed) {
		int frame_index;
		// Determine the right camera entry
		for(int i=0; i<times.size(); i++) {
			frame_index = i;
			if ( (i+1) < times.size() && current_time < times[i+1]) {
				break;
			}
		}
		if (frame_index == times.size() - 1) {
			current_cam = cams[frame_index];
		// check if smooth transition to next camera position is to be done 
		// if not just update camera otherwise calculate poisition of transitioning camera
		} else if ( !smooth[frame_index+1] ) {
			current_cam = cams[frame_index];
		} else {
			float time_curr_cam = times[frame_index];
			float time_next_cam = times[frame_index + 1];
			float fraq = (current_time - time_curr_cam) / (time_next_cam - time_curr_cam);
			Vector3f poi = (cams[frame_index]->poi * (1-fraq)) + (cams[frame_index+1]->poi * fraq);
			Vector3f eye = (cams[frame_index]->eye * (1-fraq)) + (cams[frame_index+1]->eye * fraq);
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
