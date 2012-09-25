/*****************************************************************************
 *
 * $Id$
 *
 * Copyright (C) 2009-2012  Florian Pose <fp@igh-essen.com>
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
#include <math.h>

#include "Scale.h"

using DLS::Scale;

/****************************************************************************/

/** Constructor.
 */
Scale::Scale(QWidget *p):
    parent(p),
    length(0),
    outerLength(0),
    majorStep(0.0),
    minorDiv(2)
{
}

/****************************************************************************/

/** Sets the scale start time (#start).
 *
 * If the value changes, this re-calculates the scale layout.
 */
void Scale::setStart(const COMTime &t)
{
    if (t != start) {
        start = t;
        update();
    }
}

/****************************************************************************/

/** Sets the scale end time (#end).
 *
 * If the value changes, this re-calculates the scale layout.
 */
void Scale::setEnd(const COMTime &t)
{
    if (t != end) {
        end = t;
        update();
    }
}

/****************************************************************************/

/** Sets the scale #length in pixel.
 *
 * If the value changes, this re-calculates the scale layout.
 */
void Scale::setLength(int l)
{
    if (l != length) {
        length = l;
        update();
    }
}

/****************************************************************************/

/** Calculates the scale's layout.
 */
void Scale::update()
{
    double rawMajorStep;
    double range = (end - start).to_dbl_time();

    if (length <= 0 || range <= 0.0) {
        outerLength = 0;
        majorStep = 0.0;
        minorDiv = 2;
        return;
    }

    QFont f = parent->font();
    f.setPointSize(8);
    QFontMetrics fm(f);
    QSize s = fm.size(0, "8888-88-88\n00:00:00");

    rawMajorStep = (s.width() + 6) * range / length;

    if (rawMajorStep > 3600.0 * 24.0) { // days
        double scaledMajor = rawMajorStep / 3600.0 / 24.0;
        int decade = (int) floor(log10(scaledMajor));
        double normMajorStep = scaledMajor / pow(10.0, decade);
        /* 1 <= step < 10 */

        if (normMajorStep > 5.0) {
            normMajorStep = 1.0;
            minorDiv = 5;
            decade++;
        } else if (normMajorStep > 2.0) {
            normMajorStep = 5.0;
            minorDiv = 5;
        } else {
            normMajorStep = 2.0;
            minorDiv = 2;
        }

        majorStep = normMajorStep * pow(10.0, decade) * 3600.0 * 24.0;
        format = "%Y-%m-%d";
    }
    else if (rawMajorStep > 3600.0) { // hours
        double normMajorStep = rawMajorStep / 3600.0;

        if (normMajorStep > 12.0) {
            normMajorStep = 24.0; // 4 * 6 h
            minorDiv = 4;
        } else if (normMajorStep > 6.0) {
            normMajorStep = 12.0; // 4 * 3 h
            minorDiv = 4;
        } else if (normMajorStep > 2.0) {
            normMajorStep = 6.0; // 6 * 1 h
            minorDiv = 6;
        } else {
            normMajorStep = 2.0; // 2 * 1 h
            minorDiv = 2;
        }

        majorStep = normMajorStep * 3600.0;
        format = "%Y-%m-%d\n%H:%M";
    }
    else if (rawMajorStep > 60.0) { // minutes
        double normMajorStep = rawMajorStep / 60.0;

        if (normMajorStep > 30.0) {
            normMajorStep = 60.0; // 6 * 10 min
            minorDiv = 6;
        } else if (normMajorStep > 20.0) {
            normMajorStep = 30; // 3 * 10 min
            minorDiv = 3;
        } else if (normMajorStep > 10.0) {
            normMajorStep = 20.0;
            minorDiv = 2;
        } else if (normMajorStep > 5.0) {
            normMajorStep = 10.0;
            minorDiv = 2;
        } else if (normMajorStep > 2.0) {
            normMajorStep = 5.0;
            minorDiv = 5;
        } else {
            normMajorStep = 2.0;
            minorDiv = 2;
        }
        majorStep = normMajorStep * 60.0;
        format = "%Y-%m-%d\n%H:%M";
    }
    else if (rawMajorStep > 1.0) { // seconds
        double normMajorStep = rawMajorStep;

        if (normMajorStep > 30.0) {
            normMajorStep = 60.0; // 6 * 10 s
            minorDiv = 6;
        } else if (normMajorStep > 20.0) {
            normMajorStep = 30; // 3 * 10 s
            minorDiv = 3;
        } else if (normMajorStep > 10.0) {
            normMajorStep = 20.0;
            minorDiv = 2;
        } else if (normMajorStep > 5.0) {
            normMajorStep = 10.0;
            minorDiv = 2;
        } else if (normMajorStep > 2.0) {
            normMajorStep = 5.0;
            minorDiv = 5;
        } else {
            normMajorStep = 2.0;
            minorDiv = 2;
        }
        majorStep = normMajorStep;
        format = "%Y-%m-%d\n%H:%M:%S";
    }
    else { // sub-second
        int decade = (int) floor(log10(rawMajorStep));
        double normMajorStep =
            rawMajorStep / pow(10.0, decade); // 1 <= step < 10

        if (normMajorStep > 5.0) {
            normMajorStep = 1.0;
            minorDiv = 5;
            decade++;
        } else if (normMajorStep > 2.0) {
            normMajorStep = 5.0;
            minorDiv = 5;
        } else {
            normMajorStep = 2.0;
            minorDiv = 2;
        }

        majorStep = normMajorStep * pow(10.0, decade);
        format = "%Y-%m-%d\n%H:%M:%S";
        subDigits = -decade;
    }

    outerLength = s.height() + 5;
}

/****************************************************************************/

/** Draws the scale into the given QRect with the given QPainter.
 */
void Scale::draw(QPainter &painter, const QRect &rect) const
{
    double value, factor, minorValue;
    QPen pen = painter.pen();
    QRect textRect;
    int l, p, lineOffset;
    unsigned int minorIndex;
    bool drawLabel;
    double range = (end - start).to_dbl_time();

    l = rect.width();

    if (!majorStep || !l || range <= 0.0)
        return;

    factor = l / range;

    textRect.setTop(rect.top() + 2);
    textRect.setWidth((int) (majorStep * factor) - 4);
    textRect.setHeight(rect.height() - 4);

    pen.setStyle(Qt::DashLine);
    QColor minorColor = parent->palette().window().color().dark(110);
    QColor majorColor = parent->palette().window().color().dark(150);
    QColor gridColor;

    value = floor(start.to_dbl_time() / majorStep) * majorStep;
    minorIndex = 0;

    while (value <= end.to_dbl_time()) {
        if (minorIndex) { // minor step, short tick
            minorValue = value + minorIndex * majorStep / minorDiv;
            if (++minorIndex == minorDiv) {
                minorIndex = 0;
                value += majorStep;
            }
            if (minorValue < start.to_dbl_time() ||
                    minorValue >= end.to_dbl_time())
                continue;
            p = (int) ((minorValue - start.to_dbl_time()) * factor);
            lineOffset = outerLength;
            drawLabel = false;
            gridColor = minorColor;
        } else { // major step, long tick
            minorIndex++;
            if (value < start.to_dbl_time() || value >= end.to_dbl_time())
                continue;
            p = (int) ((value - start.to_dbl_time()) * factor);
            lineOffset = 0;
            drawLabel = true;
            gridColor = majorColor;
        }

        pen.setColor(gridColor);
        painter.setPen(pen);
        painter.drawLine(rect.left() + p, rect.top() + lineOffset,
                rect.left() + p, rect.bottom());
        if (drawLabel) {
            QString text = formatValue(value);
            textRect.moveLeft(rect.left() + p + 4);
            QFont f = painter.font();
            f.setPointSize(8);
            QFontMetrics fm(f);
            QSize s = fm.size(0, text);
            if (textRect.left() + s.width() <= rect.right()) {
                painter.setFont(f);
                pen.setColor(Qt::black);
                painter.setPen(pen);
                painter.drawText(textRect, text);
            }
        }
    }
}

/****************************************************************************/

/** Formats a numeric value.
 */
QString Scale::formatValue(double value) const
{
    COMTime t;

    t.from_dbl_time(value);
    return t.format_time(format.toLatin1().constData()).c_str();
}

/****************************************************************************/
