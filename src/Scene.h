#ifndef MESHUP_SCENE_H_
#define MESHUP_SCENE_H_

#include <vector>

struct Animation;
struct MeshupModel;

struct Scene {
	Scene() :
		current_time (0.f),
		longest_animation (0.f)
	{};
	float current_time;
	float longest_animation;

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
