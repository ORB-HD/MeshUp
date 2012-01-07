#ifndef _MODELDATA_H
#define _MODELDATA_H

#include <vector>
#include <list>
#include <iostream>
#include <map>
#include <boost/shared_ptr.hpp>

#include "SimpleMath.h"

struct Material {
	Vector4d ambient;
	Vector4d diffuse;
	Vector4d specular;
	double shininess;
};

struct MeshData {
	MeshData() :
		parented_segment(""),
		vbo_id(0),
		started(false)
	{}

	void begin();
	void end();

	void addVertice (float x, float y, float z);
	void addVerticefv (float vert[3]);
	void addNormal (float x, float y, float z);
	void addNormalfv (float norm[3]);

	unsigned int generate_vbo();
	void delete_vbo();

	void draw();

	std::string parented_segment;

	unsigned int vbo_id;
	bool started;

	std::vector<Vector3f> vertices;
	std::vector<Vector3f> normals;
	/// \note always 3 succeeding calls of addVertice are assumed to be a
	// triangle!
	std::vector<unsigned int> triangle_indices;
};

struct Bone;
typedef boost::shared_ptr<Bone> BonePtr;

struct Bone {
	Bone() :
		name (""),
		parent_translation (0.f, 0.f, 0.f),
		parent_rotation_ZYXeuler (0.f, 0.f, 0.f),
		pose_transform (Matrix44f::Identity ())
	{}

	std::string name;
	Vector3f parent_translation;
	Vector3f parent_rotation_ZYXeuler;

	/** Transformation from base to pose */
	Matrix44f pose_transform;

	std::vector<BonePtr> children;

	void updatePoseTransform(const Matrix44f &parent_pose_transform);
};

struct Segment {
	Segment () :
		name ("unnamed"),
		dimensions (1.f, 1.f, 1.f),
		bone (BonePtr())
	{}

	std::string name;

	Vector3f dimensions;
	Vector3f color;
	MeshData mesh;
	BonePtr bone;
};

struct BonePose {
	BonePose() :
		timestamp (-1.f),
		translation (0.f, 0.f, 0.f),
		rotation_ZYXeuler (0.f, 0.f, 0.f),
		scaling (1.f, 1.f, 1.f)
	{}

	float timestamp;
	Vector3f translation;
	Vector3f rotation_ZYXeuler;
	Vector3f scaling;
};

struct BoneAnimationTrack {
	typedef std::list<BonePose> BonePoseList;
	BonePoseList poses;

	BonePose interpolatePose (float time);
};

struct ModelData {
	ModelData() {
		// create the BASE bone
		BonePtr base_bone (new (Bone));
		base_bone->name = "BASE";
		base_bone->parent_translation.setZero();
		base_bone->parent_rotation_ZYXeuler.setZero();

		bones.push_back (base_bone);
		bonemap["BASE"] = base_bone;
	}

	typedef std::list<Segment> SegmentList;
	SegmentList segments;
	typedef std::list<MeshData> MeshDataList;
	MeshDataList meshes;
	typedef std::vector<BonePtr> BoneVector;
	BoneVector bones;
	typedef std::map<std::string, BonePtr> BoneMap;
	BoneMap bonemap;
	typedef std::map<BonePtr, BoneAnimationTrack> BoneAnimationTrackMap;
	BoneAnimationTrackMap bonetracks;

	void addBone (const std::string &parent_bone_name,
			const Vector3f &parent_translation,
			const Vector3f &parent_rotation_ZYXeuler,
			const char* bone_name);

	void addSegment (
			const std::string &bone_name,
			const std::string &segment_name,
			const Vector3f &dimensions,
			const Vector3f &color,
			const MeshData &mesh);

	void addBonePose (const std::string &bone_name,
			float time,
			const Vector3f &bone_translation,
			const Vector3f &bone_rotation_ZYXeuler,
			const Vector3f &bone_scaling
			);

	BonePtr findBone (const char* bone_name) {
		BoneMap::iterator bone_iter = bonemap.find (bone_name);

		if (bone_iter == bonemap.end()) {
			std::cerr << "Error: Could not find bone '" << bone_name << "'!" << std::endl;
			return BonePtr();
		}

		return bone_iter->second;
	}
	void updateBones();

	void draw();
};

#endif
