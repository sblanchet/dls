/*****************************************************************************
 *
 * $Id$
 *
 ****************************************************************************/

#include <QList>

#include "Node.h"

/*****************************************************************************/

namespace LibDLS {
    class Job;
}

namespace QtDls {

class Channel;

class Job:
   public Node
{
    public:
        Job(Node *, LibDLS::Job *);
        ~Job();

        QUrl url() const;
        Channel *findChannel(const QString &);

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

        LibDLS::Job *getJob() const { return job; }

    private:
        LibDLS::Job * const job;
        QList<Channel *> channels;

        Job();
};

} // namespace

/****************************************************************************/
