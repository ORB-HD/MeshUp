#ifndef _SIMPLEMATHGL_H_
#define _SIMPLEMATHGL_H_

#include "SimpleMath.h"
#include <cmath>

inline Matrix44f smRotate (float rot_deg, float x, float y, float z) {
	float c = cosf (rot_deg * M_PI / 180.f);
	float s = sinf (rot_deg * M_PI / 180.f);
	return Matrix44f (
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

inline Matrix44f smTranslate (float x, float y, float z) {
	return Matrix44f (
			1.f, 0.f, 0.f, 0.f,
			0.f, 1.f, 0.f, 0.f,
			0.f, 0.f, 1.f, 0.f,
			  x,   y,   z, 1.f
			);
}

inline Matrix44f smScale (float x, float y, float z) {
	return Matrix44f (
			  x, 0.f, 0.f, 0.f,
			0.f,   y, 0.f, 0.f,
			0.f, 0.f,   z, 0.f,
			0.f, 0.f, 0.f, 1.f
			);
}

/** Quaternion 
 *
 * order: x,y,z,w
 */
class smQuaternion : public Vector4f {
	public:
		smQuaternion (float x, float y, float z, float w):
			Vector4f (x, y, z, w)
		{}
		smQuaternion operator* (const smQuaternion &quat) const {
			// we basically want to do a
			//   quat * this

			Vector3f v1 (quat[0], quat[1], quat[2]);
			Vector3f v2 ( (*this)[0], (*this)[1], (*this)[2]);
			float w1 = quat[3];
			float w2 = (*this)[3];

			Vector3f v = w1 * v2 + w2 * v1 + v1.cross(v2);

			return smQuaternion (
					v[0], v[1], v[2],
					w1 * w2 - v1.transpose() * v2
					);
		}

		static smQuaternion fromGLRotate (float angle, float x, float y, float z) {
			float st = sin (angle * M_PI / 360.f);
			return smQuaternion (
						st * x,
						st * y,
						st * z,
						cos (angle * M_PI / 360.f)
						);
		}

		Matrix44f toGLMatrix() const {
			float x = (*this)[0];
			float y = (*this)[1];
			float z = (*this)[2];
			float w = (*this)[3];
			return Matrix44f (
					1 - 2*y*y - 2*z*z,
					2*x*y + 2*w*z,
					2*x*y - 2*w*y,
					0.f,

					2*x*y - 2*w*z,
					1 - 2*x*x - 2*z*z,
					2*y*z + 2*w*x,
					0.f,

					2*x*z + 2*w*y,
					2*y*z - 2*w*x,
					1 - 2*x*x - 2*y*y,
					0.f,
					
					0.f,
					0.f,
					0.f,
					1.f);
		}
};

/* _SIMPLEMATHGL_H_ */
#endif
