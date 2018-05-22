#ifndef _FORCESTORQUES_H
#define _FORCESTORQUES_H

#include <vector>

#include "Math.h"
#include "Model.h"
#include "Arrow.h"

struct ForcesTorques {
	ForcesTorques(MeshupModel* model) :
		forces_filename(""),
		times (std::vector<float>()),
		forces (std::vector<ArrowList*>()),
		torques (std::vector<ArrowList*>())
	{
		model_ref = model;
	}
	~ForcesTorques() {
		for(int i=0;i<times.size();i++) {
			delete forces[i];
			delete torques[i];
		}
	}
	// Metadata
	std::string forces_filename;
	MeshupModel* model_ref;
	float duration;

	// Data Storage
	std::vector<float> times;
	std::vector<ArrowList*> forces;
	std::vector<ArrowList*> torques;

	// Drawing Parameters 
	double force_threshold;
	ArrowProperties force_properties;
	double torque_threshold;
	ArrowProperties torque_properties;


	bool loadFromFile (const char* filename, bool strict = true);
	void addForcesTorques(VectorNd data);
	unsigned int getIndexAtTime(float time);
	float getTimeFraction(float time, unsigned int index);
	Vector3f interpolate(const Vector3f &prev, const Vector3f &next, float fraction);
	ArrowList getForcesAtTime(float time);
	ArrowList getTorquesAtTime(float time);
};

#endif  // FORCESTORQUES_H
