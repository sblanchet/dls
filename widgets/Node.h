/*****************************************************************************
 *
 * $Id$
 *
 ****************************************************************************/

#ifndef DLS_NODE_H
#define DLS_NODE_H

#include <QModelIndex>

/*****************************************************************************/

namespace LibDLS {
    class Channel;
}

namespace QtDls {

class Node
{
    public:
        Node(Node *);
        virtual ~Node();

        virtual int rowCount() const = 0;
        virtual QVariant data(const QModelIndex &, int) const = 0;
        virtual void *child(int) const = 0;
        virtual int row(void *) const = 0;
        virtual Qt::ItemFlags flags() const;
        virtual LibDLS::Channel *channel() const;

        Node *parent() const;

    private:
        Node *const parentNode;

        Node();
};

} // namespace

#endif // DLS_NODE_H

/****************************************************************************/
