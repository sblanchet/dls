/*****************************************************************************
 *
 * $Id$
 *
 ****************************************************************************/

#include <QDebug>

#include "SectionModel.h"
#include "Section.h"
#include "Layer.h"

#include "lib_channel.hpp"

using namespace DLS;

/*****************************************************************************/

SectionModel::SectionModel(Section *section):
    section(section)
{
}

/****************************************************************************/

SectionModel::~SectionModel()
{
}

/****************************************************************************/

/** Implements the model interface.
 */
int SectionModel::rowCount(const QModelIndex &index) const
{
    int ret = 0;

    if (index.isValid()) {
        ret = 0;
    }
    else {
        return section->layers.size();
    }

    return ret;
}

/****************************************************************************/

/** Implements the model interface.
 *
 * \returns Number of columns.
 */
int SectionModel::columnCount(const QModelIndex &index) const
{
    Q_UNUSED(index);
    return 6;
}

/****************************************************************************/

/** Implements the SectionModel interface.
 */
QVariant SectionModel::data(const QModelIndex &index, int role) const
{
    QVariant ret;

    if (index.isValid()) {
        int row = index.row();
        Layer *layer = section->layers[row];

        if (role == Qt::DisplayRole) {
            switch (index.column()) {
                case 0:
                    ret = layer->getChannel()->name().c_str();
                    break;
                case 1:
                    ret = layer->getName();
                    break;
                case 2:
                    ret = layer->getUnit();
                    break;
                case 3:
                    ret = layer->getColor().name();
                    break;
                case 4:
                    ret = layer->getScale();
                    break;
                case 5:
                    ret = layer->getOffset();
                    break;
                default:
                    break;
            }
        }
        else if (role == Qt::EditRole) {
            switch (index.column()) {
                case 1:
                    ret = layer->getName();
                    break;
                case 2:
                    ret = layer->getUnit();
                    break;
                case 3:
                    ret = layer->getColor().rgb();
                    break;
                case 4: 
                    ret = QLocale().toString(layer->getScale());
                    break;
                case 5:
                    ret = QLocale().toString(layer->getOffset());
                    break;
                default:
                    break;
            }
        }
        else if (index.column() == 3 && role == Qt::DecorationRole) {
            ret = layer->getColor();
        }
    }

    return ret;
}

/****************************************************************************/

/** Implements the SectionModel interface.
 */
QVariant SectionModel::headerData(
        int section,
        Qt::Orientation o,
        int role
        ) const
{
    QVariant ret;

    if (o == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
            case 0:
                ret = tr("Channel");
                break;
            case 1:
                ret = tr("Name");
                break;
            case 2:
                ret = tr("Unit");
                break;
            case 3:
                ret = tr("Color");
                break;
            case 4:
                ret = tr("Scale");
                break;
            case 5:
                ret = tr("Offset");
                break;
            default:
                break;
        }
    }

    return ret;
}

/****************************************************************************/

/** Implements the SectionModel interface.
 */
Qt::ItemFlags SectionModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags f;

    if (index.isValid()) {
        f |= Qt::ItemIsEnabled;

        if (index.column() > 0) {
            f |= Qt::ItemIsEditable;
        }
    }

    return f;
}

/****************************************************************************/

bool SectionModel::setData(const QModelIndex &index, const QVariant &value,
        int role)
{
    bool accepted = false;

    if (role != Qt::EditRole) {
        qDebug() << "not edit role: " << role;
        return accepted;
    }

    int row = index.row();
    Layer *layer = section->layers[row];

    switch (index.column()) {
        case 1:
            layer->setName(value.toString());
            accepted = true;
            break;
        case 2:
            layer->setUnit(value.toString());
            accepted = true;
            break;
        case 3: {
            QColor color;
            color = color.fromRgb(value.toInt());
            layer->setColor(color);
            accepted = true;
            }
            break;
        case 4: {
                bool ok;
                double num = QLocale().toDouble(value.toString(), &ok);
                if (ok) {
                    layer->setScale(num);
                    accepted = true;
                }
            }
            break;
        case 5: {
                bool ok;
                double num = QLocale().toDouble(value.toString(), &ok);
                if (ok) {
                    layer->setOffset(num);
                    accepted = true;
                }
            }
            break;
        default:
            break;
    }

    if (accepted) {
        emit dataChanged(index, index);
    }
    return accepted;
}

/****************************************************************************/
