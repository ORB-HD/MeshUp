// RenderVideoDialog.h
// header file for render video dialog box
// Author: Stephen Meisenbacher

#ifndef RENDERVIDEODIALOG_H
#define RENDERVIDEODIALOG_H

class RenderVideoDialog;

#include "ui_RenderVideoDialog.h"

class RenderVideoDialog : public QDialog, public Ui::RenderVideoDialog {
    Q_OBJECT

public:
    RenderVideoDialog (QWidget *parent = 0) {
			   setupUi(this);
		}

};
#endif
