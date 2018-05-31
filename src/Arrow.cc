#include "Arrow.h"
#include "GL/glew.h"

enum Axis{
	XDir,
	YDir,
	ZDir
};

inline Axis calculateArrowDirection(Vector3f forceVector){
	Vector3f unit_force_vector = forceVector.normalize();

	Vector3f x_dir(1.f,0.f,0.f);
	Vector3f y_dir(0.f,1.f,0.f);
	Vector3f z_dir(0.f,0.f,1.f);

	double x_val = abs(x_dir.dot(unit_force_vector));
	double y_val = abs(y_dir.dot(unit_force_vector));
	double z_val = abs(z_dir.dot(unit_force_vector));

	if(x_val < y_val && x_val < z_val) {
		return Axis::XDir;
	}
	else if(y_val < z_val){
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
	arrow3d.colors.clear();

	circle_arrow3d = CreateUnit3DCircleArrow();
	circle_arrow3d.colors.clear();
}

void ArrowCreator::drawArrow(MeshVBO *basearrow, Arrow arrow, ArrowProperties properties) {
	// Calculate need transformation values
	double dir_norm = arrow.direction.norm();
	double total_scale = dir_norm*properties.scale;
	Vector3f vec_dir_normalized = arrow.direction.normalized();
	Vector4f color(properties.color[0], properties.color[1], properties.color[2], properties.transparency);

	Axis axis_dir = calculateArrowDirection(vec_dir_normalized);
	Vector3f axis_vec = createUnitAxisVector(axis_dir);

	Vector3f rot_axis = axis_vec.cross(vec_dir_normalized).normalize();
	double rot_angle_radian = SimpleMath::Fixed::calcAngleRadian(axis_vec, vec_dir_normalized);
	double rot_angle_degree = rot_angle_radian * 180 / M_PI;

	// Add Transformation Matricies
	glPushMatrix();
		// Translate
		glTranslatef(arrow.pos[0], arrow.pos[1], arrow.pos[2]);
		// Rotate
		if ( axis_dir == Axis::XDir ) {
			glMultMatrixf(SimpleMath::GL::RotateMat44(270, 0., 0., 1.).data());
		} else if ( axis_dir == Axis::ZDir ) {
			glMultMatrixf(SimpleMath::GL::RotateMat44(90, 1., 0., 0.).data());
		}
		glMultMatrixf(SimpleMath::GL::RotateMat44(rot_angle_degree, rot_axis[0], rot_axis[1], rot_axis[2]).data());
		// Scale
		glMultMatrixf(SimpleMath::GL::ScaleMat44(total_scale, total_scale, total_scale).data());
		// Set Drawing Color 
		glColor4f(color[0], color[1], color[2], color[3]);
		// Draw Arrow
		basearrow->draw(GL_TRIANGLES);
	glPopMatrix();
}

void ArrowList::addArrow(const Vector3f pos, const Vector3f direction) {
	Arrow *a = new Arrow();
	a->pos = pos;
	a->direction = direction;
	arrows.push_back(a);
}

Arrow Arrow::createBaseChangedArrow(Matrix33f base_change) {
	Arrow result;
	result.pos = base_change.transpose() * pos;
	result.direction = base_change.transpose() * direction;
	return result;
}
