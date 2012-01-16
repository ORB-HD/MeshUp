#include <QApplication>

#include "MeshupApp.h"
#include "glwidget.h"

#include <iostream>

using namespace std;

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	MeshupApp *main_window = new MeshupApp;

	main_window->parseArguments (argc, argv);

	main_window->show();
	return app.exec();
}
