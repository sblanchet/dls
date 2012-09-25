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

void Section::draw(QPainter &painter, int y, int width) const
{
    QPen pen;
    QBrush brush;

    pen.setColor(Qt::red);
    painter.setPen(pen);

    brush.setColor(QColor(0, 255, 0, 10));
    brush.setStyle(Qt::SolidPattern);
    painter.setBrush(brush);

    painter.drawRect(0, y, width - 1, height - 1);

    if (isDropTarget) {
        pen.setColor(Qt::blue);
        pen.setStyle(Qt::SolidLine);
        pen.setWidth(1);
        painter.setPen(pen);

        brush.setColor(QColor(0, 0, 255, 63));
        brush.setStyle(Qt::SolidPattern);
        painter.setBrush(brush);

        painter.drawRect(5, y + 5, width - 10, height - 10);
    }

    pen.setColor(Qt::black);
    QFont f = painter.font();
    f.setPixelSize(8);

    int off = 0;
    for (QList<Layer *>::const_iterator l = layers.begin();
            l != layers.end(); l++) {
        (*l)->draw(painter, y, width);

        painter.setPen(pen);
        painter.setFont(f);
        painter.drawText(5, y + off + 5, width - 10, 10, Qt::AlignLeft,
                (*l)->getChannel()->name().c_str(), NULL);
        off += 10;
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
