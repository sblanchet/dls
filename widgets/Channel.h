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
        Channel(Node *, LibDLS::Channel *);
        ~Channel();

        QUrl url() const;

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

        LibDLS::Channel *channel() const;

    private:
        LibDLS::Channel * const ch;

        Channel();
};

} // namespace

/****************************************************************************/
