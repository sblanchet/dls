/*****************************************************************************
 *
 * $Id$
 *
 ****************************************************************************/

#include <QList>

#include "Node.h"

/*****************************************************************************/

namespace LibDLS {
    class Directory;
}

namespace QtDls {

class Job;
class Channel;

class Dir:
   public Node
{
    public:
        Dir(LibDLS::Directory *);
        ~Dir();

        QUrl url() const;
        Channel *findChannel(unsigned int, const QString &);

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

        LibDLS::Directory *getDir() const { return dir; }

    private:
        LibDLS::Directory * const dir;
        QList<Job *> jobs;

        Dir();
};

} // namespace

/****************************************************************************/
