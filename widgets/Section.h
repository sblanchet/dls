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

#ifndef DLS_SECTION_H
#define DLS_SECTION_H

#include <QTextDocument>

#include "../src/com_time.hpp"

namespace LibDLS {
    class Channel;
}

namespace DLS {

class Graph;
class Layer;

/****************************************************************************/

/** Graph section.
 */
class Section
{
    public:
        Section(Graph *graph);
        Section(const Section &);
        virtual ~Section();

        Graph *getGraph() { return graph; }

        bool getAutoScale() const { return autoScale; }
        void setAutoScale(bool);
        double getScaleMinimum() const { return scaleMin; }
        void setScaleMinimum(double);
        double getScaleMaximum() const { return scaleMax; }
        void setScaleMaximum(double);
        int getHeight() const { return height; };
        void setHeight(int);

        void resize(int);
        void draw(QPainter &, const QRect &, int);

        Layer *appendLayer(LibDLS::Channel *);

        void getRange(bool &, COMTime &, COMTime &);
        void loadData(const COMTime &, const COMTime &, int);

        QColor nextColor() const;

        enum {Margin = 1};

    private:
        Graph * const graph;
        QList<Layer *> layers; /**< List of data layers. */
        bool autoScale;
        double scaleMin;
        double scaleMax;
        int height;
        QTextDocument legend;

        static const QColor colorList[];

        void updateLegend();
};

/****************************************************************************/

} // namespace

#endif
