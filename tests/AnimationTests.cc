#include <UnitTest++.h>

#include "Model.h"
#include "Animation.h"
#include "SimpleMath/SimpleMathGL.h"

#include <iostream>

using namespace std;

const float TEST_PREC = 1.0e-6;

struct ModelFixture {
	ModelFixture() {
		model = MeshupModelPtr (new MeshupModel());
		model->skip_vbo_generation = true;
		model->meshmap.insert (make_pair<std::string, MeshPtr>("M1", MeshPtr (new MeshVBO())));
		model->addFrame("ROOT", "F1", SimpleMath::GL::TranslateMat44 (0.f, 1.f, 0.f));
		model->addSegment("F1", "S1",
			Vector3f (1.1, 1.2, 1.3),
			Vector3f (2.1, 2.2, 2.3),
			Vector3f (1.f, 1.f, 1.f),
			"M1",
			Vector3f (3.1, 3.2, 3.3),
			Vector3f (4.1, 4.2, 4.3)
			);
	}

	MeshupModelPtr model;
	AnimationPtr animation;
	FrameConfig configuration;
};

TEST_FIXTURE (ModelFixture, TestRotX) {
	
}
