#include "ForcesTorques.h"
#include "GL/glew.h"
#include "MeshVBO.h"
#include "string_utils.h"
#include <iostream>
#include <fstream>
#include <boost/filesystem.hpp>

extern "C"
{
   #include <lua.h>
   #include <lauxlib.h>
   #include <lualib.h>
}
#include "luatables.h"

using namespace std;

bool ForcesTorques::loadFromFile (const char* filename, bool strict) {
	ifstream file_in (filename);

	if (!file_in) {
		cerr << "Error opening force file " << filename << "!";

		if (strict)
			exit (1);

		return false;
	}

	if (times.size() > 0) {
		for(int i=0;i<times.size();i++){
			delete forces[i];
			delete torques[i];
		}
		times.clear();
		forces.clear();
		torques.clear();
	}

	double force_fps_previous_frame = 0.;
	int force_fps_frame_count = 0;
	bool last_line = false;

	string filename_str (filename);

	cout << "Loading forces and torques " << filename << endl;

	string previous_line;
	string line;

	bool found_column_section = false;
	bool found_data_section = false;
	bool column_section = false;
	bool data_section = false;
	bool csv_mode = false;
	int column_number = 0;
	int line_number = 0;

	//Load Drawing Parameters from Modelfile
	Forces f;
	Torques t;
	LuaTable model_table = LuaTable::fromFile(model_ref->model_filename.c_str());
	Vector3f force_color = model_table["animation_settings"]["force_color"].getDefault(f.color);
	Vector3f torque_color = model_table["animation_settings"]["torque_color"].getDefault(t.color);
	float force_scale = model_table["animation_settings"]["force_scale"].getDefault(f.scale);
	float torque_scale = model_table["animation_settings"]["torque_scale"].getDefault(t.scale);
	float force_transparency = model_table["animation_settings"]["force_transparency"].getDefault(f.transparency); 
	float torque_transparency = model_table["animation_settings"]["torque_transparency"].getDefault(t.transparency); 
	double force_threshold = model_table["animation_settings"]["force_threshold"].getDefault(f.draw_threshold);
	double torque_threshold = model_table["animation_settings"]["torque_threshold"].getDefault(t.draw_threshold);

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

			line = strip_comments (strip_whitespaces(line.substr(string("COLUMNS:").size(), line.size())));
			if (line.size() == 0)
				continue;
		}

		if (line.substr (0, string("DATA:").size()) == "DATA:"	&& !csv_mode) {
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
			cout << "Loading animation data from " << data_path.string() << endl;

			if (!file_in) {
				cerr << "Error opening animation file " << data_path.string() <<
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
			if (csv_mode) {
				data_columns = tokenize_csv_strip_whitespaces (line);
			} else {
				data_columns = tokenize (line);
			}

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
			addForcesTorques(state_values);

			//set drawing parameters
			forces[times.size()-1]->setDrawParams(force_color, force_scale, force_transparency, force_threshold);
			torques[times.size()-1]->setDrawParams(torque_color, torque_scale, torque_transparency, torque_threshold);


			force_fps_previous_frame = state_time;
			force_fps_frame_count++;

			if (state_time > duration)
				duration = state_time;
			continue;
		}
	}
	

	if (!found_data_section) {
		cerr << "Error: did not find DATA: section in forces and torques file!"
			 << endl;
		abort();
	}

	forces_filename = filename;

	return true;
}

Forces ForcesTorques::getForcesAtTime(float time) {
	unsigned int index = getIndexAtTime(time);
	//if at beginning or end use first or last entry
	if (index == 0) {
		return *forces[index];
	} else if ( time > duration) {
		return *forces[forces.size()-1];
	//if inbetween interpolate values for smooth animation
	} else {
		float time_fraction = getTimeFraction(time, index);
		Forces* prev = forces[index-1];
		Forces* next = forces[index];
		Forces f;
		f.setDrawParams(forces[index]->color, forces[index]->scale, forces[index]->transparency, forces[index]->draw_threshold);

		//iterate over all entries and interpolate them
		for(int i=0;i<prev->pos.size();i++) {
			Vector3f pos = interpolate(prev->pos[i], next->pos[i], time_fraction);
			Vector3f data = interpolate(prev->data[i], next->data[i], time_fraction);
			f.addForce(pos, data);
		}

		return f;
	}
}

Torques ForcesTorques::getTorquesAtTime(float time) {
	unsigned int index = getIndexAtTime(time);
	//if at beginning or end use first or last entry
	if (index == 0) {
		return *torques[index];
	} else if ( time > duration) {
		return *torques[forces.size()-1];
	//if inbetween interpolate values for smooth animation
	} else {
		float time_fraction = getTimeFraction(time, index);
		Torques* prev = torques[index-1];
		Torques* next = torques[index];
		Torques t;
		t.setDrawParams(torques[index]->color, torques[index]->scale, torques[index]->transparency, torques[index]->draw_threshold);

		//iterate over all entries and interpolate them
		for(int i=0;i<prev->pos.size();i++) {
			Vector3f pos = interpolate(prev->pos[i], next->pos[i], time_fraction);
			Vector3f data = interpolate(prev->data[i], next->data[i], time_fraction);
			t.addTorque(pos, data);
		}

		return t;
	}
}

unsigned int ForcesTorques::getIndexAtTime(float time) {
	//Find index for values corresponding to current time
	unsigned int index = 0;
	while ((time > times[index]) && index < times.size() - 1) {
		index++;
	}
	return index;
}

float ForcesTorques::getTimeFraction(float time, unsigned int index) {
	//Calculate timefraqtion used for interpolation
	float time_prev_frame = times[index - 1];
	float time_next_frame = times[index];
	return (time - time_prev_frame) / (time_next_frame - time_prev_frame);
}

Vector3f ForcesTorques::interpolate(Vector3f prev, Vector3f next, float fraction) {
	//Interpolate two Vector3f
	return (prev * (1-fraction)) + (next * fraction); 
}

void ForcesTorques::addForcesTorques(VectorNd data) {
	//first entry is time-stamp
	float time = data[0];

	//read force and torque data of the current time-stamp
	Forces* f = new Forces();
	Torques* t = new Torques();
	int count = (data.size() - 1)/9;
	for (int i=0; i<count ; i++) {
		Vector3f pos(data[i*9+1], data[i*9+2], data[i*9+3]);
		Vector3f force_data(data[i*9+4], data[i*9+5], data[i*9+6]);
		Vector3f torque_data(data[i*9+7], data[i*9+8], data[i*9+9]);

		f->addForce(pos, force_data);
		t->addTorque(pos, torque_data);
	}

	times.push_back(time);
	forces.push_back(f);
	torques.push_back(t);
}

void Forces::addForce(Vector3f pos, Vector3f data) {
	this->pos.push_back(pos);
	this->data.push_back(data);
}

void Forces::draw(Matrix33f baseChange) {
	for(int i=0;i<pos.size();i++) {
		//only draw force if force is bigger than threshold
		if (data[i].norm() > draw_threshold) {

			bool blend_enabled = glIsEnabled(GL_BLEND);
			glEnable(GL_BLEND);
			glDepthMask(GL_FALSE);
			glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);

			Vector4f forceColor(color[0], color[1], color[2], transparency);
			Vector3f vecPos = baseChange.transpose() * pos[i];
			Vector3f vecData = baseChange.transpose() * data[i];
			MeshVBO forceArrow = Create3DArrow(vecPos, vecData, forceColor, scale);

			glPushMatrix();
			forceArrow.draw(GL_TRIANGLES);
			glPopMatrix();

			glDepthMask(GL_TRUE);
			if (!blend_enabled) {
				glDisable(GL_BLEND);
			}
		}
	}
}

void Torques::draw(Matrix33f baseChange) {
	for(int i=0;i<pos.size();i++) {
		//only draw force if force is bigger than threshold
		if (data[i].norm() > draw_threshold) {

			bool blend_enabled = glIsEnabled(GL_BLEND);
			glEnable(GL_BLEND);
			glDepthMask(GL_FALSE);
			glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);

			Vector4f torqueColor(color[0], color[1], color[2], transparency);
			Vector3f vecPos = baseChange.transpose() * pos[i];
			Vector3f vecData = baseChange.transpose() * data[i];
			MeshVBO torqueArrow = Create3DCircleArrow(vecPos, vecData, torqueColor, scale);

			glPushMatrix();
			torqueArrow.draw(GL_TRIANGLES);
			glPopMatrix();

			glDepthMask(GL_TRUE);
			if (!blend_enabled) {
				glDisable(GL_BLEND);
			}
		}
	}
}

void Torques::addTorque(Vector3f pos, Vector3f data) {
	this->pos.push_back(pos);
	this->data.push_back(data);
}

