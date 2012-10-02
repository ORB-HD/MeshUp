#ifndef _CHECKBOXDELEGATE_H
#define _CHECKBOXDELEGATE_H

#include <QStyledItemDelegate>

class AnimationEditModel;

class CheckBoxDelegate : public QStyledItemDelegate {
	Q_OBJECT

	public:
		CheckBoxDelegate (QObject *parent = 0);

		QWidget *createEditor (QWidget *parent, const QStyleOptionViewItem &option,
				const QModelIndex &index) const;

		void setEditorData(QWidget *editor, const QModelIndex &index) const;
		
		void setModelData(QWidget *editor, QAbstractItemModel *model,
				const QModelIndex &index) const;

		void updateEditorGeometry(QWidget *editor,
				const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

/* _CHECKBOXDELEGATE_H */
#endif

