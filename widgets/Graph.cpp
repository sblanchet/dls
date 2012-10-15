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
#include "SectionDialog.h"

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
    prevViewAction(this),
    nextViewAction(this),
    zoomAction(this),
    panAction(this),
    measureAction(this),
    zoomInAction(this),
    zoomOutAction(this),
    zoomResetAction(this),
    removeSectionAction(this),
    sectionPropertiesAction(this),
    printAction(this),
    selectedSection(NULL),
    splitterWidth(
            QApplication::style()->pixelMetric(QStyle::PM_SplitterWidth)),
    splitterSection(NULL),
    movingSection(NULL),
    startHeight(0),
    scrollBar(this),
    scrollBarNeeded(false),
    currentView(views.begin())
{
    setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMinimumSize(60, 50);
    setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    scrollBar.setVisible(false);
    scrollBar.setCursor(Qt::ArrowCursor);
    connect(&scrollBar, SIGNAL(valueChanged(int)),
            this, SLOT(sliderValueChanged(int)));

    COMTime t, diff;
    t.set_now();
    diff.from_dbl_time(2000000000.0);
    scale.setRange(t - diff, t);
    newView();

    updateCursor();

    prevViewAction.setText(tr("&Previous view"));
    prevViewAction.setShortcut(Qt::ALT | Qt::Key_Left);
    prevViewAction.setStatusTip(tr("Return to previous view."));
    prevViewAction.setIcon(QIcon(":/images/edit-undo.svg"));
    connect(&prevViewAction, SIGNAL(triggered()), this, SLOT(previousView()));

    nextViewAction.setText(tr("&Next view"));
    nextViewAction.setShortcut(Qt::ALT | Qt::Key_Right);
    nextViewAction.setStatusTip(tr("Proceed to next view."));
    nextViewAction.setIcon(QIcon(":/images/edit-redo.svg"));
    connect(&nextViewAction, SIGNAL(triggered()), this, SLOT(nextView()));

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
    connect(&measureAction, SIGNAL(triggered()),
            this, SLOT(interactionSlot()));

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
    zoomResetAction.setStatusTip(
            tr("Automatically zoom to the data extent."));
    zoomResetAction.setIcon(QIcon(":/images/view-fullscreen.svg"));
    connect(&zoomResetAction, SIGNAL(triggered()), this, SLOT(zoomReset()));

    removeSectionAction.setText(tr("Remove section"));
    removeSectionAction.setStatusTip(tr("Remove the selected section."));
    removeSectionAction.setIcon(QIcon(":/images/list-remove.svg"));
    connect(&removeSectionAction, SIGNAL(triggered()),
            this, SLOT(removeSelectedSection()));

    sectionPropertiesAction.setText(tr("Section properties..."));
    sectionPropertiesAction.setStatusTip(tr("Open the section configuration"
                " dialog."));
    sectionPropertiesAction.setIcon(
            QIcon(":/images/document-properties.svg"));
    connect(&sectionPropertiesAction, SIGNAL(triggered()),
            this, SLOT(sectionProperties()));

    printAction.setText(tr("Print..."));
    printAction.setStatusTip(tr("Open the print dialog."));
    printAction.setIcon(QIcon(":/images/document-print.svg"));
    connect(&printAction, SIGNAL(triggered()), this, SLOT(print()));
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
    updateScrollBar();
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

    updateScrollBar();

    return s;
}

/****************************************************************************/

void Graph::removeSection(Section *section)
{
    int num = sections.removeAll(section);
    updateScrollBar();

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
        newView();
        update();
    }
}

/****************************************************************************/

void Graph::loadData()
{
    int dataWidth = contentsRect().width();
    if (scrollBarNeeded) {
        dataWidth -= scrollBar.width();
    }

    for (QList<Section *>::iterator s = sections.begin();
            s != sections.end(); s++) {
        (*s)->loadData(scale.getStart(), scale.getEnd(), dataWidth);
    }
    update();
}

/****************************************************************************/

void Graph::setRange(const COMTime &start, const COMTime &end)
{
    scale.setRange(start, end);
    autoRange = false;

    newView();
    loadData();
}

/****************************************************************************/

void Graph::previousView()
{
    if (currentView == views.begin()) {
        return;
    }

    currentView--;
    scale.setRange(currentView->start, currentView->end);
    autoRange = false;
    updateActions();
    loadData();
}

/****************************************************************************/

void Graph::nextView()
{
    if (currentView + 1 == views.end()) {
        return;
    }

    currentView++;
    scale.setRange(currentView->start, currentView->end);
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

    if (zooming && interaction != Zoom) {
        zooming = false;
    }
    if (panning && interaction != Pan) {
        panning = false;
    }

    updateMeasuring();
    updateActions();
    updateCursor();
    update();
}

/****************************************************************************/

void Graph::pan(double fraction)
{
    COMTime diff;
    diff.from_dbl_time((getEnd() - getStart()).to_dbl_time() * fraction);
    setRange(getStart() + diff, getEnd() + diff);
}

/****************************************************************************/

void Graph::print()
{
    QPrinter printer;
    printer.setOrientation(QPrinter::Landscape);
    printer.setPaperSize(QPrinter::A4);
    printer.setOutputFileName("dls-export.pdf");

    QPrintDialog dialog(&printer, this);
    dialog.exec();

    QPainter painter;

    if (!painter.begin(&printer)) {
        qWarning("failed to open file, is it writable?");
        return;
    }

    // get widget data height
    int displayHeight = contentsRect().height() - scale.getOuterLength() - 1;
    if (displayHeight <= 0) {
        qWarning("invalid data height.");
        return;
    }

    QRect page(printer.pageRect());
    page.moveTo(0, 0);

    Scale printScale(this);
    printScale.setRange(scale.getStart(), scale.getEnd());
    printScale.setLength(page.width());

    QRect scaleRect(page);

    QList<Section *>::iterator first = sections.begin();

    while (first != sections.end()) {

        // choose sections for current page
        QList<Section *>::iterator last = first;
        int heightSum = (*first)->getHeight();
        unsigned int count = 1;
        while (heightSum < displayHeight) {
            QList<Section *>::iterator next = last + 1;
            if (next == sections.end() ||
                    heightSum + (*next)->getHeight() > displayHeight) {
                break;
            }

            last = next;
            heightSum += (*last)->getHeight();
            count++;
        }

        printScale.draw(painter, scaleRect);

        QPen verLinePen;
        painter.setPen(verLinePen);
        painter.drawLine(page.left(),
                page.top() + printScale.getOuterLength(),
                page.right(),
                page.top() + printScale.getOuterLength());

        int top = page.top() + printScale.getOuterLength() + 1;
        QRect dataRect(page);
        dataRect.setTop(page.top() + printScale.getOuterLength() + 1);
        for (QList<Section *>::iterator s = first; s != last + 1; s++) {
            int height = (*s)->getHeight() * (dataRect.height() - count - 1)
                / heightSum;
            QRect r(page.left(), top, page.width(), height);

            Section drawSection(**s);
            drawSection.setHeight(height);
            drawSection.resize(page.width());
            drawSection.loadData(scale.getStart(), scale.getEnd(),
                page.width());
            drawSection.draw(painter, r, -1);

            QPen pen;
            painter.setPen(pen);
            painter.drawLine(page.left(), top + height,
                    page.right(), top + height);

            top += height + 1;
        }

        first = last + 1;
        if (first != sections.end()) {
            printer.newPage();
        }
    }

    painter.end();
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

    if (event->button() & Qt::LeftButton) {
        if (splitterSection) {
            movingSection = splitterSection;
            startHeight = splitterSection->getHeight();
        }
        else if (interaction == Zoom) {
            zooming = true;
        }
        else if (interaction == Pan) {
            panning = true;
        }

        update();
    }
}

/****************************************************************************/

/** Mouse press event.
 */
void Graph::mouseMoveEvent(QMouseEvent *event)
{
    endPos = event->pos();

    if (movingSection) {
        int dh = endPos.y() - startPos.y();
        int h = startHeight + dh;
        if (h < 0) {
            h = 0;
        }
        movingSection->setHeight(h);
        updateScrollBar();
    }

    if (zooming) {
        update();
    }

    if (panning) {
        int w = contentsRect().width();
        COMTime range = getEnd() - getStart();

        if (range > 0.0 && w > 0) {
            double xScale = range.to_dbl_time() / w;

            COMTime diff;
            diff.from_dbl_time((endPos.x() - startPos.x()) * xScale);
            scale.setRange(dragStart - diff, dragEnd - diff);
            autoRange = false;
            updateActions();
            update();
        }
    }

    updateMeasuring();

    Section *sec = NULL;
    int top = contentsRect().top() + scale.getOuterLength() + 1 -
        scrollBar.value();;
    QRect splitterRect(contentsRect());
    splitterRect.setHeight(splitterWidth);
    if (scrollBarNeeded) {
        splitterRect.setWidth(contentsRect().width() - scrollBar.width());
    }

    for (QList<Section *>::iterator s = sections.begin();
            s != sections.end(); s++) {
        if (top > event->pos().y()) {
            break;
        }
        splitterRect.moveTop(top + (*s)->getHeight());
        if (splitterRect.contains(event->pos())) {
            sec = *s;
            break;
        }
        top += (*s)->getHeight() + splitterWidth;
    }

    if (splitterSection != sec) {
        splitterSection = sec;
        update();
    }

    updateCursor();
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
    movingSection = NULL;
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
        case Qt::Key_M:
            setInteraction(Measure);
            break;
        case Qt::Key_Right:
            if (event->modifiers() & Qt::AltModifier) {
                nextView();
            }
            else {
                pan(0.125);
            }
            break;
        case Qt::Key_Left:
            if (event->modifiers() & Qt::AltModifier) {
                previousView();
            }
            else {
                pan(-0.125);
            }
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
    updateScrollBar();

    int dataWidth = contentsRect().width();
    if (scrollBarNeeded) {
        dataWidth -= scrollBar.width();
    }

    for (QList<Section *>::iterator s = sections.begin();
            s != sections.end(); s++) {
        (*s)->resize(dataWidth);
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
        if (height > contentsRect().height()) {
            height = contentsRect().height();
            break;
        }
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
    QRect dataRect(contentsRect());
    dataRect.setTop(top);
    if (scrollBarNeeded) {
        dataRect.setWidth(contentsRect().width() - scrollBar.width());
        scrollBar.move(dataRect.right() + 1, top);
        scrollBar.resize(scrollBar.width(), dataRect.height());
    }

    painter.setClipRect(dataRect);

    for (QList<Section *>::iterator s = sections.begin();
            s != sections.end(); s++) {
        QRect sectionRect(dataRect);
        sectionRect.setTop(top - scrollBar.value());
        sectionRect.setHeight((*s)->getHeight());
        (*s)->draw(painter, sectionRect, mp);

        QRect splitterRect(sectionRect);
        splitterRect.setTop(top + (*s)->getHeight() - scrollBar.value());
        splitterRect.setHeight(splitterWidth);
        painter.fillRect(splitterRect, palette().window());

        QStyleOption styleOption;
        styleOption.initFrom(this);
        styleOption.rect = splitterRect;
        QStyle::State state = QStyle::State_MouseOver;
        styleOption.state &= ~state;
        if (splitterSection == *s && !zooming && !panning) {
            styleOption.state |= state;
        }
        QStyle *style = QApplication::style();
        style->drawControl(QStyle::CE_Splitter, &styleOption, &painter, this);

        if ((*s == dropSection && dropLine < 0) || *s == selectedSection) {
            drawDropRect(painter, sectionRect);
        }

        top += (*s)->getHeight() + splitterWidth;
    }

    painter.setClipping(false);

    if (dropLine >= 0) {
        QPen pen;
        pen.setColor(Qt::blue);
        pen.setWidth(5);
        painter.setPen(pen);

        painter.drawLine(5, dropLine, dataRect.width() - 10, dropLine);
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
    sectionPropertiesAction.setEnabled(selectedSection);

    menu.addAction(&prevViewAction);
    menu.addAction(&nextViewAction);
    menu.addSeparator();
    menu.addAction(&zoomAction);
    menu.addAction(&panAction);
    menu.addAction(&measureAction);
    menu.addSeparator();
    menu.addAction(&zoomInAction);
    menu.addAction(&zoomOutAction);
    menu.addAction(&zoomResetAction);
    menu.addSeparator();
    menu.addAction(&removeSectionAction);
    menu.addAction(&sectionPropertiesAction);
    menu.addSeparator();
    menu.addAction(&printAction);

    menu.exec(event->globalPos());
    selectedSection = NULL;
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
    int top = contentsRect().top() + scale.getOuterLength() + 1 -
        scrollBar.value(), y = p.y();

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

        top += (*s)->getHeight() + splitterWidth;
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
    QCursor cur;

    if (zooming) {
        cur = Qt::ArrowCursor;
    }
    else if (panning) {
        cur = Qt::ClosedHandCursor;
    }
    else if (splitterSection) {
        cur = Qt::SizeVerCursor;
    }
    else if (interaction == Pan) {
        cur = Qt::OpenHandCursor;
    }

    setCursor(cur);
}

/****************************************************************************/

void Graph::updateActions()
{
    prevViewAction.setEnabled(currentView != views.begin());
    nextViewAction.setEnabled(currentView + 1 != views.end());
    zoomAction.setEnabled(interaction != Zoom);
    panAction.setEnabled(interaction != Pan);
    measureAction.setEnabled(interaction != Measure);
    zoomResetAction.setEnabled(!autoRange);
}

/****************************************************************************/

/** Updates the measuring flag.
 */
void Graph::updateMeasuring()
{
    if (interaction != Measure) {
        measuring = false;
        return;
    }

    QRect measureRect(contentsRect());
    COMTime range = getEnd() - getStart();

    if (range <= 0.0 || !measureRect.isValid() ||
            !measureRect.contains(endPos)) {
        measuring = false;
    }
    else {
        double xScale = range.to_dbl_time() / measureRect.width();

        measurePos = endPos.x() - measureRect.left();
        measureTime.from_dbl_time(measurePos * xScale);
        measureTime += getStart();
        measuring = true;
    }

    update();
}

/****************************************************************************/

/** Updates the scroll bar.
 */
void Graph::updateScrollBar()
{
    int height = 0;
    for (QList<Section *>::iterator s = sections.begin();
            s != sections.end(); s++) {
        QRect rect(contentsRect().left(), height,
                contentsRect().width(), (*s)->getHeight());
        height += (*s)->getHeight() + splitterWidth;
    }

    int displayHeight = contentsRect().height() - scale.getOuterLength() - 1;
    bool needed = height > displayHeight;

    if (needed) {
        scrollBar.setMaximum(height - displayHeight);
        scrollBar.setPageStep(displayHeight);
    }
    else {
        scrollBar.setMaximum(0);
    }

    if (needed != scrollBarNeeded) {
        scrollBarNeeded = needed;
        scrollBar.setVisible(needed);
        update();
    }
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

    int top = contentsRect().top() + scale.getOuterLength() + 1 -
        scrollBar.value();
    for (QList<Section *>::iterator s = sections.begin();
            s != sections.end(); s++) {
        QRect rect(contentsRect().left(), top,
                contentsRect().width(), (*s)->getHeight());
        if (rect.contains(pos)) {
            return *s;
        }
        top += (*s)->getHeight() + splitterWidth;
    }

    return NULL;
}

/****************************************************************************/

/** Draws a drop target rectangle.
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

/** Removes remaining views and places the current view at the end.
 */
void Graph::newView()
{
    if (currentView != views.end()) {
        // erase all views after the current
        views.erase(currentView + 1, views.end());
    }

    View v;
    v.start = scale.getStart();
    v.end = scale.getEnd();
    currentView = views.insert(views.end(), v);

    updateActions();
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

void Graph::sectionProperties()
{
    if (!selectedSection) {
        return;
    }

    SectionDialog *dialog = new SectionDialog(selectedSection, this);
    dialog->exec();
    delete dialog;
    selectedSection = NULL;
}

/****************************************************************************/

void Graph::sliderValueChanged(int value)
{
    Q_UNUSED(value);
    update();
}

/****************************************************************************/
