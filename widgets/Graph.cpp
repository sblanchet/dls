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

/****************************************************************************/

/** Constructor.
 */
Graph::Graph(
        QWidget *parent /**< parent widget */
        ): QWidget(parent),
    scale(this),
    dropSection(NULL),
    dropLine(-1),
    dropRemaining(-1)
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

/** Event handler.
 */
bool Graph::event(
        QEvent *event /**< Paint event flags. */
        )
{
    switch (event->type()) {
        case QEvent::MouseButtonDblClick:
            return true;

        case QEvent::LanguageChange:
            break;

        default:
            break;
    }

    return QWidget::event(event);
}

/****************************************************************************/

/** Handles the widget's resize event.
 */
void Graph::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    scale.setLength(width());
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
        pen.setWidth(3);
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

    int height_sum = 0;
    for (QList<Section *>::iterator s = sections.begin();
            s != sections.end(); s++) {
        (*s)->draw(painter, height_sum, width());
        height_sum += (*s)->getHeight();
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
}

/****************************************************************************/

void Graph::updateDragging(QPoint p)
{
    int height_sum = 0, y = p.y();

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
