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

class Dir:
   public Node
{
    public:
        Dir(LibDLS::Directory *);
        ~Dir();


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

    private:
        LibDLS::Directory * const dir;
        QList<Job *> jobs;

        Dir();
};

} // namespace

/****************************************************************************/
