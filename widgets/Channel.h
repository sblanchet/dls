/*****************************************************************************
 *
 * $Id$
 *
 ****************************************************************************/

#include <QList>
#include <QMutex>

#include "lib_channel.hpp"

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
        QString name() const;

        class Exception
        {
            public:
                Exception(const QString &);
                QString msg;
        };

        void fetchData(COMTime, COMTime, unsigned int, LibDLS::DataCallback,
                void *);

        struct TimeRange
        {
            COMTime start;
            COMTime end;
        };
        vector<TimeRange> chunkRanges();
        void getRange(COMTime &, COMTime &);

        int rowCount() const;
        QVariant data(const QModelIndex &, int) const;
        void *child(int) const;
        int row(void *) const;
        Qt::ItemFlags flags() const;

    private:
        LibDLS::Channel * const ch;
        QMutex mutex;

        static bool range_before(const TimeRange &, const TimeRange &);

        Channel();
};

} // namespace

/****************************************************************************/
