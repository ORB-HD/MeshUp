// CameraAnimate.cc
// implementation of camera animation

#include "CameraAnimate.h"
#include "MeshVBO.h"
#include "string_utils.h"
#include <iostream>
#include <fstream>
#include <boost/filesystem.hpp>


using namespace std;

bool CameraAnimate::loadFromFile (const char* filename, bool strict) {
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

    raw_values.clear();

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

        if (line.substr (0, string("DATA:").size()) == "DATA:"  && !csv_mode) {
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
            raw_values.push_back(state_values);

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

// Interpolates the camera values to make animation smooth
// Returns a vector with the camera values corresponding
// to the input time
VectorNd CameraAnimate::getCameraAtTime(float time){

    int frame_index = 0;

    // Determine the right camera entry
    while ((time > raw_values[frame_index][0]) && frame_index <
                          raw_values.size() - 1){
        frame_index++;
    }

    // At the beginning of the animation, always return first entry.
    // This is done here to avoid adressing an array with a negative number
    if(frame_index == 0){
        return raw_values[frame_index];

    } else {

    float time_prev_frame = raw_values[frame_index - 1][0];
    float time_next_frame = raw_values[frame_index][0];
    float time_fraction;

    // At the end return last frame. This is done here to avoid
    // time_fraction of becoming infinite
    if(time_prev_frame == time_next_frame){
      return raw_values[frame_index -1];
    }
    else{
      time_fraction = (time - time_prev_frame) /
            (time_next_frame - time_prev_frame);
    }

    VectorNd interpolatedValues = raw_values[frame_index - 1];

    // Interpolate between previous and next frame
    for(int i = 0; i < interpolatedValues.size(); i++){
      interpolatedValues[i] = (1 - time_fraction) * interpolatedValues[i] +
          (time_fraction * raw_values[frame_index][i]);
    }

    return interpolatedValues;
    }
}
