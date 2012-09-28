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

#include "lib_channel.hpp"

#include "Graph.h"
#include "Section.h"
#include "Layer.h"

using DLS::Graph;
using DLS::Section;

#define DROP_TOLERANCE 10

/****************************************************************************/

/** Constructor.
 */
Graph::Graph(
        QWidget *parent /**< parent widget */
        ): QWidget(parent),
    scale(this),
    autoRange(true),
    dropSection(NULL),
    dropLine(-1),
    dropRemaining(-1),
    zooming(false),
    interaction(Zoom),
    panning(false),
    zoomAction(this),
    panAction(this),
    zoomInAction(this),
    zoomOutAction(this),
    zoomResetAction(this)
{
    //setAttribute(Qt::WA_NoBackground);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMinimumSize(60, 50);
    setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus);
    COMTime t, diff;
    t.set_now();
    diff.from_dbl_time(2000000000.0);
    scale.setRange(t - diff, t);
    updateActions();

    zoomAction.setText(tr("&Zoom"));
    zoomAction.setShortcut(Qt::Key_Z);
    zoomAction.setStatusTip(tr("Set mouse interaction to zooming."));
    zoomAction.setIcon(QIcon(":/images/system-search.svg"));
    connect(&zoomAction, SIGNAL(triggered()), this, SLOT(interactionSlot()));

    panAction.setText(tr("&Pan"));
    panAction.setShortcut(Qt::Key_P);
    panAction.setStatusTip(tr("Set mouse interaction to panning."));
    panAction.setIcon(QIcon(":/images/go-next.svg"));
    connect(&panAction, SIGNAL(triggered()), this, SLOT(interactionSlot()));

    zoomInAction.setText(tr("Zoom in"));
    zoomInAction.setShortcut(Qt::Key_Plus);
    zoomInAction.setStatusTip(tr("Zoom the current view in to half"
                " of the time around the center."));
    zoomInAction.setIcon(QIcon(":/images/system-search.svg"));
    connect(&zoomInAction, SIGNAL(triggered()), this, SLOT(zoomIn()));

    zoomOutAction.setText(tr("Zoom out"));
    zoomOutAction.setShortcut(Qt::Key_Minus);
    zoomOutAction.setStatusTip(tr("Zoom the current view out the double"
                " time around the center."));
    zoomOutAction.setIcon(QIcon(":/images/system-search.svg"));
    connect(&zoomOutAction, SIGNAL(triggered()), this, SLOT(zoomOut()));

    zoomResetAction.setText(tr("Auto range"));
    zoomResetAction.setShortcut(Qt::Key_Minus);
    zoomResetAction.setStatusTip(tr("Automatically zoom to the data extent."));
    zoomResetAction.setIcon(QIcon(":/images/view-fullscreen.svg"));
    connect(&zoomResetAction, SIGNAL(triggered()), this, SLOT(zoomReset()));
}

/****************************************************************************/

/** Destructor.
 */
Graph::~Graph()
{
}

/****************************************************************************/

/** Gives a hint aboute the optimal size.
 */
QSize Graph::sizeHint() const
{
    return QSize(300, 100);
}

/****************************************************************************/

Section *Graph::appendSection()
{
    Section *s = new Section(this);
    sections.append(s);
    return s;
}

/****************************************************************************/

Section *Graph::insertSectionBefore(Section *before)
{
    int index = sections.indexOf(before);
    Section *s = new Section(this);

    if (index > -1) {
        sections.insert(index, s);
    }
    else {
        sections.append(s);
    }

    return s;
}

/****************************************************************************/

void Graph::updateRange()
{
    COMTime start, end;
    bool valid = false;

    for (QList<Section *>::const_iterator s = sections.begin();
            s != sections.end(); s++) {
        (*s)->getRange(valid, start, end);
    }

    if (valid) {
        scale.setRange(start, end);
        update();
    }
}

/****************************************************************************/

void Graph::loadData()
{
    for (QList<Section *>::iterator s = sections.begin();
            s != sections.end(); s++) {
        (*s)->loadData(scale.getStart(), scale.getEnd(),
                contentsRect().width());
    }
    update();
}

/****************************************************************************/

void Graph::setRange(const COMTime &start, const COMTime &end)
{
    scale.setRange(start, end);
}

/****************************************************************************/

void Graph::zoomIn()
{
    COMTime diff;
    diff.from_dbl_time((getEnd() - getStart()).to_dbl_time() / 4.0);
    setRange(getStart() + diff, getEnd() - diff);
    autoRange = false;
    updateActions();
    loadData();
}

/****************************************************************************/

void Graph::zoomOut()
{
    COMTime diff;
    diff.from_dbl_time((getEnd() - getStart()).to_dbl_time() / 2.0);
    setRange(getStart() - diff, getEnd() + diff);
    autoRange = false;
    updateActions();
    loadData();
}

/****************************************************************************/

void Graph::zoomReset()
{
    autoRange = true;
    updateActions();
    updateRange();
    loadData();
}

/****************************************************************************/

void Graph::setInteraction(Interaction i)
{
    interaction = i;

    updateActions();
    updateCursor();
}

/****************************************************************************/

/** Event handler.
 */
bool Graph::event(
        QEvent *event /**< Paint event flags. */
        )
{
    switch (event->type()) {
        case QEvent::MouseButtonDblClick:
            zoomReset();
            break;

        case QEvent::LanguageChange:
            break;

        default:
            break;
    }

    return QWidget::event(event);
}

/****************************************************************************/

/** Mouse press event.
 */
void Graph::mousePressEvent(QMouseEvent *event)
{
    startPos = event->pos();
    endPos = event->pos();
    dragStart = getStart();
    dragEnd = getEnd();
}

/****************************************************************************/

/** Mouse press event.
 */
void Graph::mouseMoveEvent(QMouseEvent *event)
{
    endPos = event->pos();

    if (endPos.x() != startPos.x() && interaction == Zoom) {
        zooming = true;
        updateCursor();
        update();
    }

    if (interaction == Pan) {
        int w = width() - 2 * Layer::Margin;
        COMTime range = getEnd() - getStart();
        double x_scale = range.to_dbl_time() / w;

        if (range <= 0.0 || w <= 0) {
            return;
        }

        panning = true;
        COMTime diff;
        diff.from_dbl_time((endPos.x() - startPos.x()) * x_scale);
        setRange(dragStart - diff, dragEnd - diff);
        autoRange = false;
        updateActions();
        updateCursor();
        update();
    }
}

/****************************************************************************/

/** Mouse release event.
 */
void Graph::mouseReleaseEvent(QMouseEvent *event)
{
    bool wasZooming = zooming;
    bool wasPanning = panning;
    int w = width() - 2 * Layer::Margin;
    COMTime range = getEnd() - getStart();

    zooming = false;
    panning = false;
    updateCursor();
    update();

    if (startPos.x() == endPos.x() || w <= 0 || range <= 0.0) {
        return;
    }

    double x_scale = range.to_dbl_time() / w;

    if (wasZooming) {
        COMTime diff;
        diff.from_dbl_time((startPos.x() - Layer::Margin) * x_scale);
        COMTime newStart = getStart() + diff;
        diff.from_dbl_time((event->pos().x() - Layer::Margin) * x_scale);
        COMTime newEnd = getStart() + diff;
        setRange(newStart, newEnd);
        autoRange = false;
        updateActions();
    }
    else if (wasPanning) {
        COMTime diff;
        diff.from_dbl_time((endPos.x() - startPos.x()) * x_scale);
        setRange(dragStart - diff, dragEnd - diff);
        autoRange = false;
        updateActions();
    }

    loadData();
}

/****************************************************************************/

void Graph::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
        case Qt::Key_Plus:
            zoomIn();
            break;
        case Qt::Key_Minus:
            zoomOut();
            break;
        case Qt::Key_Z:
            setInteraction(Zoom);
            break;
        case Qt::Key_P:
            setInteraction(Pan);
            break;
        default:
            QWidget::keyPressEvent(event);
            break;
    }
}

/****************************************************************************/

/** Handles the widget's resize event.
 */
void Graph::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    scale.setLength(width());
    loadData();
}

/****************************************************************************/

/** Paint function.
 */
void Graph::paintEvent(
        QPaintEvent *event /**< paint event flags */
        )
{
    Q_UNUSED(event);

    QPainter painter(this);

    scale.draw(painter, contentsRect());

    if (dropLine >= 0) {
        QPen pen;
        pen.setColor(Qt::blue);
        pen.setWidth(5);
        painter.setPen(pen);

        painter.drawLine(5, dropLine, width() - 10, dropLine);
    }
    else if (dropRemaining >= 0) {
        QPen pen;
        QBrush brush = painter.brush();

        pen.setColor(Qt::blue);
        painter.setPen(pen);

        brush.setColor(QColor(0, 0, 255, 63));
        brush.setStyle(Qt::SolidPattern);
        painter.setBrush(brush);

        painter.drawRect(5, dropRemaining + 5,
                width() - 10, height() - dropRemaining - 10);
    }

    int height_sum = scale.getOuterLength();
    for (QList<Section *>::iterator s = sections.begin();
            s != sections.end(); s++) {
        (*s)->draw(painter, height_sum, width());
        height_sum += (*s)->getHeight();
    }

    if (zooming) {
        QPen pen;
        pen.setColor(Qt::red);
        painter.setPen(pen);

        painter.drawLine(startPos.x(), scale.getOuterLength() + 5,
                startPos.x(), height() - 5);
        pen.setColor(Qt::yellow);
        painter.setPen(pen);
        painter.drawLine(endPos.x(), scale.getOuterLength() + 5,
                endPos.x(), height() - 5);
    }
}

/****************************************************************************/

/** Shows the context menu.
 */
void Graph::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);
    menu.addAction(&zoomAction);
    menu.addAction(&panAction);
    menu.addSeparator();
    menu.addAction(&zoomInAction);
    menu.addAction(&zoomOutAction);
    menu.addAction(&zoomResetAction);
    menu.exec(event->globalPos());
}

/****************************************************************************/

void Graph::dragEnterEvent(QDragEnterEvent *event)
{
    if (!event->mimeData()->hasFormat("application/dls_channel")) {
        return;
    }

    updateDragging(event->pos());

    event->acceptProposedAction();
}

/****************************************************************************/

void Graph::dragLeaveEvent(QDragLeaveEvent *event)
{
    Q_UNUSED(event);

    resetDragging();
}

/****************************************************************************/

void Graph::dragMoveEvent(QDragMoveEvent *event)
{
    updateDragging(event->pos());
}

/****************************************************************************/

void Graph::dropEvent(QDropEvent *event)
{
    Section *s = NULL;

    updateDragging(event->pos());

    if (dropSection) {
        if (dropLine >= 0) {
            /* insert before dropSection */
            s = insertSectionBefore(dropSection);
        }
        else {
            s = dropSection;
        }
    }
    else {
        s = appendSection();
    }

    QByteArray ba = event->mimeData()->data("application/dls_channel");
    QDataStream stream(&ba, QIODevice::ReadOnly);
    quint64 addr;
    while (!stream.atEnd()) {
        stream >> addr;
        LibDLS::Channel *ch = (LibDLS::Channel *) addr;
        s->appendLayer(ch);
    }

    resetDragging();
    event->acceptProposedAction();

    loadData();
}

/****************************************************************************/

void Graph::wheelEvent(QWheelEvent *event)
{
    if (event->delta() > 0) { // zoom in
        zoomIn();
    }
    else { // zoom out
        zoomOut();
    }
}

/****************************************************************************/

void Graph::updateDragging(QPoint p)
{
    int height_sum = scale.getOuterLength(), y = p.y();

    resetDragging();

    for (QList<Section *>::iterator s = sections.begin();
            s != sections.end(); s++) {
        if (y <= height_sum + DROP_TOLERANCE) {
            dropSection = *s;
            dropLine = height_sum;
            break;
        } else if (y <= height_sum + (*s)->getHeight() - DROP_TOLERANCE) {
            dropSection = *s;
            break;
        }

        height_sum += (*s)->getHeight();
    }

    if (dropSection) {
        if (dropLine < 0) {
            dropSection->setDropTarget(true);
        }
        update();
    } else { // no sections
        dropRemaining = height_sum;
        update();
    }
}

/****************************************************************************/

void Graph::resetDragging()
{
    for (QList<Section *>::iterator s = sections.begin();
            s != sections.end(); s++) {
        (*s)->setDropTarget(false);
    }
    dropSection = NULL;
    dropLine = -1;
    dropRemaining = -1;
    update();
}

/****************************************************************************/

void Graph::updateCursor()
{
    switch (interaction) {
        case Zoom:
            setCursor(Qt::ArrowCursor);
            break;
        case Pan:
            if (panning) {
                setCursor(Qt::ClosedHandCursor);
            }
            else {
                setCursor(Qt::OpenHandCursor);
            }
            break;
    }
}

/****************************************************************************/

void Graph::updateActions()
{
    zoomAction.setEnabled(interaction != Zoom);
    panAction.setEnabled(interaction != Pan);
    zoomResetAction.setEnabled(!autoRange);
}

/****************************************************************************/

void Graph::interactionSlot()
{
    if (sender() == &zoomAction) {
        setInteraction(Zoom);
    }
    else if (sender() == &panAction) {
        setInteraction(Pan);
    }
}

/****************************************************************************/
