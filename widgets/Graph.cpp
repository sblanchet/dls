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

using DLS::Graph;
using DLS::Section;

#define DROP_TOLERANCE 10
#define MARGIN 5

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
    zooming(false)
{
    //setAttribute(Qt::WA_NoBackground);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMinimumSize(60, 50);
    setAcceptDrops(true);
    COMTime t, diff;
    t.set_now();
    diff.from_dbl_time(20.0);
    scale.setStart(t - diff);
    scale.setEnd(t);
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
    if (!autoRange) {
        return;
    }

    COMTime start, end;
    bool valid = false;

    for (QList<Section *>::const_iterator s = sections.begin();
            s != sections.end(); s++) {
        (*s)->getRange(valid, start, end);
    }

    if (valid) {
        scale.setStart(start);
        scale.setEnd(end);
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

void Graph::setStart(const COMTime &t)
{
    scale.setStart(t);
}

/****************************************************************************/

void Graph::setEnd(const COMTime &t)
{
    scale.setEnd(t);
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
            zooming = false;
            autoRange = true;
            updateRange();
            loadData();
            update();
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
    zooming = true;
    update();
}

/****************************************************************************/

/** Mouse press event.
 */
void Graph::mouseMoveEvent(QMouseEvent *event)
{
    endPos = event->pos();
    update();
}

/****************************************************************************/

/** Mouse release event.
 */
void Graph::mouseReleaseEvent(QMouseEvent *event)
{
    zooming = false;
    update();

    if (startPos.x() == endPos.x()) {
        return;
    }

    if (width() - 2 * MARGIN <= 0) {
        return;
    }

    COMTime range = getEnd() - getStart();

    if (range <= 0.0) {
        return;
    }

    double scale = range.to_dbl_time() / (width() - 2 * MARGIN);

    COMTime diff;
    diff.from_dbl_time((startPos.x() - MARGIN) * scale);
    COMTime newStart = getStart() + diff;
    diff.from_dbl_time((event->pos().x() - MARGIN) * scale);
    COMTime newEnd = getStart() + diff;

    setStart(newStart);
    setEnd(newEnd);
    autoRange = false;

    loadData();
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
    Q_UNUSED(event);
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
