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

//#define DEBUG

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

/** Sets the scale start and end time.
 *
 * If the values change, this re-calculates the scale layout.
 */
void Scale::setRange(const COMTime &t1, const COMTime &t2)
{
    bool changed;

    if (t1 < t2) {
        changed = start != t1 || end != t2;
        start = t1;
        end = t2;
    }
    else if (t1 > t2) {
        changed = start != t2 || end != t1;
        start = t2;
        end = t1;
    }
    else {
        changed = start != t1 || end != t1 + (uint64_t) 1;
        start = t1;
        end = t1 + (uint64_t) 1;
    }

    if (changed) {
#ifdef DEBUG
        qDebug() << start.to_real_time().c_str()
            << end.to_real_time().c_str();
#endif
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
    double ticPeriod;
    double range = (end - start).to_dbl_time();

    if (length <= 0 || range <= 0.0) {
        tics = Time;
        format = "";
        subDigits = 0;
        outerLength = 0;
        majorStep = 0.0;
        minorDiv = 2;
        return;
    }

    QFont f = parent->font();
    f.setPointSize(8);
    QFontMetrics fm(f);
    QSize s;

    s = fm.size(0, "88. 88. 8888\n88:88:88\n888.888 ms");
    ticPeriod = range * (s.width() + 6) / length;

    if (ticPeriod < 0.5) { // sub-second
#ifdef DEBUG
        qDebug() << "sub-second";
#endif
        int decade = (int) floor(log10(ticPeriod));
        if (decade < -6) {
            decade = -6;
        }
        double normMajorStep =
            ticPeriod / pow(10.0, decade); // 1 <= step < 10

        if (normMajorStep > 5.0) {
            normMajorStep = 1.0;
            minorDiv = 5;
            decade++;
        } else if (normMajorStep > 2.0) {
            normMajorStep = 5.0;
            minorDiv = 5;
        } else if (normMajorStep > 1.0) {
            normMajorStep = 2.0;
            minorDiv = 2;
        } else {
            normMajorStep = 1.0;
            minorDiv = 1;
        }

        tics = Time;
        majorStep = normMajorStep * pow(10.0, decade);
        format = "%x\n%H:%M:%S";
        subDigits = -decade;
        outerLength = s.height() + 5;
        return;
    }

    s = fm.size(0, "88. 88. 8888\n88:88:88");
    ticPeriod = range * (s.width() + 6) / length;

    if (ticPeriod < 30.0) { // seconds
#ifdef DEBUG
        qDebug() << "second";
#endif
        if (ticPeriod > 30.0) {
            ticPeriod = 60.0; // 6 * 10 s
            minorDiv = 6;
        } else if (ticPeriod > 20.0) {
            ticPeriod = 30; // 3 * 10 s
            minorDiv = 3;
        } else if (ticPeriod > 10.0) {
            ticPeriod = 20.0;
            minorDiv = 2;
        } else if (ticPeriod > 5.0) {
            ticPeriod = 10.0;
            minorDiv = 2;
        } else if (ticPeriod > 2.0) {
            ticPeriod = 5.0;
            minorDiv = 5;
        } else if (ticPeriod > 1.0) {
            ticPeriod = 2.0;
            minorDiv = 4;
        } else {
            ticPeriod = 1.0;
            minorDiv = 4;
        }

        tics = Time;
        majorStep = ticPeriod;
        format = "%x\n%H:%M:%S";
        subDigits = 0;
        outerLength = s.height() + 5;
        return;
    }

    s = fm.size(0, "8888-88-88\n88:88");
    ticPeriod = range * (s.width() + 6) / length;

    if (ticPeriod < 3600.0) { // minutes
#ifdef DEBUG
        qDebug() << "minutes";
#endif
        double minutes = ticPeriod / 60.0;

        if (minutes > 30.0) {
            minutes = 60.0; // 6 * 10 min
            minorDiv = 6;
        } else if (minutes > 20.0) {
            minutes = 30; // 3 * 10 min
            minorDiv = 6;
        } else if (minutes > 10.0) {
            minutes = 20.0;
            minorDiv = 4;
        } else if (minutes > 5.0) {
            minutes = 10.0;
            minorDiv = 5;
        } else if (minutes > 2.0) {
            minutes = 5.0;
            minorDiv = 5;
        } else {
            minutes = 2.0;
            minorDiv = 4;
        }

        tics = Time;
        majorStep = minutes * 60.0;
        format = "%x\n%H:%M";
        subDigits = 0;
        outerLength = s.height() + 5;
        return;
    }

    if (ticPeriod < 3600.0 * 12.0) { // hours
        double hours = ticPeriod / 3600.0;

        if (hours > 12.0) {
            majorStep = 24.0; // 4 * 6 h
            minorDiv = 4;
        } else if (hours > 6.0) {
            majorStep = 12.0; // 4 * 3 h
            minorDiv = 4;
        } else if (hours > 3.0) {
            majorStep = 6.0; // 6 * 1 h
            minorDiv = 6;
        } else if (hours > 2.0) {
            majorStep = 3.0; // 6 * 1 h
            minorDiv = 3;
        } else {
            majorStep = 2.0; // 2 * 1 h
            minorDiv = 4;
        }

        tics = Hours;
        format = "%x\n%H:%M";
        subDigits = 0;
        outerLength = s.height() + 5;
#ifdef DEBUG
        qDebug() << "hours" << hours << majorStep << minorDiv;
#endif
        return;
    }

    s = fm.size(0, "Sep. 8888\nSo. XX");
    ticPeriod = range * (s.width() + 6) / length;

    if (ticPeriod < 3600.0 * 24.0 * 14.0) { // days
        double days = ticPeriod / 3600.0 / 24.0;

        if (days > 7.0) {
            majorStep = 14.0;
            minorDiv = 2;
        } else if (days > 2.0) {
            majorStep = 7.0;
            minorDiv = 7;
        } else if (days > 1.0) {
            majorStep = 2.0;
            minorDiv = 2;
        } else {
            majorStep = 1.0;
            minorDiv = 1;
        }

        tics = Days;
        format = "%b. %Y\n%d (%a.)";
        subDigits = 0;
        outerLength = s.height() + 5;
#ifdef DEBUG
        qDebug() << "days" << days << majorStep << minorDiv;
#endif
        return;
    }

    s = fm.size(0, "September");
    ticPeriod = range * (s.width() + 6) / length;

    if (ticPeriod < 3600.0 * 24.0 * 366.0) { // months
        double months = ticPeriod / 3600.0 / 24.0 / 28.0;
        QString sample;

        if (months < 1.0) {
            months = 1.0;
        }

        if (months > 6.0) {
            majorStep = 12.0;
            format = "%Y";
            sample = "8888";
            minorDiv = 4;
        } else if (months > 3.0) {
            majorStep = 6.0;
            format = "%Y\n%B";
            sample = "8888\nSeptember";
            minorDiv = 2;
        } else if (months > 1.0) {
            majorStep = 3.0;
            format = "%Y\n%B";
            sample = "8888\nSeptember";
            minorDiv = 3;
        } else {
            majorStep = 1.0;
            format = "%Y\n%B";
            sample = "8888\nSeptember";
            minorDiv = 1;
        }

        tics = Months;
        subDigits = 0;
        s = fm.size(0, sample);
        outerLength = s.height() + 5;
#ifdef DEBUG
        qDebug() << "months" << months << majorStep << minorDiv;
#endif
        return;
    }

    s = fm.size(0, "8888");
    ticPeriod = range * (s.width() + 6) / length;

    { // years
        double years = ticPeriod / 3600.0 / 24.0 / 366.0;
        if (years < 1.0) {
            years = 1.0;
        }
        int decade = (int) floor(log10(years));
        double normMajorStep = years / pow(10.0, decade);

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

        tics = Years;
        majorStep = normMajorStep * pow(10.0, decade);
        format = "%Y";
        subDigits = 0;
        outerLength = s.height() + 5;
#ifdef DEBUG
        qDebug() << "years" << years << majorStep << minorDiv;
#endif
        return;
    }
}

/****************************************************************************/

/** Draws the scale into the given QRect with the given QPainter.
 */
void Scale::draw(QPainter &painter, const QRect &rect) const
{
    double scale, range = (end - start).to_dbl_time();
    QString label;

    if (!majorStep || rect.width() <= 0 || range <= 0.0)
        return;

    scale = rect.width() / range;

    switch (tics) {
        case Time: {
            COMTime t, step;
            step.from_dbl_time(majorStep);
            t.from_dbl_time(
                    floor(start.to_dbl_time() / majorStep) * majorStep);

            while (t < end) {
                if (t >= start) {
                    drawMajor(painter, rect, scale, t, t + step, label);
                }

                for (unsigned int i = 1; i < minorDiv; i++) {
                    COMTime minor;
                    minor.from_dbl_time(
                            t.to_dbl_time() + i * majorStep / minorDiv);
                    if (minor >= start && minor < end) {
                        drawMinor(painter, rect, scale, minor);
                    }
                }

                t += step;
            }
        }
        break;

        case Hours: {
            int y = start.year(), m = start.month(), d = start.day(),
                h = start.hour();
#ifdef DEBUG
            qDebug() << "hours" << start.to_real_time().c_str();
#endif
            h = floor(h / majorStep) * majorStep;
#ifdef DEBUG
            qDebug() << y << m << d << h;
#endif
            COMTime t, step;
            t.set_date(y, m, d, h);
            step.from_dbl_time(majorStep * 3600.0);

            while (t < end) {
                if (t >= start) {
                    drawMajor(painter, rect, scale, t, t + step, label);
                }

                for (unsigned int i = 1; i < minorDiv; i++) {
                    COMTime minor;
                    minor.from_dbl_time( t.to_dbl_time() +
                            i * majorStep * 3600.0 / minorDiv);
                    if (minor >= start && minor < end) {
                        drawMinor(painter, rect, scale, minor);
                    }
                }

                t += step;
            }
        }
        break;

        case Days: {
            int y = start.year(), m = start.month(), d = start.day();
#ifdef DEBUG
            qDebug() << "days" << start.to_real_time().c_str();
#endif
            d = floor((d - 1) / majorStep) * majorStep + 1;
#ifdef DEBUG
            qDebug() << y << m << d;
#endif
            COMTime t, next;
            t.set_date(y, m, d);

            while (t < end) {
                int days = t.month_days();
                int my = y, mm = m, md = d;

                for (int i = 0; i < majorStep; i++) {
                    d++;
                    if (d > days) {
                        d = 1;
                        m++;
                        if (m > 12) {
                            m = 1;
                            y++;
                        }
                        break;
                    }
                }
                if (days - d + 1 < majorStep) {
                    d = 1;
                    m++;
                    if (m > 12) {
                        m = 1;
                        y++;
                    }
                }
                next.set_date(y, m, d);

                if (t >= start) {
                    drawMajor(painter, rect, scale, t, next, label);
                }

                while (1) {
                    COMTime minor;
                    int minorStep = majorStep / minorDiv;

                    while (minorStep--) {
                        md++;
                        if (md > days) {
                            md = 1;
                            mm++;
                            if (mm > 12) {
                                mm = 1;
                                my++;
                            }
                        }
                    }
                    minor.set_date(my, mm, md);
                    if (minor >= next || minor >= end) {
                        break;
                    }

                    if (minor >= start) {
                        drawMinor(painter, rect, scale, minor);
                    }
                }

                t = next;
            }
        }
        break;

        case Months: {
            int y = start.year(), m = start.month();
#ifdef DEBUG
            qDebug() << start.to_real_time().c_str();
#endif
            m = floor((m - 1) / majorStep) * majorStep + 1;
#ifdef DEBUG
            qDebug() << y << m;
#endif
            COMTime t;
            t.set_date(y, m);

            while (t < end) {
                if (t >= start) {
                    COMTime next;
                    int ny = y, nm = m;
                    for (int i = 0; i < majorStep; i++) {
                        nm++;
                        if (nm > 12) {
                            nm = 1;
                            ny++;
                        }
                    }
                    next.set_date(ny, nm);
                    drawMajor(painter, rect, scale, t, next, label);
                }

                for (unsigned int i = 1; i < minorDiv; i++) {
                    COMTime minor;
                    int my = y;
                    int md = i * majorStep / minorDiv;
                    int mm = m;
                    while (md--) {
                        mm++;
                        if (mm > 12) {
                            mm = 1;
                            my++;
                        }
                    }
                    minor.set_date(my, mm);
                    if (minor >= start && minor < end) {
                        drawMinor(painter, rect, scale, minor);
                    }
                }

                // next major
                for (int i = 0; i < majorStep; i++) {
                    m++;
                    if (m > 12) {
                        m = 1;
                        y++;
                    }
                }
                t.set_date(y, m);
            }
        }
        break;

        case Years: {
            int y = start.year();
            y = floor(y / majorStep) * majorStep;
            COMTime t;
            t.set_date(y);

            while (t < end) {
                if (t >= start) {
                    COMTime next;
                    next.set_date(y + majorStep);
                    drawMajor(painter, rect, scale, t, next, label);
                }

                for (unsigned int i = 1; i < minorDiv; i++) {
                    COMTime minor;
                    minor.set_date(y + i * majorStep / minorDiv);
                    if (minor >= start && minor < end) {
                        drawMinor(painter, rect, scale, minor);
                    }
                }

                y += majorStep;
                t.set_date(y);
            }
        }
        break;
    }
}

/****************************************************************************/

/** Formats a numeric value.
 */
QString Scale::formatValue(const COMTime &t, QString &prevLabel) const
{
    QString label;

    label = QString::fromLocal8Bit(
            t.format_time(format.toLatin1().constData()).c_str());

    if (prevLabel.isEmpty()) {
        prevLabel = label;
    }
    else {
        QStringList newLines = label.split("\n");
        QStringList prevLines = prevLabel.split("\n");

        if (newLines.size() == prevLines.size()) {
            for (int i = 0; i < newLines.size(); i++) {
                if (newLines[i] == prevLines[i]) {
                    newLines[i] = "";
                }
                else {
                    prevLines[i] = newLines[i];
                }
            }
            label = newLines.join("\n");
            prevLabel = prevLines.join("\n");
        } else {
            prevLabel = label;
        }
    }

    if (subDigits > 0) {
        int64_t us = t.to_int64() % 1000000;
        double ms = us / 1e3;

        int prec = subDigits - 3;
        if (prec < 0) {
            prec = 0;
        }

        QString fmt = "\n%0." + QString().setNum(prec) + "lf ms";
        label += QString().sprintf(fmt.toLatin1().constData(), ms);
    }

    return label;
}

/****************************************************************************/

/** Draws a major tick with a label.
 */
void Scale::drawMajor(
        QPainter &painter,
        const QRect &rect,
        double scale,
        const COMTime &t,
        const COMTime &n,
        QString &prevLabel
        ) const
{
    QPen pen = painter.pen();
    QRect textRect;

    pen.setColor(parent->palette().window().color().dark(150));
    pen.setStyle(Qt::DashLine);
    painter.setPen(pen);

    int p = (int) ((t - start).to_dbl_time() * scale + 0.5);
    int pn = (int) ((n - start).to_dbl_time() * scale + 0.5);

    painter.drawLine(rect.left() + p, rect.top(),
            rect.left() + p, rect.bottom());

    QString text = formatValue(t, prevLabel);

    textRect.setTop(rect.top() + 2);
    textRect.setHeight(rect.height() - 4);
    textRect.setLeft(rect.left() + p + 4);
    textRect.setRight(rect.left() + pn - 2);

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

/****************************************************************************/

/** Draws a minor tick.
 */
void Scale::drawMinor(
        QPainter &painter,
        const QRect &rect,
        double scale,
        const COMTime &t
        ) const
{
    QPen pen = painter.pen();

    pen.setColor(parent->palette().window().color().dark(110));
    pen.setStyle(Qt::DashLine);
    painter.setPen(pen);

    int p = (int) ((t - start).to_dbl_time() * scale + 0.5);

    painter.drawLine(rect.left() + p, rect.top() + outerLength,
            rect.left() + p, rect.bottom());
}

/****************************************************************************/
