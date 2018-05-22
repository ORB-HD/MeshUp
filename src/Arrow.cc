#include "Arrow.h"

enum Axis{
	XDir,
	YDir,
	ZDir
};

inline Axis calculateArrowDirection(Vector3f forceVector){
	Vector3f unitForceVector = forceVector.normalize();

	Vector3f xDir(1.f,0.f,0.f);
	Vector3f yDir(0.f,1.f,0.f);
	Vector3f zDir(0.f,0.f,1.f);

	double xVal = abs(xDir.dot(unitForceVector));
	double yVal = abs(yDir.dot(unitForceVector));
	double zVal = abs(zDir.dot(unitForceVector));

	if(xVal < yVal && xVal < zVal) {
		return Axis::XDir;
	}
	else if(yVal < zVal){
		return Axis::YDir;
	}
	return Axis::ZDir;
}

// Creates an unit axis vector in the indicated direction. 1 for X, 2 for Y, 3 for Z.
inline Vector3f createUnitAxisVector(Axis dir) {

	if(dir < Axis::XDir || dir > Axis::ZDir) {
		std::cout << "Invalid axis direction supplied as parameter!" << std::endl;
		abort();
	}

	if(dir == Axis::XDir) {
		return Vector3f(1.f,0.f,0.f);
	}
	else if(dir == Axis::YDir) {
		return Vector3f(0.f,1.f,0.f);
	}
	return Vector3f(0.f,0.f,1.f);
}

ArrowCreator::ArrowCreator() {
	arrow3d = CreateUnit3DArrow();
	circle_arrow3d = CreateUnit3DCircleArrow();
}

MeshVBO ArrowCreator::createArrow(MeshVBO *basearrow, Arrow arrow, ArrowProperties properties) {
	// Calculate need transformation values
	double dirNorm = arrow.data.norm();
	Vector3f vecDirNormalized = arrow.data.normalized();
	Vector4f color(properties.color[0], properties.color[1], properties.color[2], properties.transparency);

	Axis axisDir = calculateArrowDirection(vecDirNormalized);
	Vector3f axisVec = createUnitAxisVector(axisDir);

	Vector3f rotAxis = axisVec.cross(vecDirNormalized).normalize();
	double rotAngleRadian = SimpleMath::Fixed::calcAngleRadian(axisVec, vecDirNormalized);
	double rotAngleDegree = rotAngleRadian * 180 / M_PI;

	// Create Arrow Mesh that will be transforemed  into the final form
	MeshVBO final_arrow;
	if ( axisDir == Axis::XDir ) {
		final_arrow.join(SimpleMath::GL::RotateMat44(270, 0., 0., 1.), *basearrow);
	} else if ( axisDir == Axis::YDir ) {
		final_arrow = MeshVBO(*basearrow);
	} else if ( axisDir == Axis::ZDir ) {
		final_arrow.join(SimpleMath::GL::RotateMat44(90, 1., 0., 0.), *basearrow);
	}

	// Apply transformations
	final_arrow.transform(SimpleMath::GL::ScaleMat44(dirNorm, dirNorm, dirNorm));
	final_arrow.transform(SimpleMath::GL::ScaleMat44(properties.scale, properties.scale, properties.scale));
	final_arrow.transform(SimpleMath::GL::RotateMat44(rotAngleDegree, rotAxis[0], rotAxis[1], rotAxis[2]));
	final_arrow.transform(SimpleMath::GL::TranslateMat44(arrow.pos[0], arrow.pos[1], arrow.pos[2]));
	final_arrow.setColor(color);

	return final_arrow;
}

void ArrowList::addArrow(const Vector3f pos, const Vector3f data) {
	Arrow *a = new Arrow();
	a->pos = pos;
	a->data = data;
	arrows.push_back(a);
}

Arrow Arrow::createBaseChangedArrow(Matrix33f base_change) {
	Arrow result;
	result.pos = base_change.transpose() * pos;
	result.data = base_change.transpose() * data;
	return result;
}
