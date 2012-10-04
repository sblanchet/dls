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
        LibDLS::Channel *channel
        ):
    section(section),
    channel(channel),
    minimum(0.0),
    maximum(0.0)
{
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

void Layer::draw(QPainter &painter, const QRect &rect) const
{
    int dataWidth = rect.width() - 2 * Margin;
    int dataHeight = section->height - 2 * Margin;

    if (dataWidth <= 0) {
        return;
    }

    double xScale = dataWidth /
        (section->graph->getEnd() - section->graph->getStart()).to_dbl_time();

    drawGaps(painter, rect, dataWidth, xScale);

    if (minimum >= maximum || dataHeight <= 0) {
        return;
    }

    double yScale = (dataHeight - 1) / (maximum - minimum);
    int yOff = rect.top() + section->height - 1 - Margin;

    if (genericData.size()) {
        double old_value = 0.0;
        int old_xp = 0, old_yp = 0, dx, dy;
        bool first_in_chunk = true;

        QPen pen;
        pen.setColor(Qt::blue);
        painter.setPen(pen);

        for (QList<LibDLS::Data *>::const_iterator d = genericData.begin();
                d != genericData.end(); d++) {

            for (unsigned int i = 0; i < (*d)->size(); i++) {
                double value = (*d)->value(i);
                COMTime dt = (*d)->time(i) - section->graph->getStart();
                double xv = dt.to_dbl_time() * xScale;
                double yv = (value - minimum) * yScale;
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
                        painter.drawPoint(rect.left() + Margin + xp,
                                yOff - yp);
                    }
                    else {
                        dx = xp - old_xp;
                        dy = yp - old_yp;

                        if ((float) dx * (float) dx
                                + (float) dy * (float) dy > 0) {
                            painter.drawLine(rect.left() + Margin + old_xp,
                                    yOff - old_yp,
                                    rect.left() + Margin + xp, yOff - yp);
                        }
                    }

                    if (xp >= dataWidth) {
                        break;
                    }
                }

                if (xp >= dataWidth) {
                    break;
                }

                old_xp = xp;
                old_yp = yp;
                old_value = value;
                first_in_chunk = false;
            }
        }
    }
    else if (minimumData.size() && maximumData.size()) {
        double yv, value;
        int xp, yp, i;
        unsigned int j;
        int *min_px, *max_px;

        try {
            min_px = new int[dataWidth];
        }
        catch (...) {
            qWarning() << "ERROR: Failed to allocate drawing memory!";
            return;
        }

        try {
            max_px = new int[dataWidth];
        }
        catch (...) {
            qWarning() << "ERROR: Failed to allocate drawing memory!";
            delete [] min_px;
            return;
        }

        for (i = 0; i < dataWidth; i++) {
            min_px[i] = -1;
            max_px[i] = -1;
        }

        QPen pen;
        pen.setColor(Qt::darkGreen);
        painter.setPen(pen);

        for (QList<LibDLS::Data *>::const_iterator d = minimumData.begin();
                d != minimumData.end(); d++) {

            for (j = 0; j < (*d)->size(); j++) {
                value = (*d)->value(j);
                COMTime dt = (*d)->time(j) - section->graph->getStart();
                double xv = dt.to_dbl_time() * xScale;
                yv = (value - minimum) * yScale;

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

                if (xp >= 0 && xp < dataWidth) {
                    if (min_px[xp] == -1
                            || (min_px[xp] != -1 && yp < min_px[xp])) {
                        min_px[xp] = yp;
                    }
                }

                else if (xp >= dataWidth) {
                    break;
                }
            }
        }

        for (QList<LibDLS::Data *>::const_iterator d = maximumData.begin();
                d != maximumData.end(); d++) {

            for (j = 0; j < (*d)->size(); j++) {
                value = (*d)->value(j);
                COMTime dt = (*d)->time(j) - section->graph->getStart();
                double xv = dt.to_dbl_time() * xScale;
                yv = (value - minimum) * yScale;

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

                if (xp >= 0 && xp < dataWidth) {
                    if (max_px[xp] == -1
                            || (max_px[xp] != -1 && yp > max_px[xp])) {
                        max_px[xp] = yp;
                    }
                }

                else if (xp >= dataWidth) {
                    break;
                }
            }
        }

        for (i = 0; i < dataWidth; i++) {
            if (min_px[i] != -1 && max_px[i] != -1) {
                if (min_px[i] != max_px[i]) {
                    painter.drawLine(rect.left() + Margin + i,
                            yOff - min_px[i],
                            rect.left() + Margin + i, yOff - max_px[i]);
                }
                else {
                    painter.drawPoint(rect.left() + Margin + i,
                            yOff - min_px[i]);
                }
            }
            else {
                if (min_px[i] != -1) {
                    painter.drawPoint(rect.left() + Margin + i,
                            yOff - min_px[i]);
                }
                if (max_px[i] != -1) {
                    painter.drawPoint(rect.left() + Margin + i,
                            yOff - max_px[i]);
                }
            }
        }

        delete [] min_px;
        delete [] max_px;
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

void Layer::drawGaps(QPainter &painter, const QRect &rect, int dataWidth,
        double xScale) const
{
    double xp, old_xp;
    int yOff, dataHeight;
    vector<TimeRange> ranges, relevant_chunk_ranges;
    COMTime last_end;
    QColor gapColor(255, 255, 220, 127);

    yOff = rect.top() + Margin;
    dataHeight = section->height - 2 * Margin;

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
            cerr << "WARNING: Chunks overlapping in channel \""
                 << channel->name() << "\"!" << endl;
            return;
        }
        last_end = range->end;
    }

    for (vector<TimeRange>::iterator range = ranges.begin();
         range != ranges.end(); range++) {
        if (range->end < section->graph->getStart()) {
            continue;
        }
        if (range->start > section->graph->getEnd()) {
            break;
        }
        relevant_chunk_ranges.push_back(*range);
    }

    old_xp = -1;

    for (vector<TimeRange>::iterator range = relevant_chunk_ranges.begin();
         range != relevant_chunk_ranges.end(); range++) {
        xp = (range->start -
                section->graph->getStart()).to_dbl_time() * xScale;

        if (xp > old_xp + 1) {
            QRect f(rect.left() + Margin + (int) (old_xp + 1.5),
                     yOff,
                     (int) (xp - old_xp - 1),
                     dataHeight);
            painter.fillRect(f, gapColor);
        }

        old_xp = (range->end -
                section->graph->getStart()).to_dbl_time() * xScale;
    }

    if (dataWidth > old_xp + 1) {
        QRect f(rect.left() + Margin + (int) (old_xp + 1.5),
                yOff,
                (int) (dataWidth - old_xp - 1),
                dataHeight);
        painter.fillRect(f, gapColor);
    }
}

/****************************************************************************/
