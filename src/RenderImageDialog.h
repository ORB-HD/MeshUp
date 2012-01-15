#ifndef RENDERIMAGEDIALOG_H
#define RENDERIMAGEDIALOG_H
 
#include "ui_RenderImageDialog.h"

class RenderImageDialog : public QDialog, public Ui::RenderImageDialog {
    Q_OBJECT
 
public:
		RenderImageDialog (QWidget *parent = 0) {
			setupUi(this);
		}

};
#endif
