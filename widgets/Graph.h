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
#include <QAction>
#include <QtDesigner/QDesignerExportWidget>

#include "Scale.h"

namespace DLS {

class Section;

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

        Section *appendSection();
        Section *insertSectionBefore(Section *);

        void updateRange();
        void loadData();
        void setRange(const COMTime &, const COMTime &);
        const COMTime &getStart() const { return scale.getStart(); };
        const COMTime &getEnd() const { return scale.getEnd(); };

        enum Interaction {
            Zoom,
            Pan
        };

    public slots:
        void zoomIn();
        void zoomOut();
        void zoomReset();
        void setInteraction(Interaction);

    protected:
        bool event(QEvent *);
        void mousePressEvent(QMouseEvent *);
        void mouseReleaseEvent(QMouseEvent *);
        void mouseMoveEvent(QMouseEvent *);
        void keyPressEvent(QKeyEvent *event);
        void resizeEvent(QResizeEvent *);
        void paintEvent(QPaintEvent *);
        void contextMenuEvent(QContextMenuEvent *);
        void dragEnterEvent(QDragEnterEvent *);
        void dragLeaveEvent(QDragLeaveEvent *);
        void dragMoveEvent(QDragMoveEvent *);
        void dropEvent(QDropEvent *);
        void wheelEvent(QWheelEvent *);

    private:
        Scale scale;
        QList<Section *> sections; /**< List of data sections. */
        bool autoRange;
        Section *dropSection;
        int dropLine;
        int dropRemaining;
        QPoint startPos;
        QPoint endPos;
        COMTime dragStart;
        COMTime dragEnd;
        bool zooming;
        Interaction interaction;
        bool panning;
        QAction zoomAction;
        QAction panAction;
        QAction zoomInAction;
        QAction zoomOutAction;
        QAction zoomResetAction;

        void updateDragging(QPoint);
        void resetDragging();
        void updateCursor();
        void updateActions();

    private slots:
        void interactionSlot();

};

/****************************************************************************/

} // namespace

#endif
