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
    autoScale(true),
    scaleMin(0.0),
    scaleMax(100.0),
    height(100)
{
}

/****************************************************************************/

/** Copy constructor.
 */
Section::Section(
        const Section &o
        ):
    graph(o.graph),
    autoScale(o.autoScale),
    scaleMin(o.scaleMin),
    scaleMax(o.scaleMax),
    height(o.height)
{
    for (QList<Layer *>::const_iterator l = o.layers.begin();
            l != o.layers.end(); l++) {
        Layer *newLayer = new Layer(**l, this);
        layers.append(newLayer);
    }

    updateLegend();
}

/****************************************************************************/

/** Destructor.
 */
Section::~Section()
{
}

/****************************************************************************/

void Section::setAutoScale(bool a)
{
    if (a != autoScale) {
        autoScale = a;
        graph->update();
    }
}

/****************************************************************************/

void Section::setScaleMinimum(double min)
{
    if (min != scaleMin) {
        scaleMin = min;
        graph->update();
    }
}

/****************************************************************************/

void Section::setScaleMaximum(double max)
{
    if (max != scaleMax) {
        scaleMax = max;
        graph->update();
    }
}

/****************************************************************************/

void Section::setHeight(int h)
{
    if (h < 0) {
        h = 0;
    }

    if (h != height) {
        height = h;
        graph->update();
    }
}

/****************************************************************************/

void Section::resize(int width)
{
    legend.setPageSize(QSize(width, height));
}

/****************************************************************************/

void spreadGroup(QList<Layer::MeasureData> &list,
        unsigned int group, int labelHeight)
{
    int sumY = 0;
    unsigned int count = 0;

    for (QList<Layer::MeasureData>::const_iterator measure = list.begin();
            measure != list.end(); measure++) {
        if (measure->group == group) {
            sumY += measure->meanY;
            count++;
        }
    }

    if (!count) {
        return;
    }

    int off = sumY / count - (labelHeight * (count - 1) / 2);
    unsigned int index = 0;

    for (QList<Layer::MeasureData>::iterator measure = list.begin();
            measure != list.end(); measure++) {
        if (measure->group == group) {
            measure->movedY = off + labelHeight * index++;
        }
    }
}

/****************************************************************************/

void Section::draw(QPainter &painter, const QRect &rect, int measureX)
{
    QRect legendRect(rect);
    legendRect.setHeight(legend.size().height());
    QRect dataRect(rect);
    dataRect.setTop(legendRect.bottom() + 1);

    if (legendRect.isValid()) {
        painter.fillRect(legendRect, graph->palette().window());

        painter.save();
        painter.translate(rect.topLeft());
        legend.drawContents(&painter);
        painter.restore();
    }

    dataRect.adjust(0, Margin, 0, -Margin);

    if (!dataRect.isValid() || graph->getStart() >= graph->getEnd()) {
        return;
    }

    double minimum = 0.0, maximum = 0.0;

    if (autoScale) {
        bool valid = false;
        for (QList<Layer *>::const_iterator l = layers.begin();
                l != layers.end(); l++) {
            if (!(*l)->getExtremaValid()) {
                continue;
            }

            double min = (*l)->getMinimum();
            double max = (*l)->getMaximum();

            if (valid) {
                if (min < minimum) {
                    minimum = min;
                }
                if (max > maximum) {
                    maximum = max;
                }
            } else {
                minimum = min;
                maximum = max;
                valid = true;
            }
        }
    }
    else {
        minimum = scaleMin;
        maximum = scaleMax;
    }

    if (minimum > maximum) {
        return;
    }

    double xScale = (dataRect.width() - 1) /
        (graph->getEnd() - graph->getStart()).to_dbl_time();

    double yScale;
    if (minimum < maximum) {
        yScale = (dataRect.height() - 1) / (maximum - minimum);
    }
    else {
        yScale = 0.0;
    }

    Layer::MeasureData measureData;
    measureData.x = measureX;
    QString measureStr;
    QList<Layer::MeasureData> measureList;
    QFont font;
    font.setPointSize(8);
    QFontMetrics fm(font);
    unsigned int group = 1;
    int totalLabelHeight = 0;

    // draw data and capture measuring intersections
    for (QList<Layer *>::const_iterator l = layers.begin();
            l != layers.end(); l++) {
        Layer::MeasureData *measure;

        if (measureX > -1) {
            measureData.found = false;
            measure = &measureData;
        }
        else {
            measure = NULL;
        }

        (*l)->draw(painter, dataRect, xScale, yScale, minimum, measure);

        if (measure && measureData.found &&
                totalLabelHeight + fm.height() <= dataRect.height()) {
            measureData.layer = *l;
            measureData.meanY = (measureData.maxY + measureData.minY) / 2;
            measureData.movedY = measureData.meanY;
            measureData.group = group++;
            measureList.append(measureData);
            totalLabelHeight += fm.height();
        }
    }

    // sort labels by minimum value
    qStableSort(measureList.begin(), measureList.end());

    // eliminate overlaps
    bool foundOverlaps;
    do {
        foundOverlaps = false;
        int lastY = -1, lastGroup = 0;

        for (QList<Layer::MeasureData>::iterator measure =
                measureList.begin();
                measure != measureList.end(); measure++) {
            if (lastY >= 0) {
                if (measure->movedY - lastY < fm.height()) {
                    foundOverlaps = true;
                    measure->group = lastGroup; // merge groups
                    spreadGroup(measureList, measure->group, fm.height());
                }
            }
            lastY = measure->movedY;
            lastGroup = measure->group;
        }
    }
    while (foundOverlaps);

    // push out-of-range labels into the drawing rect from bottom side
    if (!measureList.empty()) {
        QList<Layer::MeasureData>::iterator measure = measureList.begin();
        int bottom = measure->movedY - fm.height() / 2;
        if (bottom < 0) {
            measure->movedY -= bottom;
            int lastY = measure->movedY;
            measure++;
            for (; measure != measureList.end(); measure++) {
                if (measure->movedY - lastY >= fm.height()) {
                    break;
                }
                measure->movedY = lastY + fm.height();
                lastY = measure->movedY;
            }
        }
    }

    // push out-of-range labels into the drawing rect from top side
    if (!measureList.empty()) {
        QList<Layer::MeasureData>::iterator measure = measureList.end();
        measure--;
        int over = measure->movedY + fm.height() / 2 - dataRect.height();
        if (over > 0) {
            measure->movedY -= over;
            int lastY = measure->movedY;

            while (measure != measureList.begin()) {
                measure--;

                if (lastY - measure->movedY >= fm.height()) {
                    break;
                }
                measure->movedY = lastY - fm.height();
                lastY = measure->movedY;
            }
        }
    }

    // draw measuring labels
    for (QList<Layer::MeasureData>::const_iterator measure =
            measureList.begin();
            measure != measureList.end(); measure++) {
        QString label;
        QPen pen;

        pen.setColor(measure->layer->getColor());
        painter.setPen(pen);
        painter.setFont(font);

        if (measure->minimum != measure->maximum) {
            label = QString().fromUtf8("%1 – %2") // unicode en-dash!
                .arg(measure->minimum).arg(measure->maximum);
        }
        else {
            label = QString("%1").arg(measure->minimum);
        }

        QRect textRect(dataRect);
        textRect.setLeft(dataRect.left() + measure->x + 10);
        textRect.moveTop(
                dataRect.bottom() + 1 - measure->movedY - fm.height() / 2);
        textRect.setHeight(fm.height());
        int flags = Qt::AlignLeft | Qt::AlignVCenter;
        QRect rect = fm.boundingRect(textRect, flags, label);

        // try drawing left from measure line
        if (rect.width() > textRect.width() &&
                measure->x - 10 > textRect.width()) {
            textRect.setLeft(dataRect.left());
            textRect.setWidth(measure->x - 10);
            rect.moveRight(textRect.right());
            flags = Qt::AlignRight | Qt::AlignVCenter;
        }

        painter.save();
        painter.setClipRect(dataRect, Qt::IntersectClip);
        QRect backRect = rect.adjusted(-2, 0, 2, 0);
        painter.fillRect(backRect, Qt::white);
        painter.drawText(textRect, flags, label);
        QPen linePen;
        painter.setPen(linePen);
        painter.drawLine(backRect.bottomLeft(), backRect.bottomRight());
        painter.drawLine(backRect.bottomRight(), backRect.topRight());
        painter.restore();
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
