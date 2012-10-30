/*****************************************************************************
 *
 * $Id$
 *
 ****************************************************************************/

#include <QDebug>
#include <QUrl>
#include <QIcon>

#include "Channel.h"

#include "lib_channel.hpp"

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

LibDLS::Channel *Channel::channel() const
{
    return ch;
}

/****************************************************************************/
