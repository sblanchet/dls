/*****************************************************************************
 *
 * $Id$
 *
 ****************************************************************************/

#include <QList>
#include <QString>
#include <QAbstractItemModel>

/*****************************************************************************/

namespace LibDLS {
    class Directory;
}

namespace QtDls {

class Dir;

class Model:
    public QAbstractItemModel
{
    public:
        Model();
        ~Model();

        void addLocalDir(LibDLS::Directory *);
        void clear();

        class Exception
        {
            public:
                Exception(const QString &);
                QString msg;
        };

        // from QAbstractItemModel
        int rowCount(const QModelIndex &) const;
        int columnCount(const QModelIndex &) const;
        QModelIndex index(int, int, const QModelIndex &) const;
        QModelIndex parent(const QModelIndex &) const;
        QVariant data(const QModelIndex &, int) const;
        QVariant headerData(int, Qt::Orientation, int) const;
        Qt::ItemFlags flags(const QModelIndex &) const;
        QStringList mimeTypes() const;
        QMimeData *mimeData(const QModelIndexList &) const;

    private:
        QList<Dir *> dirs;
};

} // namespace

/****************************************************************************/
