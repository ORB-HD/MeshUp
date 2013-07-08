#include <iostream>
#include <cstdlib>

#include <Model.h>

using namespace std;

void usage(const char *argv0) {
	cout << "Usage: " << argv0 << " <infile> <outfile>" << endl;
	cout << "Converts MeshUp models from Json format to Lua format and vice-versa." << endl;
	exit(1);
}

int main (int argc, char* argv[]) {
	if (argc != 3) {
		usage(argv[0]);
	}

	for (int i = 0; i < argc; i++) {
		if (string(argv[i]) == "-h" || string(argv[i]) == "--help")
			usage(argv[0]);
	}

	string infile = argv[1];
	string outfile = argv[2];

	MeshupModel model;
	model.skip_vbo_generation = true;

	if (!model.loadModelFromFile(infile.c_str())) {
		cerr << "Error loading " << infile << endl;
		abort();
	}

	model.saveModelToFile (outfile.c_str());

	cout << "Conversion successful!" << endl;

	return 0;
}
