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

#include <set>

#include <QTextDocument>
#include <QReadWriteLock>

#include "com_time.hpp"

#include "ValueScale.h"

class QDomElement;
class QDomDocument;

namespace LibDLS {
    class Job;
}

namespace QtDls {
    class Model;
    class Channel;
}

namespace DLS {

class Graph;
class GraphWorker;
class Layer;

/****************************************************************************/

/** Graph section.
 */
class Q_DECL_EXPORT Section
{
    friend class SectionModel;
    friend class Layer;

    public:
        Section(Graph *graph);
        Section(const Section &);
        virtual ~Section();

        Section &operator=(const Section &);

        void load(const QDomElement &, QtDls::Model *);
        void save(QDomElement &, QDomDocument &);

        Graph *getGraph() { return graph; }

        bool getAutoScale() const { return autoScale; }
        void setAutoScale(bool);
        bool getShowScale() const { return showScale; }
        void setShowScale(bool);
        double getScaleMinimum() const { return scaleMin; }
        void setScaleMinimum(double);
        double getScaleMaximum() const { return scaleMax; }
        void setScaleMaximum(double);
        int getHeight() const { return height; };
        void setHeight(int);

        void resize(int);
        int getScaleWidth() const { return scale.getWidth(); }
        void draw(QPainter &, const QRect &, int, int);
        int legendHeight() const { return legend.size().height(); }

        Layer *appendLayer(QtDls::Channel *);

        void getRange(bool &, COMTime &, COMTime &);
        void loadData(const COMTime &, const COMTime &, int, GraphWorker *,
                std::set<LibDLS::Job *> &);

        QColor nextColor();

        enum {Margin = 1};

        bool getExtrema(double &, double &);

        class Exception {
            public:
                Exception(const QString &msg):
                    msg(msg) {}
                QString msg;
        };

        void setBusy(bool);
        void update();

        QSet<QtDls::Channel *> channels();

    private:
        Graph * const graph;
        ValueScale scale;
        QReadWriteLock rwLockLayers;
        QList<Layer *> layers; /**< List of data layers. */
        bool autoScale;
        bool showScale;
        double scaleMin;
        double scaleMax;
        int height;
        QTextDocument legend;
        double minimum;
        double maximum;
        bool extremaValid;
        bool busy;

        static const QColor colorList[];

        void updateLegend();
        void updateScale();
        void updateExtrema();
        void clearLayers();
        void loadLayers(const QDomElement &, QtDls::Model *);
};

/****************************************************************************/

} // namespace

#endif
