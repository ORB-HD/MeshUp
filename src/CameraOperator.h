// CameraAnimate.h
// header file for animation of camera class

#ifndef _CAMERAANIMATE_H
#define _CAMERAANIMATE_H

#include <vector>
#include <QWidget>
#include <QListWidgetItem>

#include "SimpleMath/SimpleMath.h"
#include "Camera.h"

struct CameraPosition {
	float time;
	Camera* cam;
	bool moving;
};

// A Class to diplay Cameras in a ListView

class CameraListItem : public QListWidgetItem {
	public:
		CameraListItem(CameraPosition* campos, QListWidget* parent=0): camera_data(campos)
		{
			data_changed();
		}

		CameraPosition* camera_data;

		void data_changed();
};

// This class is used to Manage multiple camera postions within an animation

struct CameraOperator{
    CameraOperator(QListWidget* camera_display) :
        camera_filename(""),
        camera_display(camera_display),
        cam_pos (std::vector<CameraPosition*>())
    {
	    fixed = true;
	    Camera* cam0 = new Camera();
	    mobile_cam = new Camera();
	    CameraPosition* campos = new CameraPosition();
	    campos->cam = cam0;
	    campos->time = 0.0;
	    campos->moving = false;
	    cam_pos.push_back(campos);
	    current_cam = campos->cam;
	}
	~CameraOperator() {
		for(int i=0;i<cam_pos.size();i++) {
			delete cam_pos[i]->cam;
			delete cam_pos[i];
		}
	}

    std::string camera_filename;
    float duration;
    std::vector<CameraPosition*> cam_pos;
    bool fixed;
    int height, width;

    Camera* current_cam;
    Camera* mobile_cam;
    QListWidget* camera_display;

    bool loadFromFile (const char* filename, bool strict = true);
    void addCameraPos (VectorNd data);
    void addCamera(float time, Camera* cam, bool moving);
    bool updateCamera (float current_time);
    void setFixed(bool status);
    void exportToFile(const char* filename);
    void setCamHeight(int height);
    void setCamWidth(int width);
};


#endif  // CAMERAANIMATE_H
