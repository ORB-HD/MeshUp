#include <QApplication>

#include "SimpleQtGlApp.h"
#include "glwidget.h"

#include <iostream>

using namespace std;

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	SimpleQtGlApp *main_window = new SimpleQtGlApp;

	main_window->parseArguments (argc, argv);

	main_window->show();
	return app.exec();
}
