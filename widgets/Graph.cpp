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
        ): QFrame(parent),
    scale(this),
    autoRange(true),
    dropSection(NULL),
    dropLine(-1),
    dropRemaining(-1),
    zooming(false),
    interaction(Pan),
    panning(false),
    measuring(false),
    zoomAction(this),
    panAction(this),
    measureAction(this),
    zoomInAction(this),
    zoomOutAction(this),
    zoomResetAction(this),
    removeSectionAction(this),
    selectedSection(NULL)
{
    setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMinimumSize(60, 50);
    setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus);
    COMTime t, diff;
    t.set_now();
    diff.from_dbl_time(2000000000.0);
    scale.setRange(t - diff, t);
    updateActions();
    updateCursor();

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

    measureAction.setText(tr("&Measure"));
    measureAction.setShortcut(Qt::Key_M);
    measureAction.setStatusTip(tr("Set mouse interaction to measuring."));
    measureAction.setIcon(QIcon(":/images/measure.svg"));
    connect(&measureAction, SIGNAL(triggered()), this, SLOT(interactionSlot()));

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

    removeSectionAction.setText(tr("Remove section"));
    removeSectionAction.setStatusTip(tr("Remove the selected section."));
    removeSectionAction.setIcon(QIcon(":/images/list-remove.svg"));
    connect(&removeSectionAction, SIGNAL(triggered()),
            this, SLOT(removeSelectedSection()));
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

void Graph::removeSection(Section *section)
{
    int num = sections.removeAll(section);

    delete section;

    if (num > 0) {
        update();
    }
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
    autoRange = false;
    updateActions();
    loadData();
}

/****************************************************************************/

void Graph::zoomIn()
{
    COMTime diff;
    diff.from_dbl_time((getEnd() - getStart()).to_dbl_time() / 4.0);
    setRange(getStart() + diff, getEnd() - diff);
}

/****************************************************************************/

void Graph::zoomOut()
{
    COMTime diff;
    diff.from_dbl_time((getEnd() - getStart()).to_dbl_time() / 2.0);
    setRange(getStart() - diff, getEnd() + diff);
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

    if (measuring && interaction != Measure) {
        measuring = false;
    }

    setMouseTracking(interaction == Measure);

    updateActions();
    updateCursor();
}

/****************************************************************************/

void Graph::pan(double fraction)
{
    COMTime diff;
    diff.from_dbl_time((getEnd() - getStart()).to_dbl_time() * fraction);
    setRange(getStart() + diff, getEnd() + diff);
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
        int w = contentsRect().width();
        COMTime range = getEnd() - getStart();

        if (range <= 0.0 || w <= 0) {
            return;
        }

        double xScale = range.to_dbl_time() / w;

        panning = true;
        COMTime diff;
        diff.from_dbl_time((endPos.x() - startPos.x()) * xScale);
        scale.setRange(dragStart - diff, dragEnd - diff);
        autoRange = false;
        updateActions();
        updateCursor();
        update();
    }
    else if (interaction == Measure) {
        QRect measureRect(contentsRect());
        COMTime range = getEnd() - getStart();

        if (range <= 0.0 || !measureRect.isValid() ||
                !measureRect.contains(endPos)) {
            measuring = false;
            update();
            return;
        }

        double xScale = range.to_dbl_time() / measureRect.width();

        measurePos = endPos.x() - measureRect.left();
        measureTime.from_dbl_time(measurePos * xScale);
        measureTime += getStart();
        measuring = true;
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
    int w = contentsRect().width();
    COMTime range = getEnd() - getStart();

    zooming = false;
    panning = false;
    updateCursor();
    update();

    if (startPos.x() == endPos.x() || w <= 0 || range <= 0.0) {
        return;
    }

    double xScale = range.to_dbl_time() / w;

    if (wasZooming) {
        COMTime diff;
        diff.from_dbl_time((startPos.x() - contentsRect().left()) * xScale);
        COMTime newStart = getStart() + diff;
        diff.from_dbl_time(
                (event->pos().x() - contentsRect().left()) * xScale);
        COMTime newEnd = getStart() + diff;
        setRange(newStart, newEnd);
    }
    else if (wasPanning) {
        COMTime diff;
        diff.from_dbl_time((endPos.x() - startPos.x()) * xScale);
        setRange(dragStart - diff, dragEnd - diff);
    }
}

/****************************************************************************/

/** Mouse release event.
 */
void Graph::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    measuring = false;
    update();
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
        case Qt::Key_Right:
            pan(0.125);
            break;
        case Qt::Key_Left:
            pan(-0.125);
            break;
        case Qt::Key_PageUp:
            pan(1.0);
            break;
        case Qt::Key_PageDown:
            pan(-1);
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

    scale.setLength(contentsRect().width());

    for (QList<Section *>::iterator s = sections.begin();
            s != sections.end(); s++) {
        (*s)->resize(contentsRect().width());
    }

    loadData();
}

/****************************************************************************/

/** Paint function.
 */
void Graph::paintEvent(
        QPaintEvent *event /**< paint event flags */
        )
{
    QFrame::paintEvent(event);

    QPainter painter(this);

    QRect scaleRect(contentsRect());
    int height = scale.getOuterLength();
    for (QList<Section *>::iterator s = sections.begin();
            s != sections.end(); s++) {
        height += 1 + (*s)->getHeight();
    }
    scaleRect.setHeight(height);
    scale.draw(painter, scaleRect);

    QPen verLinePen;
    painter.setPen(verLinePen);
    painter.drawLine(contentsRect().left(),
            contentsRect().top() + scale.getOuterLength(),
            contentsRect().right(),
            contentsRect().top() + scale.getOuterLength());

    int top = contentsRect().top() + scale.getOuterLength() + 1;
    int mp = measuring ? measurePos : -1;
    for (QList<Section *>::iterator s = sections.begin();
            s != sections.end(); s++) {
        QRect r(contentsRect().left(), top,
                contentsRect().width(), (*s)->getHeight());
        (*s)->draw(painter, r, mp);

        painter.setPen(verLinePen);
        painter.drawLine(contentsRect().left(),
                top + (*s)->getHeight(),
                contentsRect().right(),
                top + (*s)->getHeight());

        if (*s == dropSection && dropLine < 0) {
            drawDropRect(painter, r);
        }

        top += (*s)->getHeight() + 1;
    }

    if (dropLine >= 0) {
        QPen pen;
        pen.setColor(Qt::blue);
        pen.setWidth(5);
        painter.setPen(pen);

        painter.drawLine(5, dropLine, contentsRect().width() - 10, dropLine);
    }
    else if (dropRemaining >= 0) {
        QRect remRect(contentsRect());
        remRect.setTop(dropRemaining);
        drawDropRect(painter, remRect);
    }

    if (zooming) {
        QPen pen;
        pen.setColor(Qt::red);
        painter.setPen(pen);

        painter.drawLine(startPos.x(),
                contentsRect().top() + scale.getOuterLength() + 1,
                startPos.x(), contentsRect().bottom());
        pen.setColor(Qt::yellow);
        painter.setPen(pen);
        painter.drawLine(endPos.x(),
                contentsRect().top() + scale.getOuterLength() + 1,
                endPos.x(), contentsRect().bottom());
    }

    if (measuring) {
        QPen pen;
        pen.setColor(Qt::darkBlue);
        painter.setPen(pen);

        painter.drawLine(endPos.x(), contentsRect().top(),
                endPos.x(), contentsRect().bottom());

        QRect textRect(contentsRect());
        textRect.setLeft(endPos.x() + 3);
        textRect.setTop(contentsRect().top() + 2);
        textRect.setHeight(contentsRect().height() - 4);
        QString label(measureTime.to_real_time().c_str());
        QFontMetrics fm(painter.font());
        QSize s = fm.size(0, label);
        if (s.width() <= textRect.width()) {
            painter.fillRect(
                    QRect(textRect.topLeft(), s).adjusted(-2, 0, 2, 0),
                    Qt::white);
            painter.drawText(textRect, Qt::AlignLeft, label);
        }
        else {
            textRect.setLeft(contentsRect().left());
            textRect.setRight(endPos.x() - 3);
            if (s.width() <= textRect.width()) {
                painter.fillRect(QRect(
                            QPoint(textRect.right() + 1 - s.width(),
                            textRect.top()), s).adjusted(-2, 0, 2, 0),
                        Qt::white);
                painter.drawText(textRect, Qt::AlignRight, label);
            }
        }
    }
}

/****************************************************************************/

/** Shows the context menu.
 */
void Graph::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);

    selectedSection = sectionFromPos(event->pos());
    removeSectionAction.setEnabled(selectedSection);

    menu.addAction(&zoomAction);
    menu.addAction(&panAction);
    menu.addAction(&measureAction);
    menu.addSeparator();
    menu.addAction(&zoomInAction);
    menu.addAction(&zoomOutAction);
    menu.addAction(&zoomResetAction);
    menu.addSeparator();
    menu.addAction(&removeSectionAction);

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
    int top = contentsRect().top() + scale.getOuterLength() + 1, y = p.y();

    resetDragging();

    for (QList<Section *>::iterator s = sections.begin();
            s != sections.end(); s++) {
        if (y <= top + DROP_TOLERANCE) {
            dropSection = *s;
            dropLine = top;
            break;
        } else if (y <= top + (*s)->getHeight() - DROP_TOLERANCE) {
            dropSection = *s;
            break;
        }

        top += (*s)->getHeight() + 1;
    }

    if (!dropSection) {
        dropRemaining = top;
    }

    update();
}

/****************************************************************************/

void Graph::resetDragging()
{
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
        case Measure:
            setCursor(Qt::ArrowCursor);
            break;
    }
}

/****************************************************************************/

void Graph::updateActions()
{
    zoomAction.setEnabled(interaction != Zoom);
    panAction.setEnabled(interaction != Pan);
    measureAction.setEnabled(interaction != Measure);
    zoomResetAction.setEnabled(!autoRange);
}

/****************************************************************************/

Section *Graph::sectionFromPos(const QPoint &pos)
{
    if (!contentsRect().contains(pos)) {
        return NULL;
    }

    QRect scaleRect(contentsRect());
    scaleRect.setHeight(scale.getOuterLength());
    if (scaleRect.contains(pos)) {
        return NULL;
    }

    int top = contentsRect().top() + scale.getOuterLength() + 1;
    for (QList<Section *>::iterator s = sections.begin();
            s != sections.end(); s++) {
        QRect rect(contentsRect().left(), top,
                contentsRect().width(), (*s)->getHeight());
        if (rect.contains(pos)) {
            return *s;
        }
        top += (*s)->getHeight() + 1;
    }

    return NULL;
}

/****************************************************************************/

/** Draws a dop target rectangle.
 */
void Graph::drawDropRect(QPainter &painter, const QRect &rect)
{
    QPen pen;
    QBrush brush;

    pen.setColor(Qt::blue);
    painter.setPen(pen);

    brush.setColor(QColor(0, 0, 255, 63));
    brush.setStyle(Qt::SolidPattern);
    painter.setBrush(brush);

    painter.drawRect(rect);
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
    else if (sender() == &measureAction) {
        setInteraction(Measure);
    }
}

/****************************************************************************/

void Graph::removeSelectedSection()
{
    if (!selectedSection) {
        return;
    }

    removeSection(selectedSection);
    selectedSection = NULL;
}

/****************************************************************************/
