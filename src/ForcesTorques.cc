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
		for(int i=0;i<times.size();i++) {
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

	int line_number = 0;

	// Load Drawing Parameters from Modelfile
	LuaTable model_table = LuaTable::fromFile(model_ref->model_filename.c_str());
	Vector3f force_color = model_table["animation_settings"]["force_color"].getDefault(Vector3f(1., 0., 0.));
	Vector3f torque_color = model_table["animation_settings"]["torque_color"].getDefault(Vector3f(0., 1., 0.));
	float force_scale = model_table["animation_settings"]["force_scale"].getDefault(0.002);
	float torque_scale = model_table["animation_settings"]["torque_scale"].getDefault(0.01);
	float force_transparency = model_table["animation_settings"]["force_transparency"].getDefault(0.5); 
	float torque_transparency = model_table["animation_settings"]["torque_transparency"].getDefault(0.5); 

	force_threshold = model_table["animation_settings"]["force_threshold"].getDefault(1.0);
	torque_threshold = model_table["animation_settings"]["torque_threshold"].getDefault(0.1);

	force_properties = ArrowProperties(force_color, force_scale, force_transparency);
	torque_properties = ArrowProperties(torque_color, torque_scale, torque_transparency);

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

		// read data 
		std::vector<string> data_columns;
		data_columns = tokenize_csv_strip_whitespaces (line);

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

		force_fps_previous_frame = state_time;
		force_fps_frame_count++;

		if (state_time > duration)
			duration = state_time;
		continue;
	}

	forces_filename = filename;

	return true;
}

ArrowList ForcesTorques::getForcesAtTime(float time) {
	unsigned int index = getIndexAtTime(time);
	// if at beginning or end use first or last entry
	if (index == 0) {
		return *forces[index];
	} else if ( time > duration) {
		return *forces[forces.size()-1];
	// if inbetween interpolate values for smooth animation
	} else {
		float time_fraction = getTimeFraction(time, index);
		ArrowList* prev = forces[index-1];
		ArrowList* next = forces[index];
		ArrowList result;

		// iterate over all entries and interpolate them
		for(int i=0;i<prev->arrows.size();i++) {
			Vector3f pos = interpolate(prev->arrows[i]->pos, next->arrows[i]->pos, time_fraction);
			Vector3f direction = interpolate(prev->arrows[i]->direction, next->arrows[i]->direction, time_fraction);
			result.addArrow(pos, direction);
		}

		return result;
	}
}

ArrowList ForcesTorques::getTorquesAtTime(float time) {
	unsigned int index = getIndexAtTime(time);
	// if at beginning or end use first or last entry
	if (index == 0) {
		return *torques[index];
	} else if ( time > duration) {
		return *torques[torques.size()-1];
	// if inbetween interpolate values for smooth animation
	} else {
		float time_fraction = getTimeFraction(time, index);
		ArrowList* prev = torques[index-1];
		ArrowList* next = torques[index];
		ArrowList result;

		// iterate over all entries and interpolate them
		for(int i=0;i<prev->arrows.size();i++) {
			Vector3f pos = interpolate(prev->arrows[i]->pos, next->arrows[i]->pos, time_fraction);
			Vector3f direction = interpolate(prev->arrows[i]->direction, next->arrows[i]->direction, time_fraction);
			result.addArrow(pos, direction);
		}

		return result;
	}
}

unsigned int ForcesTorques::getIndexAtTime(float time) {
	// Find index for values corresponding to current time
	unsigned int index = 0;
	while ((time > times[index]) && index < times.size() - 1) {
		index++;
	}
	return index;
}

float ForcesTorques::getTimeFraction(float time, unsigned int index) {
	// Calculate timefraqtion used for interpolation
	float time_prev_frame = times[index - 1];
	float time_next_frame = times[index];
	return (time - time_prev_frame) / (time_next_frame - time_prev_frame);
}

Vector3f ForcesTorques::interpolate(const Vector3f &prev, const Vector3f &next, float fraction) {
	// Interpolate two Vector3f
	return (prev * (1-fraction)) + (next * fraction); 
}

void ForcesTorques::addForcesTorques(VectorNd data) {
	// first entry is time-stamp
	float time = data[0];

	// read force and torque data of the current time-stamp
	ArrowList *f = new ArrowList();
	ArrowList *t = new ArrowList();
	int count = (data.size() - 1)/9;
	for (int i=0; i<count ; i++) {
		Vector3f pos(data[i*9+1], data[i*9+2], data[i*9+3]);
		Vector3f force_data(data[i*9+4], data[i*9+5], data[i*9+6]);
		Vector3f torque_data(data[i*9+7], data[i*9+8], data[i*9+9]);

		f->addArrow(pos, force_data);
		t->addArrow(pos, torque_data);
	}

	times.push_back(time);
	forces.push_back(f);
	torques.push_back(t);
}

//void Forces::draw(Matrix33f base_change) {
//	bool blend_enabled = glIsEnabled(GL_BLEND);
//	glEnable(GL_BLEND);
//	glDepthMask(GL_FALSE);
//	glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);
//
//	for(int i=0;i<pos.size();i++) {
//		// only draw force if force is bigger than threshold
//		if (data[i].norm() > draw_threshold) {
//			Vector4f forceColor(color[0], color[1], color[2], transparency);
//			Vector3f vecPos = base_change.transpose() * pos[i];
//			Vector3f vecData = base_change.transpose() * data[i];
//			MeshVBO forceArrow = Create3DArrow(vecPos, vecData, forceColor, scale);
//
//			glPushMatrix();
//			forceArrow.draw(GL_TRIANGLES);
//			glPopMatrix();
//
//		}
//	}
//
//	glDepthMask(GL_TRUE);
//	if (!blend_enabled) {
//		glDisable(GL_BLEND);
//	}
//}

