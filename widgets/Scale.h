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

#ifndef DLS_SCALE_H
#define DLS_SCALE_H

#include <QRect>

#include "com_time.hpp"

namespace DLS {

/****************************************************************************/

/** Scale.
 */
class Scale
{
    public:
        Scale(QWidget *);

        COMTime getStart() const;
        void setStart(const COMTime &);
        COMTime getEnd() const;
        void setEnd(const COMTime &);
        int getLength() const;
        void setLength(int);

        int getOuterLength() const;

        void draw(QPainter &, const QRect &) const;

    private:
        const QWidget * const parent; /**< Parent widget. */
        COMTime start; /**< Minimum value. */
        COMTime end; /**< Maximum value. */
        int length; /**< Scale length in pixel. */

        int outerLength; /**< Space for the numbering in pixel. */
        double majorStep; /**< The major division (long ticks). */
        unsigned int minorDiv; /**< The minor division (short ticks). */
        QString format;
        int subDigits;

        Scale();
        void update();
        QString formatValue(double) const;
};

/****************************************************************************/

/**
 * \return The scale minimum value (#start).
 */
inline COMTime Scale::getStart() const
{
    return start;
}

/****************************************************************************/

/**
 * \return The scale maximum value (#end).
 */
inline COMTime Scale::getEnd() const
{
    return end;
}

/****************************************************************************/

/**
 * \return The #length.
 */
inline int Scale::getLength() const
{
    return length;
}

/****************************************************************************/

/**
 * \return The #outerLength.
 */
inline int Scale::getOuterLength() const
{
    return outerLength;
}

/****************************************************************************/

} // namespace

#endif
