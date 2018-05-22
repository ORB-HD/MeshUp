#ifndef Arrow_h_INCLUDED
#define Arrow_h_INCLUDED

#include <vector>

#include "Math.h"
#include "MeshVBO.h"

struct Arrow {
	Vector3f pos;
	Vector3f data;

	Arrow createBaseChangedArrow(Matrix33f base_change);
};

struct ArrowList{
	ArrowList() :
		arrows (std::vector<Arrow*>())
	{}
	std::vector<Arrow*> arrows;
	void addArrow(const Vector3f pos,const Vector3f data);
};

struct ArrowProperties {
	ArrowProperties() {}
	ArrowProperties(Vector3f colorp, float scalep, float transparencyp): 
		color (colorp),
		scale (scalep),
		transparency (transparencyp)
	{}
	Vector3f color;
	float scale;
	float transparency;
};

struct ArrowCreator {
	ArrowCreator();

	MeshVBO arrow3d, circle_arrow3d;

	MeshVBO createArrow(MeshVBO *basearrow, Arrow arrow, ArrowProperties properties);
};

#endif // Arrow_h_INCLUDED

