/*
 * MeshUp - A visualization tool for multi-body systems based on skeletal
 * animation and magic.
 *
 * Copyright (c) 2011-2012 Martin Felis <martin.felis@iwr.uni-heidelberg.de>
 *
 * Licensed under the MIT license. See LICENSE for more details.
 */

#include <QApplication>

#include "MeshupApp.h"
#include "glwidget.h"

#include <iostream>

using namespace std;

int main(int argc, char *argv[])
{
	setup_unix_signal_handlers();
	QApplication app(argc, argv);
	MeshupApp *main_window = new MeshupApp;

	main_window->parseArguments (argc, argv);

	main_window->show();
	return app.exec();
}
