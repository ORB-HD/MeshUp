#ifndef _ANIMATIONEDITMODEL_H
#define _ANIMATIONEDITMODEL_H

#include <QString>
#include <QAbstractTableModel>
#include <QDoubleSpinBox>

class GLWidget;

class AnimationEditModel : public QAbstractTableModel
{
	Q_OBJECT
	public:
		AnimationEditModel (QObject *parent);
		int rowCount (const QModelIndex &parent = QModelIndex()) const;
		int columnCount (const QModelIndex &parent = QModelIndex()) const;
		QVariant headerData (int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
		QVariant data (const QModelIndex &index, int role = Qt::DisplayRole) const;
		bool setData (const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
		Qt::ItemFlags flags (const QModelIndex &index) const;

		void setGlWidget (GLWidget *gl_widget) {
			glWidget = gl_widget;
		}
		void setTimeSpinBox (QDoubleSpinBox *spinbox) {
			timeDoubleSpinBox = spinbox;
		}

		bool setValue (unsigned int index, double value);
	
		void call_reset();

	private:
		GLWidget *glWidget;
		QDoubleSpinBox *timeDoubleSpinBox;
};

/* _ANIMATIONEDITMODEL_H */
#endif
