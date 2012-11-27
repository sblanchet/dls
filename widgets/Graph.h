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

#include <QFrame>
#include <QAction>
#include <QScrollBar>
#include <QtDesigner/QDesignerExportWidget>

#include "Scale.h"

class QDomElement;

namespace QtDls {
    class Model;
}

namespace DLS {

class Section;

/****************************************************************************/

/** Graph widget.
 */
class QDESIGNER_WIDGET_EXPORT Graph:
    public QFrame
{
    Q_OBJECT

    public:
        Graph(QWidget *parent = 0);
        virtual ~Graph();

        void setDropModel(QtDls::Model *);

        virtual QSize sizeHint() const;

        bool load(const QString &, QtDls::Model *);
        bool save(const QString &) const;

        Section *appendSection();
        Section *insertSectionBefore(Section *);
        void removeSection(Section *);

        void updateRange();
        void loadData();
        void setRange(const COMTime &, const COMTime &);
        const COMTime &getStart() const { return scale.getStart(); };
        const COMTime &getEnd() const { return scale.getEnd(); };

        enum Interaction {
            Zoom,
            Pan,
            Measure
        };

        enum NamedRange {
            Today,
            Yesterday,
            ThisWeek,
            LastWeek,
            ThisMonth,
            LastMonth,
            ThisYear,
            LastYear
        };

    public slots:
        void previousView();
        void nextView();
        void zoomIn();
        void zoomOut();
        void zoomReset();
        void setInteraction(Interaction);
        void setNamedRange(NamedRange);
        void pan(double);
        void print();

    protected:
        bool event(QEvent *);
        void mousePressEvent(QMouseEvent *);
        void mouseReleaseEvent(QMouseEvent *);
        void mouseMoveEvent(QMouseEvent *);
        void leaveEvent(QEvent *);
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
        QtDls::Model *dropModel;
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
        bool measuring;
        int measurePos;
        COMTime measureTime;
        QAction prevViewAction;
        QAction nextViewAction;
        QAction zoomAction;
        QAction panAction;
        QAction measureAction;
        QAction zoomInAction;
        QAction zoomOutAction;
        QAction zoomResetAction;
        QAction pickDateAction;
        QAction gotoTodayAction;
        QAction gotoYesterdayAction;
        QAction gotoThisWeekAction;
        QAction gotoLastWeekAction;
        QAction gotoThisMonthAction;
        QAction gotoLastMonthAction;
        QAction gotoThisYearAction;
        QAction gotoLastYearAction;
        QAction removeSectionAction;
        QAction sectionPropertiesAction;
        QAction printAction;
        Section *selectedSection;
        const int splitterWidth;
        Section *splitterSection;
        Section *movingSection;
        int startHeight;
        QScrollBar scrollBar;
        bool scrollBarNeeded;

        struct View {
            COMTime start;
            COMTime end;
        };
        QList<View> views;
        QList<View>::iterator currentView;

        void updateDragging(QPoint);
        void resetDragging();
        void updateCursor();
        void updateActions();
        void updateMeasuring();
        void updateScrollBar();
        Section *sectionFromPos(const QPoint &);
        static void drawDropRect(QPainter &, const QRect &);
        void newView();
        void clearSections();
        bool loadSections(const QDomElement &, QtDls::Model *);

    private slots:
        void interactionSlot();
        void removeSelectedSection();
        void sectionProperties();
        void sliderValueChanged(int);
        void pickDate();
};

/****************************************************************************/

} // namespace

#endif
