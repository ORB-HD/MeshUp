#include <QDoubleSpinBox>
#include <QDebug>

#include "DoubleSpinBoxDelegate.h"
#include "AnimationEditModel.h"

DoubleSpinBoxDelegate::DoubleSpinBoxDelegate(QObject *parent)
: QItemDelegate(parent),
	animationEditModel(NULL)
{
}

QWidget *DoubleSpinBoxDelegate::createEditor(QWidget *parent,
		const QStyleOptionViewItem &/* option */,
		const QModelIndex &index) const {
	QDoubleSpinBox *editor = new QDoubleSpinBox(parent);

	/*
	 * Here we make rather hacky casts to be able to update the values
	 * directly in the animation without having to close the edit widget.
	 *
	 * We store both the current model and the index in this
	 * DoubleSpinBoxDelegate and update the model whenever the QDoubleSpinBox
	 * was changed.
	 */
	DoubleSpinBoxDelegate *noconst_this = const_cast<DoubleSpinBoxDelegate*>(static_cast<const DoubleSpinBoxDelegate*>(this));
	AnimationEditModel *noconst_model = const_cast<AnimationEditModel*>(static_cast<const AnimationEditModel*>(index.model()));

	noconst_this->animationEditModel = noconst_model;
	noconst_this->setModelIndex (index);

	editor->setMinimum(-1000);
	editor->setMaximum(1000);

	connect (editor, SIGNAL(valueChanged(double)), this, SLOT(setValue(double)));

	return editor;
}

void DoubleSpinBoxDelegate::setEditorData(QWidget *editor,
		const QModelIndex &index) const {
	double value = index.model()->data(index, Qt::EditRole).toDouble();

	QDoubleSpinBox *spinBox = static_cast<QDoubleSpinBox*>(editor);
	spinBox->setValue(value);
}

void DoubleSpinBoxDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
		const QModelIndex &index) const {
	QDoubleSpinBox *spinBox = static_cast<QDoubleSpinBox*>(editor);
	spinBox->interpretText();
	double value = spinBox->value();

	model->setData(index, value, Qt::EditRole);
}

void DoubleSpinBoxDelegate::updateEditorGeometry(QWidget *editor,
		const QStyleOptionViewItem &option, const QModelIndex &/* undex */) const {
	editor->setGeometry(option.rect);
}

void DoubleSpinBoxDelegate::setValue(double val) {
	if (animationEditModel) {
		const_cast<AnimationEditModel*>(animationEditModel)->setValue (edit_index.row() + 1, val);
	}
}
