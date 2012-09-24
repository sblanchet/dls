/*****************************************************************************
 *
 * $Id$
 *
 ****************************************************************************/

#include <QDebug>

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
