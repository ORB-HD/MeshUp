#include <QCheckBox>
#include <QDebug>

#include "ui/CheckBoxDelegate.h"

CheckBoxDelegate::CheckBoxDelegate(QObject *parent)
: QStyledItemDelegate(parent)
{
}

QWidget *CheckBoxDelegate::createEditor(QWidget *parent,
		const QStyleOptionViewItem &/* option */,
		const QModelIndex &index) const {
	QCheckBox *editor = new QCheckBox(parent);

	return editor;
}

void CheckBoxDelegate::setEditorData(QWidget *editor,
		const QModelIndex &index) const {
	bool value = index.model()->data(index, Qt::EditRole).toBool();

	QCheckBox *checkBox = static_cast<QCheckBox*>(editor);
	checkBox->setChecked(value);
}

void CheckBoxDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
		const QModelIndex &index) const {
	QCheckBox *checkBox = static_cast<QCheckBox*>(editor);
	bool value = checkBox->isChecked();

	model->setData(index, value, Qt::EditRole);
}

void CheckBoxDelegate::updateEditorGeometry(QWidget *editor,
		const QStyleOptionViewItem &option, const QModelIndex &/* undex */) const {
	editor->setGeometry(option.rect);
}
