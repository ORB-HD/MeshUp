#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QTimer>
#include "ui_MainWindow.h"

class SimpleQtGlApp : public QMainWindow, public Ui::MainWindow
{
    Q_OBJECT
 
public:
    SimpleQtGlApp(QWidget *parent = 0);

protected:
		QTimer *timer;

};
 
#endif
