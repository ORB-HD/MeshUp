#include "AnimationEditModel.h"
#include <glwidget.h>
#include "Animation.h"

#include <QDebug>
#include <string>

using namespace std;

AnimationEditModel::AnimationEditModel(QObject *parent) : 
	QAbstractTableModel(parent),
	glWidget (NULL)
{
}

int AnimationEditModel::rowCount (const QModelIndex &/* parent */) const {
	if (!glWidget)
		return 0;

	return glWidget->animation_data->column_infos.size();
}

int AnimationEditModel::columnCount (const QModelIndex &/* parent */) const {
	if (!glWidget)
		return 0;

	return 3;
}

QVariant AnimationEditModel::headerData (int section, Qt::Orientation orientation, int role) const {
	if (role == Qt::DisplayRole) {
		if (orientation == Qt::Horizontal) {
			switch (section) {
				case 0: return QString ("Column"); break;
				case 1: return QString ("Value"); break;
				case 2: return QString ("Key"); break;
			}
		}
	}

	return QVariant();
}

QVariant AnimationEditModel::data (const QModelIndex &index, int role) const {
	float animation_time = glWidget->animation_data->current_time;
	if (role == Qt::DisplayRole) {
		if (index.column() == 0) {
			QString frame_name;
			QString axis;
			QString type;

			ColumnInfo info = glWidget->animation_data->column_infos[index.row()];
			frame_name = info.frame_name.c_str();

			if (info.is_time_column)
				return QString("Time");

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

		if (index.column() == 1) {
			if (index.row() == 0)
				return animation_time;
			return QString ("%1")
				.arg (glWidget->animation_data->getRawDataInterpolatedValue (index.row(), animation_time), 0, 'g', 4);
		}

		return QString ("Row%1, Column%2")
			.arg(index.row() + 1)
			.arg(index.column() + 1);
	} else if (role == Qt::FontRole) {
		if (index.column() == 1) {
			if ((index.row() == 0 && glWidget->animation_data->haveRawKeyValues (animation_time))	
					|| (index.row() > 0 && glWidget->animation_data->haveRawKeyValue (index.row(), animation_time))) {
				QFont boldFont;
				boldFont.setBold(true);
				return boldFont;
			}
		}
	} else if (role == Qt::TextAlignmentRole) {
		if (index.column() == 1)
			return Qt::AlignRight + Qt::AlignVCenter;
	} else if (role == Qt::EditRole) {
		if (index.column() == 1) {
			if (index.row() == 0) {
				return animation_time;
			}
			return QString ("%1")
				.arg (glWidget->animation_data->getRawDataInterpolatedValue (index.row() + 1, animation_time), 0, 'g', 4);
		}
	}

	return QVariant();
}

bool AnimationEditModel::setData (const QModelIndex &index, const QVariant &value, int role) {
	if (role == Qt::EditRole) {
		if (index.row() == 0) {
			qDebug() << "Setting of time not yet supported!";
			return false;
		}

		setValue (index.row(), value.toDouble());

		emit animationModified();

		return true;
	}

	return false;
}

Qt::ItemFlags AnimationEditModel::flags (const QModelIndex &index) const { 
	float animation_time = glWidget->animation_data->current_time;

	if (index.column() == 1 && 
			glWidget->animation_data->haveRawKeyValue (index.row() + 1, animation_time)) {
		return Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled;
	}

	return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

void AnimationEditModel::setValue (unsigned int index, double value) {
	float animation_time = glWidget->animation_data->current_time;

	glWidget->animation_data->setRawDataKeyValue (animation_time, index, value);
}

void AnimationEditModel::call_reset() {
	reset();
}

