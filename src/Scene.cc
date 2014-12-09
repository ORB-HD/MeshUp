#include "Scene.h"

#include "Model.h"
#include "Animation.h"
#include "GL/glew.h"

void Scene::setCurrentTime (double t){
	current_time = t;

	for (unsigned int i = 0; i < animations.size(); i++) {
		UpdateModelFromAnimation (models[i], animations[i], current_time);
	}
}

void Scene::drawMeshes(){
	glPushMatrix();
	glTranslatef (0., 0., 0.5 * models.size());

	for (unsigned int i = 0; i < models.size(); i++) {

		glTranslatef (0., 0., -1.);
		models[i]->draw();
	}

	glPopMatrix();
}

void Scene::drawBaseFrameAxes(){
	glPushMatrix();
	glTranslatef (0., 0., 0.5 * models.size());

	for (unsigned int i = 0; i < models.size(); i++) {

		glTranslatef (0., 0., -1.);
		models[i]->drawBaseFrameAxes();
	}

	glPopMatrix();
}

void Scene::drawFrameAxes(){
	glPushMatrix();
	glTranslatef (0., 0., 0.5 * models.size());

	for (unsigned int i = 0; i < models.size(); i++) {

		glTranslatef (0., 0., -1.);
		models[i]->drawFrameAxes();
	}

	glPopMatrix();
}

void Scene::drawPoints(){
	glPushMatrix();
	glTranslatef (0., 0., 0.5 * models.size());

	for (unsigned int i = 0; i < models.size(); i++) {

		glTranslatef (0., 0., -1.);
		models[i]->drawPoints();
	}

	glPopMatrix();
}

void Scene::drawCurves(){
	glPushMatrix();
	glTranslatef (0., 0., 0.5 * models.size());

	for (unsigned int i = 0; i < models.size(); i++) {

		glTranslatef (0., 0., -1.);
		models[i]->drawCurves();
	}

	glPopMatrix();
}

