#ifndef MESHUP_SCENE_H_
#define MESHUP_SCENE_H_

#include <vector>

#include "Math.h"

struct Animation;
struct MeshupModel;

struct Scene {
	Scene() :
		current_time (0.f),
		longest_animation (0.f),
		model_displacement (0.f, 0.f, -1.f)
	{};
	float current_time;
	float longest_animation;
	Vector3f model_displacement;

	std::vector<Animation*> animations;
	std::vector<MeshupModel*> models;

	void setCurrentTime (double t);

	void drawMeshes();
	void drawBaseFrameAxes();
	void drawFrameAxes();
	void drawPoints();
	void drawCurves();
};

#endif
