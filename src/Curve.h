#ifndef _CURVE_H
#define _CURVE_H

#include "MeshVBO.h"

/** \brief Allows drawing of three dimensional curves.
 *
 * After adding points with Curve::addPointWithColor(...) one has to first
 * compile the Vertex Buffer Object (VBO) with Curve::generate_vbo(), which
 * is then sent to the graphics card. After that one can draw the curve by
 * calling Curve::draw().
 *
 * One must not forget to free the VBO once the curve is no more used by
 * calling Curve::delete_vbo().
 *
 * The width of the line can be specified by setting the value
 * Curve::width.
 *
 * Example (drawing of a helical curve):
 * \code
 *	Curve curve;
 *
 *	int num_segments = 50;
 *	for (int i = 0; i <= num_segments + 1; i++) {
 *		float t = M_PI * 2. * static_cast<float>(i) / static_cast<float>(num_segments);
 *
 *		curve.addPointWithColor(sinf(t), t * 0.25, cosf(t), 1.f - t, t * 1.f, 1.f);
 *	}
 *	curve.generate_vbo();
 *	curve.width = 3.;
 *	curve.draw();
 *	curve.delete_vbo();
 * \endcode
 */
struct Curve {
	Curve() : width (1.f)
		{ }
	void addPointWithColor(float x, float y, float z, float r, float g, float b) {
		points.push_back (Vector3f (x, y, z));
		colors.push_back (Vector3f (r, g, b));
	}

	std::vector<Vector3f> points;
	std::vector<Vector3f> colors;

	float width;

	/// Uses data from points and colors to compile the VBO
	void generate_vbo();

	/// Frees the VBO
	void delete_vbo() {
		meshVBO.delete_vbo();
	}

	void draw();

	MeshVBO meshVBO;	
};

#endif
