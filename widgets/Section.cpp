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

const QColor Section::colorList[] = {
    Qt::blue,
    Qt::red,
    Qt::darkGreen,
    Qt::black
};

/****************************************************************************/

/** Constructor.
 */
Section::Section(
        Graph *graph
        ):
    graph(graph),
    height(100)
{
}

/****************************************************************************/

/** Destructor.
 */
Section::~Section()
{
}

/****************************************************************************/

void Section::draw(QPainter &painter, const QRect &rect)
{
    legend.setPageSize(rect.size());

    QRect legendRect(rect);
    legendRect.setHeight(legend.size().height());
    legendRect = legendRect.intersected(rect);
    QRect dataRect(rect);
    dataRect.setTop(legendRect.bottom() + 1);

    QColor legendColor(graph->palette().window().color());
    painter.fillRect(legendRect, legendColor);

    painter.translate(rect.topLeft());
    legend.drawContents(&painter, QRect(QPoint(), rect.size()));
    painter.resetTransform();

    if (!dataRect.isValid()) {
        return;
    }

    for (QList<Layer *>::const_iterator l = layers.begin();
            l != layers.end(); l++) {
        (*l)->draw(painter, dataRect);
    }
}

/****************************************************************************/

Layer *Section::appendLayer(LibDLS::Channel *ch)
{
    Layer *l = new Layer(this, ch);
    layers.append(l);
    updateLegend();
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

QColor Section::nextColor() const
{
    unsigned int used[sizeof(colorList) / sizeof(QColor)];

    for (unsigned int i = 0; i < sizeof(colorList) / sizeof(QColor); i++) {
        used[i] = 0;
    }

    // count usage and find maximum
    unsigned int max = 0U;
    for (QList<Layer *>::const_iterator l = layers.begin();
            l != layers.end(); l++) {
        for (unsigned int i = 0; i < sizeof(colorList) / sizeof(QColor);
                i++) {
            if (colorList[i] == (*l)->getColor()) {
                used[i]++;
                if (used[i] > max) {
                    max = used[i];
                }
                break;
            }
        }
    }

    // find minimum
    unsigned int min = max;
    for (unsigned int i = 0; i < sizeof(colorList) / sizeof(QColor); i++) {
        if (used[i] < min) {
            min = used[i];
        }
    }

    // return first in list that is least used
    for (unsigned int i = 0; i < sizeof(colorList) / sizeof(QColor); i++) {
        if (used[i] == min) {
            return colorList[i];
        }
    }


    return Qt::blue;
}

/****************************************************************************/

void Section::updateLegend()
{
    QString html = "<html><head><meta http-equiv=\"Content-Type\" "
        "content=\"text/html; charset=utf-8\"></head>"
        "<body style=\"font-size: 8pt\">";

    bool first = true;
    for (QList<Layer *>::const_iterator l = layers.begin();
            l != layers.end(); l++) {
        if (first) {
            first = false;
        } else {
            html += ", ";
        }
        html += "<span style=\"color: " + (*l)->getColor().name() + ";\">";
        html += (*l)->getChannel()->name().c_str();
        html += "</span>";
    }

    html += "</body></html>";

    legend.setHtml(html);
}

/****************************************************************************/
