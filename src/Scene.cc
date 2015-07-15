#include "Scene.h"

#include "Model.h"
#include "Animation.h"
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

