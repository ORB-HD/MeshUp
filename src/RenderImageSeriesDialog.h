#ifndef RENDERIMAGESERIESDIALOG_H
#define RENDERIMAGESERIESDIALOG_H
 
 

#include "ui_RenderImageSeriesDialog.h"

class RenderImageSeriesDialog : public QDialog, public Ui::RenderImageSeriesDialog {
    Q_OBJECT
 
public:
		RenderImageSeriesDialog (QWidget *parent = 0) {
			setupUi(this);
		}

};
#endif
