#include "ui/AnimationEditModel.h"
#include <glwidget.h>
#include "Animation.h"

#include <QDebug>
#include <string>

using namespace std;

AnimationEditModel::AnimationEditModel(QObject *parent) : 
	QAbstractTableModel(parent),
	glWidget (NULL),
	timeDoubleSpinBox(NULL)
{
	columnFields[0] = ColumnFieldKeyName;
	columnFields[1] = ColumnFieldValue;
	columnFields[2] = ColumnFieldKeyFrameFlag;
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
			switch (columnFields[section]) {
				case ColumnFieldKeyName: return QString ("Column"); break;
				case ColumnFieldValue: return QString ("Value"); break;
				case ColumnFieldKeyFrameFlag: return QString ("Key"); break;
			}
		}
	}
	return QVariant();
}

QVariant AnimationEditModel::data (const QModelIndex &index, int role) const {
	float animation_time = timeDoubleSpinBox->value();
	if (role == Qt::DisplayRole) {
		if (columnFields[index.column()] == ColumnFieldKeyName) {
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

		if (columnFields[index.column()] == ColumnFieldValue) {
			if (index.row() == 0)
				return animation_time;
			return QString ("%1")
				.arg (glWidget->animation_data->values.getInterpolatedValue (animation_time, index.row()), 0, 'g', 4);
		}

		if (index.row() > 0 && columnFields[index.column()] == ColumnFieldKeyFrameFlag) {
			return QVariant();
		}
	} else if (role == Qt::FontRole) {
		if (columnFields[index.column()] == ColumnFieldValue) {
			if (glWidget->animation_data->values.haveKeyValue (animation_time, index.row())) {
				QFont boldFont;
				boldFont.setBold(true);
				return boldFont;
			}
		}
	} else if (role == Qt::TextAlignmentRole) {
		if (columnFields[index.column()] == ColumnFieldValue)
			return Qt::AlignRight + Qt::AlignVCenter;
		else if (columnFields[index.column()] == ColumnFieldKeyFrameFlag) {
			return Qt::AlignHCenter;
		}
	} else if (role == Qt::EditRole) {
		if (columnFields[index.column()] == ColumnFieldValue) {
			if (index.row() == 0) {
				return animation_time;
			}
			return QString ("%1")
				.arg (glWidget->animation_data->values.getInterpolatedValue (animation_time, index.row()), 0, 'g', 4);
		} else if (columnFields[index.column()] == ColumnFieldKeyFrameFlag) {
			if (glWidget->animation_data->values.haveKeyValue (animation_time, index.row())) {
				return Qt::Checked;
			} else {
				return Qt::Unchecked;
			}
		}
	} else if (role == Qt::CheckStateRole) {
		if (index.row() > 0 && columnFields[index.column()] == ColumnFieldKeyFrameFlag) {
			if (glWidget->animation_data->values.haveKeyValue (animation_time, index.row())) {
				return Qt::Checked;
			} else {
				return Qt::Unchecked;
			}
		}
	}

	if (index.row() > 0 && columnFields[index.column()] == ColumnFieldKeyFrameFlag)
		return QVariant(0);

	return QVariant();
}

bool AnimationEditModel::setData (const QModelIndex &index, const QVariant &value, int role) {
	if (role == Qt::EditRole) {
		float animation_time = timeDoubleSpinBox->value();
		if (columnFields[index.column()] == ColumnFieldKeyFrameFlag && index.row() > 0) {
			if (!value.toBool()) {
				float current_value = glWidget->animation_data->values.getInterpolatedValue (animation_time, index.row());
				glWidget->animation_data->values.addKeyValue (animation_time, index.row(), current_value);
				glWidget->animation_data->updateAnimationFromRawValues();
			} else {
//				qDebug() << "deleting value";
				glWidget->animation_data->values.deleteKeyValue (animation_time, index.row());
				glWidget->animation_data->updateAnimationFromRawValues();
			}

			emit dataChanged(index, index);
			return true;
		} else {
			emit dataChanged(index, index);
			return setValue (index.row(), value.toDouble());
		}
	}

	return false;
}

Qt::ItemFlags AnimationEditModel::flags (const QModelIndex &index) const { 
	float animation_time = timeDoubleSpinBox->value();

	Qt::ItemFlags editable_flags = Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled;
	Qt::ItemFlags readonly_flags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;

	if (columnFields[index.column()] == ColumnFieldValue && index.row() == 0 && fabs(animation_time) < 1.0e-5)
		return readonly_flags;

	if (columnFields[index.column()] == ColumnFieldValue && 
			glWidget->animation_data->values.haveKeyValue (animation_time, index.row())) {

		return editable_flags;
	}

	if (index.row() > 0 && columnFields[index.column()] == ColumnFieldKeyFrameFlag) {
		return Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable;
	}

	return readonly_flags;
}

bool AnimationEditModel::setValue (unsigned int index, double value) {
	float animation_time = timeDoubleSpinBox->value();

	if (index == 0) {
		// we do not want to move the first keyframe
		if (fabs(animation_time) < 1.0e-5)
			return false;

		// we only want to move existing keyframes
		if (!glWidget->animation_data->values.haveKeyFrame(animation_time))
			return false;

		glWidget->animation_data->values.moveKeyFrame (animation_time, value);
	} else {
		glWidget->animation_data->values.addKeyValue (animation_time, index, value);
	}

	glWidget->animation_data->updateAnimationFromRawValues();

	return true;
}

void AnimationEditModel::call_reset() {
	reset();
}
