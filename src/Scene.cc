#include "Scene.h"

#include "Model.h"
#include "Animation.h"
#include "ForcesTorques.h"
#include "GL/glew.h"

#include <iostream>

using namespace std;

void Scene::setCurrentTime (double t){
	current_time = t;
	for (unsigned int i = 0; i < animations.size(); i++) {
		UpdateModelFromAnimation (models[i], animations[i], current_time);
	}
}

void Scene::drawMeshes() {
	Vector3f offset_start (0.f, 0.f, 0.f);
	
	if (models.size() > 1) {
		offset_start = - model_displacement * models.size() * 0.5;
	}

	glPushMatrix();
	glTranslatef (offset_start[0], offset_start[1], offset_start[2]);

	for (unsigned int i = 0; i < models.size(); i++) {
		glTranslatef (model_displacement[0], model_displacement[1], model_displacement[2]);
		models[i]->draw();
	}

	glPopMatrix();
}

void Scene::drawBaseFrameAxes(){
	Vector3f offset_start (0.f, 0.f, 0.f);
	
	if (models.size() > 1) {
		offset_start = - model_displacement * models.size() * 0.5;
	}

	glPushMatrix();
	glTranslatef (offset_start[0], offset_start[1], offset_start[2]);

	for (unsigned int i = 0; i < models.size(); i++) {
		glTranslatef (model_displacement[0], model_displacement[1], model_displacement[2]);
		models[i]->drawBaseFrameAxes();
	}

	glPopMatrix();
}

void Scene::drawFrameAxes(){
	Vector3f offset_start (0.f, 0.f, 0.f);
	
	if (models.size() > 1) {
		offset_start = - model_displacement * models.size() * 0.5;
	}

	glPushMatrix();
	glTranslatef (offset_start[0], offset_start[1], offset_start[2]);

	for (unsigned int i = 0; i < models.size(); i++) {
		glTranslatef (model_displacement[0], model_displacement[1], model_displacement[2]);
		models[i]->drawFrameAxes();
	}

	glPopMatrix();
}

void Scene::drawPoints(){
	Vector3f offset_start (0.f, 0.f, 0.f);
	
	if (models.size() > 1) {
		offset_start = - model_displacement * models.size() * 0.5;
	}

	glPushMatrix();
	glTranslatef (offset_start[0], offset_start[1], offset_start[2]);

	for (unsigned int i = 0; i < models.size(); i++) {
		glTranslatef (model_displacement[0], model_displacement[1], model_displacement[2]);
		models[i]->drawPoints();
	}

	glPopMatrix();
}

void Scene::drawCurves(){
	Vector3f offset_start (0.f, 0.f, 0.f);
	
	if (models.size() > 1) {
		offset_start = - model_displacement * models.size() * 0.5;
	}

	glPushMatrix();
	glTranslatef (offset_start[0], offset_start[1], offset_start[2]);

	for (unsigned int i = 0; i < models.size(); i++) {
		glTranslatef (model_displacement[0], model_displacement[1], model_displacement[2]);
		models[i]->drawCurves();
	}

	glPopMatrix();
}

void Scene::drawForces() {
	Vector3f offset_start (0.f, 0.f, 0.f);
	
	if (models.size() > 1) {
		offset_start = - model_displacement * models.size() * 0.5;
	}

	glPushMatrix();
	glTranslatef (offset_start[0], offset_start[1], offset_start[2]);

	bool blend_enabled = glIsEnabled(GL_BLEND);
	glEnable(GL_BLEND);
	glDepthMask(GL_FALSE);
	glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);

	for (int i = 0; i < forcesTorquesQueue.size(); i++) {
		glTranslatef (model_displacement[0], model_displacement[1], model_displacement[2]);
		Matrix33f baseChange = models[i]->configuration.axes_rotation;
		ArrowList forces = forcesTorquesQueue[i]->getForcesAtTime(current_time);
		for (int j = 0; j < forces.arrows.size(); j++) {
			Arrow changed = forces.arrows[j]->createBaseChangedArrow(baseChange);
			if (changed.direction.norm() > forcesTorquesQueue[i]->force_threshold) {
				arrow_creator.drawArrow(&arrow_creator.arrow3d, changed, forcesTorquesQueue[i]->force_properties);
			}
		}
	}

	glDepthMask(GL_TRUE);
	if (!blend_enabled) {
		glDisable(GL_BLEND);
	}

	glPopMatrix();
}

void Scene::drawTorques() {
	Vector3f offset_start (0.f, 0.f, 0.f);
	
	if (models.size() > 1) {
		offset_start = - model_displacement * models.size() * 0.5;
	}

	glPushMatrix();
	glTranslatef (offset_start[0], offset_start[1], offset_start[2]);

	bool blend_enabled = glIsEnabled(GL_BLEND);
	glEnable(GL_BLEND);
	glDepthMask(GL_FALSE);
	glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);

	for (int i = 0; i < forcesTorquesQueue.size(); i++) {
		glTranslatef (model_displacement[0], model_displacement[1], model_displacement[2]);
		Matrix33f baseChange = models[i]->configuration.axes_rotation;
		ArrowList torques = forcesTorquesQueue[i]->getTorquesAtTime(current_time);
		for (int j = 0; j < torques.arrows.size(); j++) {
			if (torques.arrows[j]->direction.norm() > forcesTorquesQueue[i]->torque_threshold) {
				arrow_creator.drawArrow(&arrow_creator.circle_arrow3d, torques.arrows[j]->createBaseChangedArrow(baseChange), forcesTorquesQueue[i]->torque_properties);
			}
		}
	}

	glDepthMask(GL_TRUE);
	if (!blend_enabled) {
		glDisable(GL_BLEND);
	}

	glPopMatrix();
}
