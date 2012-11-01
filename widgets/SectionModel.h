/*****************************************************************************
 *
 * $Id$
 *
 ****************************************************************************/

#include <QList>
#include <QString>
#include <QAbstractTableModel>

/*****************************************************************************/

namespace DLS {

class Section;

class SectionModel:
    public QAbstractTableModel
{
    public:
        SectionModel(Section *);
        ~SectionModel();

        int rowCount(const QModelIndex &) const;
        int columnCount(const QModelIndex &) const;
        QVariant data(const QModelIndex &, int) const;
        QVariant headerData(int, Qt::Orientation, int) const;
        Qt::ItemFlags flags(const QModelIndex &) const;
        bool setData(const QModelIndex &, const QVariant &, int);
        bool removeRows(int, int, const QModelIndex &);

    private:
        Section * const section;

        SectionModel();
};

} // namespace

/****************************************************************************/
