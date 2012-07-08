#ifndef _ANIMATIONEDITMODEL_H
#define _ANIMATIONEDITMODEL_H

#include <QAbstractTableModel>

class AnimationEditModel : public QAbstractTableModel
{
	Q_OBJECT
	public:
		AnimationEditModel (QObject *parent);
		int rowCount (const QModelIndex &parent = QModelIndex()) const;
		int columnCount (const QModelIndex &parent = QModelIndex()) const;
		QVariant data (const QModelIndex &index, int role = Qt::DisplayRole) const;
};

/* _ANIMATIONEDITMODEL_H */
#endif
