#ifndef MESHUP_SCENE_H_
#define MESHUP_SCENE_H_

#include <vector>

#include "Math.h"
#include "Arrow.h"

struct Animation;
struct MeshupModel;
struct ForcesTorques;

struct Scene {
	Scene() :
		current_time (0.f),
		longest_animation (0.f),
		model_displacement (0.f, 0.f, -1.f)
	{};
	float current_time;
	float longest_animation;
	Vector3f model_displacement;
	ArrowCreator arrow_creator;
	bool drawingForces;
	bool drawingTorques;

	std::vector<Animation*> animations;
	std::vector<MeshupModel*> models;
	std::vector<ForcesTorques*> forcesTorquesQueue;

	void setCurrentTime (double t);

	void drawMeshes();
	void drawBaseFrameAxes();
	void drawFrameAxes();
	void drawPoints();
	void drawCurves();
	void drawForces();
	void drawTorques();
};

#endif
