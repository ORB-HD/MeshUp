#ifndef _FORCESTORQUES_H
#define _FORCESTORQUES_H

#include <vector>

#include "Math.h"
#include "Model.h"

struct Forces {
	Forces() : 
		pos (std::vector<Vector3f>()),
		data (std::vector<Vector3f>()),
		color (Vector3f(1., 0., 0.)),
		scale (0.002),
		transparency (0.5),
		draw_threshold (1.0)
	{}

	std::vector<Vector3f> pos;
	std::vector<Vector3f> data;
	Vector3f color;
	float scale;
	float transparency;
	double draw_threshold;

	void setDrawParams(Vector3f color, float scale, float transparency, double threshold) {
		this->color = color;
		this->scale = scale;
		this->transparency = transparency;
		this->draw_threshold = threshold;
	}
	void addForce(Vector3f pos, Vector3f data);
	void draw(Matrix33f baseChange);
};

struct Torques {
	Torques() : 
		pos (std::vector<Vector3f>()),
		data (std::vector<Vector3f>()),
		color (Vector3f(0., 1., 0.)),
		scale (0.01),
		transparency (0.5),
		draw_threshold (0.1)
	{}

	std::vector<Vector3f> pos;
	std::vector<Vector3f> data;
	Vector3f color;
	float scale;
	float transparency;
	double draw_threshold;

	void setDrawParams(Vector3f color, float scale, float transparency, double threshold) {
		this->color = color;
		this->scale = scale;
		this->transparency = transparency;
		this->draw_threshold = threshold;
	}

	void addTorque(Vector3f pos, Vector3f data);
	void draw(Matrix33f baseChange);
};

struct ForcesTorques {
	ForcesTorques(MeshupModel* model) :
		forces_filename(""),
		times (std::vector<float>()),
		forces (std::vector<Forces*>()),
		torques (std::vector<Torques*>())
	{
		model_ref = model;
	}
	~ForcesTorques() {
		for(int i=0;i<times.size();i++) {
			delete forces[i];
			delete torques[i];
		}
	}

	std::string forces_filename;
	float duration;
	MeshupModel* model_ref;
	std::vector<float> times;

	bool loadFromFile (const char* filename, bool strict = true);
	void addForcesTorques(VectorNd data);
	Forces getForcesAtTime(float time);
	Torques getTorquesAtTime(float time);

private:
	std::vector<Forces*> forces;
	std::vector<Torques*> torques;

	unsigned int getIndexAtTime(float time);
	float getTimeFraction(float time, unsigned int index);
	Vector3f interpolate(Vector3f prev, Vector3f next, float fraction);
};

#endif  // FORCESTORQUES_H
