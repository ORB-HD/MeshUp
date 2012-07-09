#ifndef _DOUBLESPINBOXDELEGATE_H
#define _DOUBLESPINBOXDELEGATE_H

#include <QItemDelegate>

class AnimationEditModel;

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

		void setAnimationEditModel (AnimationEditModel *edit_model) {
			animationEditModel = edit_model;
		}

		void setModelIndex (QModelIndex index) {
			edit_index = index;
		}

	public slots:
		void setValue (double val);

	private:
		AnimationEditModel *animationEditModel;
		QModelIndex edit_index;
};

/* _DOUBLESPINBOXDELEGATE_H */
#endif

