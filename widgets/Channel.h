/*****************************************************************************
 *
 * $Id$
 *
 ****************************************************************************/

#include <QList>

#include "Node.h"

/*****************************************************************************/

namespace LibDLS {
    class Channel;
}

namespace QtDls {

class Channel:
   public Node
{
    public:
        Channel(Node *, const LibDLS::Channel *);
        ~Channel();


        class Exception
        {
            public:
                Exception(const QString &);
                QString msg;
        };

        int rowCount() const;
        QVariant data(const QModelIndex &, int) const;
        void *child(int) const;
        int row(void *) const;
        Qt::ItemFlags flags() const;

    private:
        const LibDLS::Channel * const channel;

        Channel();
};

} // namespace

/****************************************************************************/
