// CameraAnimate.h
// header file for animation of camera class

#ifndef _CAMERAANIMATE_H
#define _CAMERAANIMATE_H

#include <vector>

#include "SimpleMath/SimpleMath.h"
#include "Camera.h"

// This class is used to Manage multiple camera postions within an animation

struct CameraOperator{
    CameraOperator() :
        camera_filename(""),
        times (std::vector<float>()),
        pois (std::vector<Vector3f>()),
        eyes (std::vector<Vector3f>()),
        smooth (std::vector<bool>()),
        cams (std::vector<Camera*>())
    {
	    fixed = true;
	    Camera* cam0 = new Camera();
	    mobile_cam = new Camera();
	    cams.push_back(cam0);
	    current_cam = cams[0];
	    times.push_back(0.0);
	}
	~CameraOperator() {
		for(int i=0;i<cams.size();i++) {
			delete cams[i];
		}
	}

    std::string camera_filename;
    float duration;
	std::vector<float> times;
    std::vector<Vector3f> pois;
    std::vector<Vector3f> eyes;
    std::vector<bool> smooth;
    std::vector<Camera*> cams;
    bool fixed;
    int height, width;

    Camera* current_cam;
    Camera* mobile_cam;

    bool loadFromFile (const char* filename, bool strict = true);
    void addCameraPos (VectorNd data);
    bool updateCamera (float current_time);
    void reset();
    void store();
    void setFixed(bool status);
    void exportToFile(const char* filename);
    void setCamHeight(int height);
    void setCamWidth(int width);
};


#endif  // CAMERAANIMATE_H
