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

#include <algorithm>

#include "lib_channel.hpp"

#include "Section.h"
#include "Layer.h"
#include "Graph.h"

using DLS::Section;
using DLS::Layer;

/****************************************************************************/

/** Constructor.
 */
Layer::Layer(
        Section *section,
        LibDLS::Channel *channel,
        const QColor &c
        ):
    section(section),
    channel(channel),
    color(c),
    scale(1.0),
    offset(0.0),
    minimum(0.0),
    maximum(0.0),
    extremaValid(false)
{
    if (!color.isValid()) {
        color = section->nextColor();
    }

    channel->fetch_chunks();
}

/****************************************************************************/

/** Copy constructor.
 */
Layer::Layer(
        const Layer &o,
        Section *section
        ):
    section(section),
    channel(o.channel),
    name(o.name),
    unit(o.unit),
    color(o.color),
    scale(o.scale),
    offset(o.offset),
    minimum(o.minimum),
    maximum(o.maximum),
    extremaValid(o.extremaValid)
{
    copyDataList(genericData, o.genericData);
    copyDataList(minimumData, o.minimumData);
    copyDataList(maximumData, o.maximumData);
}

/****************************************************************************/

/** Destructor.
 */
Layer::~Layer()
{
    clearDataList(genericData);
    clearDataList(minimumData);
    clearDataList(maximumData);
}

/****************************************************************************/

void Layer::setName(const QString &n)
{
    if (n != name) {
        name = n;
        section->updateLegend();
    }
}

/****************************************************************************/

void Layer::setUnit(const QString &u)
{
    if (u != unit) {
        unit = u;
        section->updateLegend();
    }
}

/****************************************************************************/

void Layer::setColor(QColor c)
{
    if (!c.isValid()) {
        c = section->nextColor();
    }

    if (c != color) {
        color = c;
        section->updateLegend();
    }
}

/****************************************************************************/

void Layer::setScale(double s)
{
    if (s != scale) {
        scale = s;

        bool first = true;
        updateExtrema(genericData, &first);
        updateExtrema(minimumData, &first);
        updateExtrema(maximumData, &first);
        extremaValid = !first;

        section->update();
    }
}

/****************************************************************************/

void Layer::setOffset(double o)
{
    if (o != offset) {
        offset = o;

        bool first = true;
        updateExtrema(genericData, &first);
        updateExtrema(minimumData, &first);
        updateExtrema(maximumData, &first);
        extremaValid = !first;

        section->update();
    }
}

/****************************************************************************/

void Layer::loadData(const COMTime &start, const COMTime &end, int min_values)
{
    clearDataList(genericData);
    clearDataList(minimumData);
    clearDataList(maximumData);

    channel->fetch_data(start, end, min_values, dataCallback, this);

    bool first = true;
    updateExtrema(genericData, &first);
    updateExtrema(minimumData, &first);
    updateExtrema(maximumData, &first);

    extremaValid = !first;
}

/****************************************************************************/

QString Layer::title() const
{
    QString ret;

    if (!name.isEmpty()) {
        ret += name;
    }
    else {
        ret += channel->name().c_str();
    }

    if (!unit.isEmpty()) {
        ret += " [" + unit + "]";
    }

    return ret;
}

/****************************************************************************/

QString Layer::formatValue(double value) const
{
    QString ret;

    ret.setNum(value);

    if (!unit.isEmpty()) {
        if (unit != "°") {
            ret += QChar(0x202f); // narrow no-break space U+202f
        }

        ret += unit;
    }

    return ret;
}

/****************************************************************************/

int Layer::dataCallback(LibDLS::Data *data, void *cb_data)
{
    Layer *l = (Layer *) cb_data;
    l->newData(data);
    return 1; // adopt object
}

/****************************************************************************/

void Layer::newData(LibDLS::Data *data)
{
    switch (data->meta_type()) {
        case DLSMetaGen:
            genericData.push_back(data);
            break;
        case DLSMetaMin:
            minimumData.push_back(data);
            break;
        case DLSMetaMax:
            maximumData.push_back(data);
            break;
        default:
            break;
    }
}

/****************************************************************************/

void Layer::clearDataList(QList<LibDLS::Data *> &list)
{
    for (QList<LibDLS::Data *>::iterator d = list.begin();
            d != list.end(); d++) {
        delete *d;
    }

    list.clear();
}

/****************************************************************************/

void Layer::copyDataList(QList<LibDLS::Data *> &list,
        const QList<LibDLS::Data *> &other)
{
    clearDataList(list);

    for (QList<LibDLS::Data *>::const_iterator d = other.begin();
            d != other.end(); d++) {
        LibDLS::Data *data = new LibDLS::Data(**d);
        list.push_back(data);
    }
}

/****************************************************************************/

void Layer::updateExtrema(const QList<LibDLS::Data *> &list, bool *first)
{
    for (QList<LibDLS::Data *>::const_iterator d = list.begin();
            d != list.end(); d++) {
        double current_min, current_max;

        if (!(*d)->calc_min_max(&current_min, &current_max)) {
            continue;
        }

        current_min = current_min * scale + offset;
        current_max = current_max * scale + offset;

        if (*first) {
            minimum = current_min;
            maximum = current_max;
            *first = false;
        }
        else {
            if (current_min < minimum) {
                minimum = current_min;
            }
            if (current_max > maximum) {
                maximum = current_max;
            }
        }
    }
}

/****************************************************************************/

void Layer::draw(QPainter &painter, const QRect &rect, double xScale,
        double yScale, double min, MeasureData *measure) const
{
    drawGaps(painter, rect, xScale);

    if (genericData.size()) {
        double prev_value = 0.0;
        int prev_xp = 0, prev_yp = 0;
        double prev_xv = 0.0, prev_yv = 0.0;
        bool first_in_chunk = true;

        QPen pen;
        pen.setColor(color);

        painter.save();
        painter.setPen(pen);
        painter.setClipRect(rect, Qt::IntersectClip);

        for (QList<LibDLS::Data *>::const_iterator d = genericData.begin();
                d != genericData.end(); d++) {

            for (unsigned int i = 0; i < (*d)->size(); i++) {
                double value = (*d)->value(i) * scale + offset;
                COMTime dt = (*d)->time(i) - section->getGraph()->getStart();
                double xv = dt.to_dbl_time() * xScale;
                double yv = (value -  min) * yScale;
                int xp, yp;

                if (xv >= 0.0) {
                    xp = (int) (xv + 0.5);
                }
                else {
                    xp = (int) (xv - 0.5);
                }

                if (yv >= 0.0) {
                    yp = (int) (yv + 0.5);
                }
                else {
                    yp = (int) (yv - 0.5);
                }

                if (xp >= 0) {
                    if (first_in_chunk) {
                        QPointF p(rect.left() + xv, rect.bottom() - yv);
                        painter.drawPoint(p);
                    }
                    else {
                        QPointF prev(rect.left() + prev_xv,
                                rect.bottom() - prev_yv);
                        QPointF inter(rect.left() + xv,
                                rect.bottom() - prev_yv);
                        QPointF cur(rect.left() + xv, rect.bottom() - yv);

                        painter.drawLine(prev, inter);
                        painter.drawLine(inter, cur);
                    }

                    if (measure) {
                        if (xp == measure->x) {
                            if (measure->found) {
                                if (value < measure->minimum) {
                                    measure->minimum = value;
                                    measure->minY = yp;
                                }
                                if (value > measure->maximum) {
                                    measure->maximum = value;
                                    measure->maxY = yp;
                                }
                            }
                            else {
                                measure->minimum = value;
                                measure->maximum = value;
                                measure->minY = yp;
                                measure->maxY = yp;
                                measure->found = true;
                            }
                        }
                        else if (xp > measure->x && prev_xp
                                < measure->x && !first_in_chunk) {
                            if (measure->x - prev_xp < xp - measure->x) {
                                measure->minimum = prev_value;
                                measure->maximum = prev_value;
                                measure->minY = prev_yp;
                                measure->maxY = prev_yp;
                            }
                            else {
                                measure->minimum = value;
                                measure->maximum = value;
                                measure->minY = yp;
                                measure->maxY = yp;
                            }

                            measure->found = true;
                        }
                    }

                    if (xp >= rect.width()) {
                        break;
                    }
                }

                if (xp >= rect.width()) {
                    break;
                }

                prev_xv = xv;
                prev_yv = yv;
                prev_xp = xp;
                prev_yp = yp;
                prev_value = value;
                first_in_chunk = false;
            }
        }

        painter.restore();
    }
    else if (minimumData.size() && maximumData.size()) {
        double yv, value;
        int xp, yp, i;
        unsigned int j;

        struct extrema {
            int min;
            int max;
            bool minValid;
            bool maxValid;
        };
        struct extrema *extrema;

        try {
            extrema = new struct extrema[rect.width()];
        }
        catch (...) {
            qWarning() << "ERROR: Failed to allocate drawing memory!";
            return;
        }

        for (i = 0; i < rect.width(); i++) {
            extrema[i].minValid = false;
            extrema[i].maxValid = false;
        }

        QPen pen;
        pen.setColor(color);
        painter.setPen(pen);

        for (QList<LibDLS::Data *>::const_iterator d = minimumData.begin();
                d != minimumData.end(); d++) {

            for (j = 0; j < (*d)->size(); j++) {
                value = (*d)->value(j) * scale + offset;
                COMTime dt = (*d)->time(j) - section->getGraph()->getStart();
                double xv = dt.to_dbl_time() * xScale;
                yv = (value - min) * yScale;

                if (xv >= 0.0) {
                    xp = (int) (xv + 0.5);
                }
                else {
                    xp = (int) (xv - 0.5);
                }
                if (yv >= 0.0) {
                    yp = (int) (yv + 0.5);
                }
                else {
                    yp = (int) (yv - 0.5);
                }

                if (xp >= 0 && xp < rect.width()) {
                    if (!extrema[xp].minValid ||
                            (extrema[xp].minValid && yp < extrema[xp].min)) {
                        extrema[xp].min = yp;
                        extrema[xp].minValid = true;

                        if (measure && xp == measure->x) {
                            if (measure->found) {
                                if (value < measure->minimum) {
                                    measure->minimum = value;
                                    measure->minY = yp;
                                }
                                if (value > measure->maximum) {
                                    measure->maximum = value;
                                    measure->maxY = yp;
                                }
                            }
                            else {
                                measure->minimum = value;
                                measure->maximum = value;
                                measure->minY = yp;
                                measure->maxY = yp;
                                measure->found = true;
                            }
                        }
                    }
                }

                else if (xp >= rect.width()) {
                    break;
                }
            }
        }

        for (QList<LibDLS::Data *>::const_iterator d = maximumData.begin();
                d != maximumData.end(); d++) {

            for (j = 0; j < (*d)->size(); j++) {
                value = (*d)->value(j) * scale + offset;
                COMTime dt = (*d)->time(j) - section->getGraph()->getStart();
                double xv = dt.to_dbl_time() * xScale;
                yv = (value - min) * yScale;

                if (xv >= 0.0) {
                    xp = (int) (xv + 0.5);
                }
                else {
                    xp = (int) (xv - 0.5);
                }
                if (yv >= 0.0) {
                    yp = (int) (yv + 0.5);
                }
                else {
                    yp = (int) (yv - 0.5);
                }

                if (xp >= 0 && xp < rect.width()) {
                    if (!extrema[xp].maxValid ||
                            (extrema[xp].maxValid && yp > extrema[xp].max)) {
                        extrema[xp].max = yp;
                        extrema[xp].maxValid = true;

                        if (measure && xp == measure->x) {
                            if (measure->found) {
                                if (value < measure->minimum) {
                                    measure->minimum = value;
                                    measure->minY = yp;
                                }
                                if (value > measure->maximum) {
                                    measure->maximum = value;
                                    measure->maxY = yp;
                                }
                            }
                            else {
                                measure->minimum = value;
                                measure->maximum = value;
                                measure->minY = yp;
                                measure->maxY = yp;
                                measure->found = true;
                            }
                        }
                    }
                }

                else if (xp >= rect.width()) {
                    break;
                }
            }
        }

        QRect col;
        col.setWidth(1);
        for (i = 0; i < rect.width(); i++) {
            col.moveLeft(rect.left() + i);
            if (extrema[i].minValid && extrema[i].maxValid) {
                if (extrema[i].min >= rect.height() || extrema[i].max < 0) {
                    continue;
                }
                if (extrema[i].min < 0) {
                    extrema[i].min = 0;
                }
                if (extrema[i].max >= rect.height()) {
                    extrema[i].max = rect.height() - 1;
                }
                if (extrema[i].min != extrema[i].max) {
                    col.setTop(rect.bottom() - extrema[i].max);
                    col.setHeight(extrema[i].max - extrema[i].min + 1);
                }
                else {
                    col.setTop(rect.bottom() - extrema[i].max);
                    col.setHeight(1);
                }
                painter.fillRect(col, color);
            }
            else {
                if (extrema[i].minValid && extrema[i].min >= 0 &&
                        extrema[i].min < rect.height()) {
                    col.setTop(rect.bottom() - extrema[i].min);
                    col.setHeight(1);
                    painter.fillRect(col, color);
                }
                if (extrema[i].maxValid && extrema[i].max >= 0 &&
                        extrema[i].max < rect.height()) {
                    col.setTop(rect.bottom() - extrema[i].max);
                    col.setHeight(1);
                    painter.fillRect(col, color);
                }
            }
        }

        delete [] extrema;
    }
}

/****************************************************************************/

bool Layer::range_before(
        const Layer::TimeRange &range1,
        const Layer::TimeRange &range2
        )
{
    return range1.start < range2.start;
}

/****************************************************************************/

void Layer::drawGaps(QPainter &painter, const QRect &rect,
        double xScale) const
{
    double xp, prev_xp;
    vector<TimeRange> ranges, relevant_chunk_ranges;
    COMTime last_end;
    QColor gapColor(255, 255, 220, 127);

    for (list<LibDLS::Chunk>::const_iterator c = channel->chunks().begin();
            c != channel->chunks().end(); c++) {
        TimeRange r;
        r.start = c->start();
        r.end = c->end();
        ranges.push_back(r);
    }

    sort(ranges.begin(), ranges.end(), range_before);

    // check if chunks overlap
    last_end.set_null();
    for (vector<TimeRange>::iterator range = ranges.begin();
         range != ranges.end();
         range++) {
        if (range->start <= last_end) {
            qWarning() << "WARNING: Chunks overlapping in channel"
                 << channel->name().c_str();
            return;
        }
        last_end = range->end;
    }

    for (vector<TimeRange>::iterator range = ranges.begin();
         range != ranges.end(); range++) {
        if (range->end < section->getGraph()->getStart()) {
            continue;
        }
        if (range->start > section->getGraph()->getEnd()) {
            break;
        }
        relevant_chunk_ranges.push_back(*range);
    }

    prev_xp = -1;

    for (vector<TimeRange>::iterator range = relevant_chunk_ranges.begin();
         range != relevant_chunk_ranges.end(); range++) {
        xp = (range->start -
                section->getGraph()->getStart()).to_dbl_time() * xScale;

        if (xp > prev_xp + 1) {
            QRect gapRect(rect.left() + (int) (prev_xp + 1.5),
                     rect.top(),
                     (int) (xp - prev_xp - 1),
                     rect.height());
            painter.fillRect(gapRect, gapColor);
        }

        prev_xp = (range->end -
                section->getGraph()->getStart()).to_dbl_time() * xScale;
    }

    if (rect.width() > prev_xp + 1) {
        QRect gapRect(rect.left() + (int) (prev_xp + 1.5),
                rect.top(),
                (int) (rect.width() - prev_xp - 1),
                rect.height());
        painter.fillRect(gapRect, gapColor);
    }
}

/****************************************************************************/
