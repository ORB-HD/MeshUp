// CameraAnimate.h
// header file for animation of camera class

#ifndef _CAMERAANIMATE_H
#define _CAMERAANIMATE_H

#include <vector>

#include "SimpleMath/SimpleMath.h"


struct CameraAnimate {
    CameraAnimate() :
        camera_filename(""),
        raw_values (std::vector<VectorNd>())
	//max_values (std::vector<double>())
    {}

    std::string camera_filename;
    float duration;

    std::vector<VectorNd> raw_values;
    //std::vector<double> max_values;

    bool loadFromFile (const char* filename, bool strict = true);

    VectorNd getCameraAtTime(float time);
};


#endif  // CAMERAANIMATE_H
