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

#include "lib_channel.hpp"

#include "Graph.h"
#include "Section.h"
#include "Layer.h"

using DLS::Section;
using DLS::Layer;

/****************************************************************************/

/** Constructor.
 */
Section::Section(
        Graph *graph
        ):
    graph(graph),
    height(100),
    isDropTarget(false)
{
}

/****************************************************************************/

/** Destructor.
 */
Section::~Section()
{
}

/****************************************************************************/

/** Gives a hint aboute the optimal size.
 */
void Section::setDropTarget(bool d)
{
    isDropTarget = d;
}

/****************************************************************************/

void Section::draw(QPainter &painter, const QRect &rect) const
{
    QString legend;
    for (QList<Layer *>::const_iterator l = layers.begin();
            l != layers.end(); l++) {
        if (!legend.isEmpty()) {
            legend += ", ";
        }
        legend += (*l)->getChannel()->name().c_str();
    }

    QFont f = painter.font();
    f.setPointSize(8);
    QFontMetrics fm(f);

    QRect textRect(rect);
    textRect.setWidth(rect.width() - 4);
    QRect bound = fm.boundingRect(textRect, Qt::TextWordWrap, legend);

    QRect legendRect(rect);
    legendRect.setHeight(bound.height() + 4);
    QRect dataRect(rect);
    dataRect.setTop(legendRect.bottom() + 1);

    QColor legendColor(graph->palette().window().color());
    painter.fillRect(legendRect, legendColor);

    QPen pen;
    painter.setPen(pen);
    painter.setFont(f);
    painter.drawText(legendRect.adjusted(2, 2, -2, -2),
            Qt::AlignLeft, legend);

    for (QList<Layer *>::const_iterator l = layers.begin();
            l != layers.end(); l++) {
        (*l)->draw(painter, dataRect);
    }

    if (isDropTarget) {
        QPen pen;
        pen.setColor(Qt::blue);
        painter.setPen(pen);

        QBrush brush;
        brush.setColor(QColor(0, 0, 255, 63));
        brush.setStyle(Qt::SolidPattern);
        painter.setBrush(brush);

        painter.drawRect(rect);
    }
}

/****************************************************************************/

Layer *Section::appendLayer(LibDLS::Channel *ch)
{
    Layer *l = new Layer(this, ch);
    layers.append(l);
    graph->updateRange();
    return l;
}

/****************************************************************************/

void Section::getRange(bool &valid, COMTime &start, COMTime &end)
{
    for (QList<Layer *>::const_iterator l = layers.begin();
            l != layers.end(); l++) {
        LibDLS::Channel *ch = (*l)->getChannel();
        if (valid) {
            COMTime t = ch->start();
            if (t < start) {
                start = t;
            }
            t = ch->end();
            if (t > end) {
                end = t;
            }
        }
        else {
            start = ch->start();
            end = ch->end();
            valid = true;
        }
    }
}

/****************************************************************************/

void Section::loadData(const COMTime &start, const COMTime &end,
        int min_values)
{
    for (QList<Layer *>::const_iterator l = layers.begin();
            l != layers.end(); l++) {
        (*l)->loadData(start, end, min_values);
    }
}

/****************************************************************************/
