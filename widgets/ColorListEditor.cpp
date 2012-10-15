/*****************************************************************************
 *
 * $Id$
 *
 * Copyright (C) 2012  Florian Pose <fp@igh-essen.com>
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

#include <QtGui>

#include "ColorListEditor.h"

using DLS::ColorListEditor;

/****************************************************************************/

ColorListEditor::ColorListEditor(QWidget *widget):
    QComboBox(widget)
{
    populateList();
}

/****************************************************************************/

QColor ColorListEditor::color() const
{
    return qVariantValue<QColor>(itemData(currentIndex(),
                Qt::DecorationRole));
}

/****************************************************************************/

void ColorListEditor::setColor(QColor color)
{
    setCurrentIndex(findData(color, int(Qt::DecorationRole)));
}

/****************************************************************************/

void ColorListEditor::populateList()
{
    QStringList colorNames = QColor::colorNames();

    for (int i = 0; i < colorNames.size(); ++i) {
        QColor color(colorNames[i]);

        insertItem(i, colorNames[i]);
        setItemData(i, color, Qt::DecorationRole);
    }
}

/****************************************************************************/
