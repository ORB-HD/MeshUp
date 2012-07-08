#include "AnimationEditModel.h"
#include <glwidget.h>
#include "Animation.h"

#include <string>

using namespace std;

AnimationEditModel::AnimationEditModel(QObject *parent) : 
	QAbstractTableModel(parent),
	glWidget (NULL)
{}

int AnimationEditModel::rowCount (const QModelIndex &parent) const {
	if (!glWidget)
		return 0;

	return glWidget->animation_data->column_infos.size() - 1;
}

int AnimationEditModel::columnCount (const QModelIndex &parent) const {
	if (!glWidget)
		return 0;

	return 3;
}

QVariant AnimationEditModel::data (const QModelIndex &index, int role) const {
	if (role == Qt::DisplayRole) {
		if (index.column() == 0) {
			QString frame_name;
			QString axis;
			QString type;

			ColumnInfo info = glWidget->animation_data->column_infos[index.row() + 1];
			frame_name = info.frame_name.c_str();

			switch (info.type) {
				case ColumnInfo::TransformTypeRotation: type = "R"; break;
				case ColumnInfo::TransformTypeTranslation: type = "T"; break;
				case ColumnInfo::TransformTypeScale: type = "S"; break;
				default: type = "?"; break;
			}

			switch (info.axis) {
				case ColumnInfo::AxisTypeX: axis = "X"; break;
				case ColumnInfo::AxisTypeY: axis = "Y"; break;
				case ColumnInfo::AxisTypeZ: axis = "Z"; break;
				case ColumnInfo::AxisTypeNegativeX: axis = "-X"; break;
				case ColumnInfo::AxisTypeNegativeY: axis = "-Y"; break;
				case ColumnInfo::AxisTypeNegativeZ: axis = "-Z"; break;
			}
			return QString ("%1:%2:%3")
				.arg(frame_name)
				.arg(type)
				.arg(axis);
		}

		return QString ("Row%1, Column%2")
			.arg(index.row() + 1)
			.arg(index.column() + 1);
	}
	return QVariant();
}

void AnimationEditModel::call_reset() {
	reset();
}

