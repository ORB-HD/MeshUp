#ifndef _SIMPLEMATHGL_H_
#define _SIMPLEMATHGL_H_

#include "SimpleMath.h"
#include <cmath>

SimpleMath::Fixed::Matrix<float, 4, 4> smRotate (float rot_deg, float x, float y, float z) {
	float c = cosf (rot_deg * M_PI / 180.f);
	float s = sinf (rot_deg * M_PI / 180.f);
	return SimpleMath::Fixed::Matrix<float, 4, 4> (
			x * x * (1.0f - c) + c,
			y * x * (1.0f - c) + z * s,
			x * z * (1.0f - c) - y * s,
			0.f, 

			x * y * (1.0f - c) - z * s,
			y * y * (1.0f - c) + c,
			y * z * (1.0f - c) + x * s,
			0.f,

			x * z * (1.0f - c) + y * s,
			y * z * (1.0f - c) - x * s,
			z * z * (1.0f - c) + c,
			0.f,

			0.f, 0.f, 0.f, 1.f
			);
}

SimpleMath::Fixed::Matrix<float, 4, 4> smTranslate (float x, float y, float z) {
	return SimpleMath::Fixed::Matrix<float, 4, 4> (
			1.f, 0.f, 0.f, 0.f,
			0.f, 1.f, 0.f, 0.f,
			0.f, 0.f, 1.f, 0.f,
			  x,   y,   z, 1.f
			);
}

SimpleMath::Fixed::Matrix<float, 4, 4> smScale (float x, float y, float z) {
	return SimpleMath::Fixed::Matrix<float, 4, 4> (
			  x, 0.f, 0.f, 0.f,
			0.f,   y, 0.f, 0.f,
			0.f, 0.f,   z, 0.f,
			0.f, 0.f, 0.f, 1.f
			);
}


/* _SIMPLEMATHGL_H_ */
#endif
