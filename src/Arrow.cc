#include "Arrow.h"
#include "GL/glew.h"

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

	Vector3f unit_axis_vec = Vector3f(0., 1., 0.);
	Vector3f v = unit_axis_vec.cross(vec_dir_normalized);

	double rot_angle_radian = SimpleMath::Fixed::calcAngleRadian(unit_axis_vec, vec_dir_normalized);
	double rot_angle_degree = rot_angle_radian * 180 / M_PI;

	if (v.norm() < 0.00001){
		if (fabs(rot_angle_degree - 180.) < 0.1){
			v = Vector3f(1., 0., 0.);
			rot_angle_degree = 180.;
		}
		else{
			v = unit_axis_vec;
			rot_angle_degree = 0.;
		}
	}	
	v = v.normalize();

	// Add Transformation Matricies
	glPushMatrix();
		// Translate
		glTranslatef(arrow.pos[0], arrow.pos[1], arrow.pos[2]);
		// Rotate
		glMultMatrixf(SimpleMath::GL::RotateMat44(rot_angle_degree, v[0], v[1], v[2]).data());
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
