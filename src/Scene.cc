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
		offset_start = model_displacement * models.size();
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
	float z_start = 0.;
	float z_offset = 0.;
	
	if (models.size() > 1) {
		z_start = 0.5f * models.size();
		z_offset = -1.f;
	}

	glPushMatrix();
	glTranslatef (0., 0., z_start);

	for (unsigned int i = 0; i < models.size(); i++) {
		glTranslatef (0., 0., z_offset);
		models[i]->drawBaseFrameAxes();
	}

	glPopMatrix();
}

void Scene::drawFrameAxes(){
	float z_start = 0.;
	float z_offset = 0.;
	
	if (models.size() > 1) {
		z_start = 0.5f * models.size();
		z_offset = -1.f;
	}

	glPushMatrix();
	glTranslatef (0., 0., z_start);

	for (unsigned int i = 0; i < models.size(); i++) {
		glTranslatef (0., 0., z_offset);
		models[i]->drawFrameAxes();
	}

	glPopMatrix();
}

void Scene::drawPoints(){
	float z_start = 0.;
	float z_offset = 0.;
	
	if (models.size() > 1) {
		z_start = 0.5f * models.size();
		z_offset = -1.f;
	}

	glPushMatrix();
	glTranslatef (0., 0., z_start);

	for (unsigned int i = 0; i < models.size(); i++) {
		glTranslatef (0., 0., z_offset);
		models[i]->drawPoints();
	}

	glPopMatrix();
}

void Scene::drawCurves(){
	float z_start = 0.;
	float z_offset = 0.;
	
	if (models.size() > 1) {
		z_start = 0.5f * models.size();
		z_offset = -1.f;
	}

	glPushMatrix();
	glTranslatef (0., 0., z_start);

	for (unsigned int i = 0; i < models.size(); i++) {
		glTranslatef (0., 0., z_offset);
		models[i]->drawCurves();
	}

	glPopMatrix();
}

