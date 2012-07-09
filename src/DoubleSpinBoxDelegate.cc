#include "DoubleSpinBoxDelegate.h"
#include <QDoubleSpinBox>
#include <QDebug>

DoubleSpinBoxDelegate::DoubleSpinBoxDelegate(QObject *parent)
: QItemDelegate(parent)
{
}

QWidget *DoubleSpinBoxDelegate::createEditor(QWidget *parent,
		const QStyleOptionViewItem &/* option */,
		const QModelIndex &/* index */) const {
	QDoubleSpinBox *editor = new QDoubleSpinBox(parent);
	editor->setMinimum(-1000);
	editor->setMaximum(1000);

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
