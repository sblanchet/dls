/*****************************************************************************
 *
 * $Id$
 *
 ****************************************************************************/

#include <QList>
#include <QReadWriteLock>

#include "lib_channel.hpp"

#include "Node.h"

/*****************************************************************************/

namespace LibDLS {
    class Channel;
    class Export;
}

namespace QtDls {

class Channel:
   public Node
{
    public:
        Channel(Node *, LibDLS::Channel *);
        ~Channel();

        QUrl url() const;
        QString name() const;
        LibDLS::Job *job() const { return ch->getJob(); }

        class Exception
        {
            public:
                Exception(const QString &);
                QString msg;
        };

        void fetchData(COMTime, COMTime, unsigned int, LibDLS::DataCallback,
                void *, unsigned int);
        bool beginExport(LibDLS::Export *, const QString &);

        struct TimeRange
        {
            COMTime start;
            COMTime end;
        };
        vector<TimeRange> chunkRanges();
        bool getRange(COMTime &, COMTime &);

        int rowCount() const;
        QVariant data(const QModelIndex &, int) const;
        void *child(int) const;
        int row(void *) const;
        Qt::ItemFlags flags() const;

    private:
        LibDLS::Channel * const ch;
        QReadWriteLock rwlock;
        vector<TimeRange> lastRanges;

        static bool range_before(const TimeRange &, const TimeRange &);

        Channel();
};

} // namespace

/****************************************************************************/
