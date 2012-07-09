#ifndef _DOUBLESPINBOXDELEGATE_H
#define _DOUBLESPINBOXDELEGATE_H

#include <QItemDelegate>

class DoubleSpinBoxDelegate : public QItemDelegate {
	Q_OBJECT

	public:
		DoubleSpinBoxDelegate (QObject *parent = 0);

		QWidget *createEditor (QWidget *parent, const QStyleOptionViewItem &option,
				const QModelIndex &index) const;

		void setEditorData(QWidget *editor, const QModelIndex &index) const;
		void setModelData(QWidget *editor, QAbstractItemModel *model,
				const QModelIndex &index) const;

		void updateEditorGeometry(QWidget *editor,
				const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

/* _DOUBLESPINBOXDELEGATE_H */
#endif

