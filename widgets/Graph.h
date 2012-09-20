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

#ifndef DLS_GRAPH_H
#define DLS_GRAPH_H

#include <QWidget>
#include <QtDesigner/QDesignerExportWidget>

//#include "Section.h"

namespace DLS {

/****************************************************************************/

/** Graph widget.
 */
class QDESIGNER_WIDGET_EXPORT Graph:
    public QWidget
{
    Q_OBJECT

    public:
        Graph(QWidget *parent = 0);
        virtual ~Graph();

        virtual QSize sizeHint() const;

    protected:
        bool event(QEvent *);
        void resizeEvent(QResizeEvent *);
        void paintEvent(QPaintEvent *);
        void contextMenuEvent(QContextMenuEvent *);

    private:
        //QList<Section *> sections; /**< List of data sections. */
};

/****************************************************************************/

} // namespace

#endif
