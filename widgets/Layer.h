/*****************************************************************************
 *
 * $Id$
 *
 * Copyright (C) 2009 - 2012  Florian Pose <fp@igh-essen.com>
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

#ifndef DLS_LAYER_H
#define DLS_LAYER_H

namespace LibDLS {
    class Channel;
}

namespace DLS {

class Section;

/****************************************************************************/

/** Graph section layer.
 */
class Layer
{
    public:
        Layer(Section *, LibDLS::Channel *, const QColor & = QColor());
        Layer(const Layer &, Section *);
        virtual ~Layer();

        LibDLS::Channel *getChannel() const { return channel; };

        void setColor(const QColor &);
        QColor getColor() const { return color; }

        void loadData(const COMTime &, const COMTime &, int);

        struct MeasureData {
            const Layer *layer;
            int x;
            double minimum;
            double maximum;
            int minY;
            int maxY;
            int meanY;
            unsigned int group;
            int movedY;
            bool found;

            bool operator<(const MeasureData &other) const {
                return minimum < other.minimum;
            }
        };

        void draw(QPainter &, const QRect &, double, double, double,
                MeasureData * = NULL) const;

        double getMinimum() const { return minimum; }
        double getMaximum() const { return maximum; }
        double getExtremaValid() const { return extremaValid; }

    private:
        Section * const section;
        LibDLS::Channel * const channel;
        QColor color;
        QList<LibDLS::Data *> genericData;
        QList<LibDLS::Data *> minimumData;
        QList<LibDLS::Data *> maximumData;
        double minimum;
        double maximum;
        bool extremaValid;

        struct TimeRange
        {
            COMTime start;
            COMTime end;
        };
        static bool range_before(const TimeRange &, const TimeRange &);

        static int dataCallback(LibDLS::Data *, void *);
        void newData(LibDLS::Data *);
        void clearDataList(QList<LibDLS::Data *> &);
        void updateExtrema(const QList<LibDLS::Data *> &, bool *);
        void drawGaps(QPainter &, const QRect &, double) const;
};

/****************************************************************************/

} // namespace

#endif
