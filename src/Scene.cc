#include "Scene.h"

#include "Model.h"
#include "Animation.h"

void Scene::setCurrentTime (double t){
	current_time = t;

	for (unsigned int i = 0; i < animations.size(); i++) {
		UpdateModelFromAnimation (models[i], animations[i], current_time);
	}
}

void Scene::drawMeshes(){
	for (unsigned int i = 0; i < models.size(); i++) {
		models[i]->draw();
	}
}

void Scene::drawBaseFrameAxes(){
	for (unsigned int i = 0; i < models.size(); i++) {
		models[i]->drawBaseFrameAxes();
	}
}

void Scene::drawFrameAxes(){
	for (unsigned int i = 0; i < models.size(); i++) {
		models[i]->drawFrameAxes();
	}
}

void Scene::drawPoints(){
	for (unsigned int i = 0; i < models.size(); i++) {
		models[i]->drawPoints();
	}
}

void Scene::drawCurves(){
	for (unsigned int i = 0; i < models.size(); i++) {
		models[i]->drawCurves();
	}
}

