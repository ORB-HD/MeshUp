#include "ModelData.h"

#include <iostream>
#include <fstream>
#include <cmath>
#include <cctype>

using namespace std;

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

const std::string whitespaces_std (" \t\n\r");

string strip_comments (const std::string &line) {
	return line.substr (0, line.find ('#'));
}

string strip_whitespaces (const std::string &line, string whitespaces = whitespaces_std) {
	string result (line);
	if (result.find (whitespaces) != string::npos) {
		result = result.substr (result.find_first_not_of (whitespaces), result.size());
	}

	while (whitespaces.find (result[result.size() - 1]) != string::npos) {
		result = result.substr (0, result.size() - 1);
	}
	return result;
}

string tolower (const std::string &line) {
	string result (line);
	for (int i = 0; i < line.size(); i++) 
		result[i] = tolower(result[i]);

	return result;
}

string trim_line (const std::string &line) {
	return tolower (strip_whitespaces (strip_comments (line)));
}

std::vector<string> tokenize (const std::string &line_in, string delimiter=whitespaces_std) {
	std::vector<string> result;
	string line = line_in;

	while (line.find_first_of (delimiter) != string::npos) {
		string token = line.substr (0, line.find_first_of (delimiter));
		line = line.substr (token.size() + 1, line.size());
		result.push_back (token);
	}

	if (line.size() > 0)
		result.push_back (line);

	return result;
}

void loadOBJ (MeshData *mesh, const char *filename) {
	string line;
	ifstream file_stream (filename);

	if (!file_stream) {
		cerr << "Error: Could not open OBJ file '" << filename << "'!" << endl;
		exit (1);
	}

	ReadState read_state = ReadStateUndefined;
	bool read_object = false;
	string object_name = "";
	string material_library = "";
	string material_name = "";
	bool smooth_shading = false;

	std::vector<Vector4f> vertices;
	std::vector<Vector3f> normals;
	std::vector<Vector3f> texture_coordinates;
	std::vector<FaceInfo> face_infos;
	int line_index = 0;

	while (!file_stream.eof()) {
		getline (file_stream, line);
		line_index ++;

//		cout << "reading line " << line_index << endl;

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
			if (object_name != "") {
				cerr << "Warning: multiple objects in OBJ file detected! Using '" << object_name << "', discarding others" << endl;
				break;
			}
			object_name = line.substr (2, line.size());
			
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

		if (line.substr (0,2) == "f ") {
			std::vector<string> tokens = tokenize (line.substr (2, line.size()));
			if (tokens.size() != 3) {
				cerr << "Error: Faces must be triangles! (" << filename << ": " << line_index << ")" << endl;
				cerr << tokens.size() << endl;
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
					if (vertex_tokens[0].size() > 0) {
						istringstream values (vertex_tokens[0]);
						values >> face_info.vertex_index[vi];
					}
					if (vertex_tokens[1].size() > 0) {
						istringstream values (vertex_tokens[1]);
						values >> face_info.texcoord_index[vi];
					}
				}

				else if (vertex_tokens.size() == 3) {
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

//	cout << "found " << vertices.size() << " vertices" << endl;
//	cout << "found " << normals.size() << " normals" << endl;
//	cout << "found " << face_infos.size() << " faces" << endl;

	mesh->begin();
	// add all vertices to the MeshData
	for (int fi = 0; fi < face_infos.size(); fi++) {
		for (int vi = 0; vi < 3; vi ++) {
			Vector4f vertex = vertices.at(face_infos[fi].vertex_index[vi] - 1);
			mesh->addVertice(vertex[0], vertex[1], vertex[2]);
	//		cout << "added vertice " << vertex.transpose() << endl;

			if (face_infos[fi].normal_index[vi] != -1) {
				Vector3f normal = normals.at(face_infos[fi].normal_index[vi] - 1);
				mesh->addNormal(normal[0], normal[1], normal[2]);
			}
		}
	}

	mesh->end();

	mesh->smooth_shading = smooth_shading;

	file_stream.close();
}

