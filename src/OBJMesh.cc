#include "GL/glew.h"

#include "OBJMesh.h"

#include "SimpleMath/SimpleMathGL.h"
#include "string_utils.h"

#include <iomanip>
#include <fstream>
#include <limits>

using namespace std;

const bool use_vbo = true;
const string invalid_id_characters = "{}[],;: \r\n\t";

/*
 * Helper structs that are needed for loading the OBJ file.
 */
enum ReadState {
	ReadStateUndefined = 0,
	ReadStateVertices,
	ReadStateInfo,
	ReadStateNormals,
	ReadStateTextureCoordinates,
	ReadStateLast
};

struct FaceInfo {
	FaceInfo () {
		vertex_index[0] = -1;
		vertex_index[1] = -1;
		vertex_index[2] = -1;

		texcoord_index[0] = -1;
		texcoord_index[1] = -1;
		texcoord_index[2] = -1;

		normal_index[0] = -1;
		normal_index[1] = -1;
		normal_index[2] = -1;
	}

	int vertex_index[3];
	int texcoord_index[3];
	int normal_index[3];
};

void OBJMesh::begin() {
	started = true;

	vertices.resize(0);
	normals.resize(0);
	triangle_indices.resize(0);
}

void OBJMesh::end() {
	if (normals.size()) {
		if (normals.size() != triangle_indices.size()) {
			std::cerr << "Error: number of normals must equal the number of vertices specified!" << endl;
			exit (1);
		}
	}

	started = false;
}

unsigned int OBJMesh::generate_vbo() {
	assert (vbo_id == 0);
	assert (started == false);
	assert (vertices.size() != 0);
	assert (vertices.size() % 3 == 0);
	assert (normals.size() == vertices.size());

//	cerr << __func__ << ": vert count = " << vertices.size() << endl;

	// create the buffer
	glGenBuffers (1, &vbo_id);

	// initialize the buffer object
	glBindBuffer (GL_ARRAY_BUFFER, vbo_id);
	glBufferData (GL_ARRAY_BUFFER, sizeof(float) * 3 * (vertices.size() + normals.size()), NULL, GL_STATIC_DRAW);

	// fill the data
	
	// multiple sub buffers
	glBufferSubData (GL_ARRAY_BUFFER, 0, sizeof (float) * 3 * vertices.size(), &vertices[0]);
	glBufferSubData (GL_ARRAY_BUFFER, sizeof(float) * 3 * vertices.size(), sizeof (float) * 3 * normals.size(), &normals[0]);

	glBindBuffer (GL_ARRAY_BUFFER, 0);

	return vbo_id;
}

void OBJMesh::delete_vbo() {
	assert (vbo_id != 0);
	glDeleteBuffers (1, &vbo_id);

	vbo_id = 0;
}

void OBJMesh::addVertice (float x, float y, float z) {
	Vector3f vertex;
	vertex[0] = x;
	vertex[1] = y;
	vertex[2] = z;
	vertices.push_back(vertex);

	bbox_max[0] = max (vertex[0], bbox_max[0]);
	bbox_max[1] = max (vertex[1], bbox_max[1]);
	bbox_max[2] = max (vertex[2], bbox_max[2]);

	bbox_min[0] = min (vertex[0], bbox_min[0]);
	bbox_min[1] = min (vertex[1], bbox_min[1]);
	bbox_min[2] = min (vertex[2], bbox_min[2]);

	triangle_indices.push_back (vertices.size() - 1);
}

void OBJMesh::addVerticefv (float vert[3]) {
	addVertice (vert[0], vert[1], vert[2]);
}

void OBJMesh::addNormal (float x, float y, float z) {
	Vector3f normal;
	normal[0] = x;
	normal[1] = y;
	normal[2] = z;
	normals.push_back (normal);
}

void OBJMesh::addNormalfv (float normal[3]) {
	addNormal (normal[0], normal[1], normal[2]);
}

void OBJMesh::draw() {
	if (smooth_shading)
		glShadeModel(GL_SMOOTH);
	else
		glShadeModel(GL_FLAT);

	if (use_vbo) {
		glBindBuffer (GL_ARRAY_BUFFER, vbo_id);

		glVertexPointer (3, GL_FLOAT, 0, 0);
		glNormalPointer (GL_FLOAT, 0, (const GLvoid *) (vertices.size() * sizeof (float) * 3));

		glEnableClientState (GL_VERTEX_ARRAY);
		glEnableClientState (GL_NORMAL_ARRAY);

		glDrawArrays (GL_TRIANGLES, 0, vertices.size());
		glBindBuffer (GL_ARRAY_BUFFER, 0);
	} else {
		glBegin (GL_TRIANGLES);
		for (int vi = 0; vi < vertices.size(); vi++) {
			glNormal3fv (normals[vi].data());
			glVertex3fv (vertices[vi].data());
		}
		glEnd();
	}
}

bool OBJMesh::loadOBJ (const char *filename, bool strict) {
	return loadOBJ (filename, NULL, strict);
}

bool OBJMesh::loadOBJ (const char *filename, const char *object_name, bool strict) {
	string line, original_line;
	ifstream file_stream (filename);

	if (!file_stream) {
		cerr << "Error: Could not open OBJ file '" << filename << "'!" << endl;

		if (strict)
			exit (1);

		return false;
	}

	ReadState read_state = ReadStateUndefined;
	bool read_object = false;
	string current_object_name = "";
	string material_library = "";
	string material_name = "";
	bool smooth_shading = false;
	bool object_found = false;

	std::vector<Vector4f> vertices;
	std::vector<Vector3f> normals;
	std::vector<Vector3f> texture_coordinates;
	std::vector<FaceInfo> face_infos;
	int line_index = 0;

	while (!file_stream.eof()) {
		getline (file_stream, line);
		line_index ++;

//		cout << "reading line " << line_index << endl;

		original_line = line;
		line = trim_line (line);
		if (line.size() == 0)
			continue;

//		cout << "inp: '" << line << "'" << endl;

		if (line.substr (0, 6) == "mtllib") {
			material_library = line.substr (8, line.size());

			continue;
		}

		if (line.substr (0, 6) == "usemtl") {
			if (line.size() > 9) {
				material_name = line.substr (8, line.size());
			}

			continue;
		}

		if (line.substr (0, 2) == "s ") {
			if (line.substr (2, 3) == "off"
					|| line.substr (2, 1) == "0")
				smooth_shading = false;
			else if (line.substr (2, 2) == "on"
					|| line.substr (2, 1) == "1")
				smooth_shading = true;

			continue;
		}

		if (line[0] == 'o') {
			// we need a copy of the line that still contains the original case
			// as line contains the line transformed to lowercase.
			string object_line = strip_whitespaces(strip_comments(original_line));

			current_object_name = object_line.substr (2, object_line.size());
//			cout << "current_object_name = " << current_object_name << endl;
			if (object_name != NULL && current_object_name == object_name)
				object_found = true;
			
			continue;
		}

		if (line.substr (0,2) == "v ") {
			float v1, v2, v3, v4 = 1.f;
			istringstream values (line.substr (2, line.size()));
			values >> v1;
			values >> v2;
			values >> v3;

			if (!(values >> v4))
				v4 == 1.f;

			vertices.push_back (Vector4f (v1, v2, v3, v4));

//			cerr << "line " << line << " ended up as vertice: " << vertices[vertices.size() -1].transpose() << endl;
			
			continue;
		}

		if (line.substr (0,3) == "vn ") {
			float v1, v2, v3;
			istringstream values (line.substr (3, line.size()));
			values >> v1;
			values >> v2;
			values >> v3;

			normals.push_back (Vector3f (v1, v2, v3));

			continue;
		}	

		if (line.substr (0,3) == "vt ") {
			float v1, v2, v3;
			istringstream values (line.substr (3, line.size()));
			values >> v1;
			values >> v2;
			values >> v3;

			texture_coordinates.push_back (Vector3f (v1, v2, v3));

			continue;
		}	

		if (line.substr (0,2) == "f "
				&& (object_name == NULL || current_object_name == object_name)
				) {
			std::vector<string> tokens = tokenize (line.substr (2, line.size()));
			if (tokens.size() != 3) {
				cerr << "Error: Faces must be triangles! (" << filename << ": " << line_index << ")" << endl;
				cerr << tokens.size() << endl;

				if (strict)
					exit (1);

				return false;
			}

			FaceInfo face_info;

			// read faces
			//
			// the following are valid face definitions:
			//   f v1 v2 v3
			//   f v1/n1 v2/n2 v3/n3
			//   f v1/t1/t1 v2/t2/t2 v3/t3/t3
			//   f v1//t1 v2//t2 v3//t3
			for (int vi = 0; vi < 3; vi++) {
				// first data
				std::vector<string> vertex_tokens = tokenize (tokens[vi], "/");

				if (vertex_tokens.size() == 1) {
					if (vertex_tokens[0].size() > 0) {
						istringstream values (vertex_tokens[0]);
						values >> face_info.vertex_index[vi];
					}
				}

				else if (vertex_tokens.size() == 2) {
					// two possible cases:
					//   v1/t1  or v1//n1
					
					// first one is always the vertex index
					if (vertex_tokens[0].size() > 0) {
						istringstream values (vertex_tokens[0]);
						values >> face_info.vertex_index[vi];
					}

					if (count_char (tokens[vi], "/") == 1) {
						// v1/t1
						if (vertex_tokens[1].size() > 0) {
							istringstream values (vertex_tokens[1]);
							values >> face_info.texcoord_index[vi];
						}
					} else {
						// v1/n1
						if (vertex_tokens[1].size() > 0) {
							istringstream values (vertex_tokens[1]);
							values >> face_info.normal_index[vi];
						}
					}
				}

				else if (vertex_tokens.size() == 3) {
					// two possible cases:
					//   v1/t1/n1

//					cout << "t1 = " << vertex_tokens[0] << " t2 = " << vertex_tokens[1] << " t3 = " << vertex_tokens[2];
					if (vertex_tokens[0].size() > 0) {
						istringstream values (vertex_tokens[0]);
						values >> face_info.vertex_index[vi];
					}
					if (vertex_tokens[1].size() > 0) {
						istringstream values (vertex_tokens[1]);
						values >> face_info.texcoord_index[vi];
					}
					if (vertex_tokens[2].size() > 0) {
						istringstream values (vertex_tokens[2]);
						values >> face_info.normal_index[vi];
					}
				}

//				cout << " parsed line '" << line << "' into: " 
//					<< face_info.vertex_index[vi] << ","
//					<< face_info.texcoord_index[vi] << ","
//					<< face_info.normal_index[vi] << endl;
			}
			face_infos.push_back (face_info);

			continue;
		}
	}

	if (object_name != NULL && object_found == false) {
		cerr << "Warning: could not find object '" << object_name << "' in OBJ file '" << filename << "'" << endl;

		if (strict)
			exit(1);

		return false;
	}

//	cout << "found " << vertices.size() << " vertices" << endl;
//	cout << "found " << normals.size() << " normals" << endl;
//	cout << "found " << face_infos.size() << " faces" << endl;

	begin();
	// add all vertices to the OBJMesh
	for (int fi = 0; fi < face_infos.size(); fi++) {
		for (int vi = 0; vi < 3; vi ++) {
			Vector4f vertex = vertices.at(face_infos[fi].vertex_index[vi] - 1);
			addVertice(vertex[0], vertex[1], vertex[2]);
	//		cout << "added vertice " << vertex.transpose() << endl;

			if (face_infos[fi].normal_index[vi] != -1) {
				Vector3f normal = normals.at(face_infos[fi].normal_index[vi] - 1);
				addNormal(normal[0], normal[1], normal[2]);
			}
		}
	}

	end();

	smooth_shading = smooth_shading;

	file_stream.close();

	return true;
}

