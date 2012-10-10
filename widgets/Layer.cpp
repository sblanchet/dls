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

/** Destructor.
 */
Layer::~Layer()
{
    clearDataList(genericData);
    clearDataList(minimumData);
    clearDataList(maximumData);
}

/****************************************************************************/

void Layer::setColor(const QColor &c)
{
    color = c;

    if (!color.isValid()) {
        color = section->nextColor();
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

void Layer::updateExtrema(const QList<LibDLS::Data *> &list, bool *first)
{
    for (QList<LibDLS::Data *>::const_iterator d = list.begin();
            d != list.end(); d++) {
        double current_min, current_max;

        if (!(*d)->calc_min_max(&current_min, &current_max)) {
            continue;
        }

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
        double old_value = 0.0;
        int old_xp = 0, old_yp = 0, dx, dy;
        bool first_in_chunk = true;

        QPen pen;
        pen.setColor(color);
        painter.setPen(pen);
        painter.setClipRect(rect);

        for (QList<LibDLS::Data *>::const_iterator d = genericData.begin();
                d != genericData.end(); d++) {

            for (unsigned int i = 0; i < (*d)->size(); i++) {
                double value = (*d)->value(i);
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
                        painter.drawPoint(rect.left() + xp,
                                rect.bottom() - yp);
                    }
                    else {
                        dx = xp - old_xp;
                        dy = yp - old_yp;

                        if ((float) dx * (float) dx
                                + (float) dy * (float) dy > 0) {
                            painter.drawLine(rect.left() + old_xp,
                                    rect.bottom() - old_yp,
                                    rect.left() + xp, rect.bottom() - yp);
                        }
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
                        else if (xp > measure->x && old_xp
                                < measure->x && !first_in_chunk) {
                            if (measure->x - old_xp < xp - measure->x) {
                                measure->minimum = old_value;
                                measure->maximum = old_value;
                                measure->minY = old_yp;
                                measure->maxY = old_yp;
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

                old_xp = xp;
                old_yp = yp;
                old_value = value;
                first_in_chunk = false;
            }
        }

        painter.setClipping(false);
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
                value = (*d)->value(j);
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
                value = (*d)->value(j);
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

        for (i = 0; i < rect.width(); i++) {
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
                    painter.drawLine(rect.left() + i,
                            rect.bottom() - extrema[i].min,
                            rect.left() + i, rect.bottom() - extrema[i].max);
                }
                else {
                    painter.drawPoint(rect.left() + i,
                            rect.bottom() - extrema[i].min);
                }
            }
            else {
                if (extrema[i].minValid && extrema[i].min >= 0 &&
                        extrema[i].min < rect.height()) {
                    painter.drawPoint(rect.left() + i,
                            rect.bottom() - extrema[i].min);
                }
                if (extrema[i].maxValid && extrema[i].max >= 0 &&
                        extrema[i].max < rect.height()) {
                    painter.drawPoint(rect.left() + i,
                            rect.bottom() - extrema[i].max);
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
    double xp, old_xp;
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

    old_xp = -1;

    for (vector<TimeRange>::iterator range = relevant_chunk_ranges.begin();
         range != relevant_chunk_ranges.end(); range++) {
        xp = (range->start -
                section->getGraph()->getStart()).to_dbl_time() * xScale;

        if (xp > old_xp + 1) {
            QRect gapRect(rect.left() + (int) (old_xp + 1.5),
                     rect.top(),
                     (int) (xp - old_xp - 1),
                     rect.height());
            painter.fillRect(gapRect, gapColor);
        }

        old_xp = (range->end -
                section->getGraph()->getStart()).to_dbl_time() * xScale;
    }

    if (rect.width() > old_xp + 1) {
        QRect gapRect(rect.left() + (int) (old_xp + 1.5),
                rect.top(),
                (int) (rect.width() - old_xp - 1),
                rect.height());
        painter.fillRect(gapRect, gapColor);
    }
}

/****************************************************************************/
