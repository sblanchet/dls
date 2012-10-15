/*****************************************************************************
 *
 * $Id$
 *
 * Copyright (C) 2009 - 2012  Florian Pose <fp@igh-essen.com>
 *
 * This file is part of the DLS widget library.
 *
 * The DLS widget library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the License,
 * or (at your option) any later version.
 *
 * The DLS widget library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with the DLS widget library. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 ****************************************************************************/

#include <QDebug>

#include "ColorDelegate.h"
#include "ColorListEditor.h"

using DLS::ColorListEditor;

/****************************************************************************/

ColorDelegate::ColorDelegate(QObject *parent):
    QItemDelegate(parent)
{
}

/****************************************************************************/

QWidget *ColorDelegate::createEditor(QWidget *parent,
        const QStyleOptionViewItem &,
        const QModelIndex &) const
{
    ColorListEditor *editor = new ColorListEditor(parent);
    return editor;
}

/****************************************************************************/

void ColorDelegate::setEditorData(QWidget *editor,
        const QModelIndex &index) const
{
    QColor color;
    color = QColor::fromRgb(index.model()->data(index, Qt::EditRole).toInt());

    ColorListEditor *colorListEditor = static_cast<ColorListEditor *>(editor);
    colorListEditor->setColor(color);
}

/****************************************************************************/

void ColorDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
        const QModelIndex &index) const
{
    ColorListEditor *colorListEditor = static_cast<ColorListEditor *>(editor);
    QColor color = colorListEditor->color();

    model->setData(index, color.rgb(), Qt::EditRole);
}

/****************************************************************************/

void ColorDelegate::updateEditorGeometry(QWidget *editor,
        const QStyleOptionViewItem &option, const QModelIndex &) const
{
    editor->setGeometry(option.rect);
}

/****************************************************************************/
