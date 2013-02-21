/*****************************************************************************
 *
 * $Id$
 *
 ****************************************************************************/

#include <QDebug>
#include <QUrl>
#include <QIcon>

#include <algorithm>

#include "lib_export.hpp"

#include "Channel.h"

using namespace QtDls;

/*****************************************************************************/

Channel::Channel(
        Node *parent,
        LibDLS::Channel *channel
        ):
    Node(parent),
    ch(channel)
{
}

/****************************************************************************/

Channel::~Channel()
{
}

/****************************************************************************/

QUrl Channel::url() const
{
    QUrl u = parent()->url();
    QString path = u.path();
    path += ch->name().c_str();
    u.setPath(path);
    return u;
}

/****************************************************************************/

QString Channel::name() const
{
    return ch->name().c_str();
}

/****************************************************************************/

void Channel::fetchData(COMTime start, COMTime end, unsigned int min_values,
        LibDLS::DataCallback callback, void *priv, unsigned int decimation)
{
    mutex.lock();
    ch->fetch_chunks();
    ch->fetch_data(start, end, min_values, callback, priv, decimation);
    mutex.unlock();
}

/****************************************************************************/

bool Channel::beginExport(LibDLS::Export *exporter, const QString &path)
{
    mutex.lock();

    try {
        exporter->begin(*ch, path.toLocal8Bit().constData());
    }
    catch (LibDLS::ExportException &e) {
        mutex.unlock();
        qDebug() << "export begin failed: " << e.msg.c_str();
        return false;
    }

    mutex.unlock();
    return true;
}

/****************************************************************************/

vector<Channel::TimeRange> Channel::chunkRanges()
{
    vector<TimeRange> ranges;

    for (list<LibDLS::Chunk>::const_iterator c = ch->chunks().begin();
            c != ch->chunks().end(); c++) {
        TimeRange r;
        r.start = c->start();
        r.end = c->end();
        ranges.push_back(r);
    }

    sort(ranges.begin(), ranges.end(), range_before);

    return ranges;
}

/****************************************************************************/

bool Channel::getRange(COMTime &start, COMTime &end)
{
    if (ch->chunks().empty()) {
        return false;
    }
    else {
        start = ch->start();
        end = ch->end();
        return true;
    }
}

/****************************************************************************/

int Channel::rowCount() const
{
    return 0;
}

/****************************************************************************/

QVariant Channel::data(const QModelIndex &index, int role) const
{
    QVariant ret;

    switch (index.column()) {
        case 0:
            switch (role) {
                case Qt::DisplayRole:
                    ret = QString(ch->name().c_str());
                    break;
                case Qt::DecorationRole:
                    ret = QIcon(":/images/utilities-system-monitor.svg");
                    break;
            }
            break;
    }

    return ret;
}

/****************************************************************************/

void *Channel::child(int row) const
{
    Q_UNUSED(row);

    Node *ret = NULL;

    return ret;
}

/****************************************************************************/

int Channel::row(void *n) const
{
    Q_UNUSED(n)
    return 0;
}

/****************************************************************************/

Qt::ItemFlags Channel::flags() const
{
    return Qt::ItemIsDragEnabled | Qt::ItemIsSelectable;
}

/****************************************************************************/

bool Channel::range_before(
        const TimeRange &range1,
        const TimeRange &range2
        )
{
    return range1.start < range2.start;
}

/****************************************************************************/
