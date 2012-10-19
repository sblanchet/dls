/*****************************************************************************
 *
 * $Id$
 *
 ****************************************************************************/

#include <QDebug>
#include <QStringList>
#include <QMimeData>

#include "Model.h"
#include "Dir.h"

using namespace QtDls;

/*****************************************************************************/

Model::Model()
{
}

/****************************************************************************/

Model::~Model()
{
    clear();
}

/****************************************************************************/

void Model::addLocalDir(
        LibDLS::Directory *d
        )
{
    Dir *dir = new Dir(d);
    beginInsertRows(QModelIndex(), dirs.count(), dirs.count());
    dirs.push_back(dir);
    endInsertRows();
}

/****************************************************************************/

void Model::clear()
{
    if (dirs.empty()) {
        return;
    }

    beginRemoveRows(QModelIndex(), 0, dirs.count() - 1);

    while (!dirs.empty()) {
        delete dirs.front();
        dirs.pop_front();
    }

    endRemoveRows();
}

/****************************************************************************/

/** Implements the model interface.
 */
int Model::rowCount(const QModelIndex &index) const
{
    int ret = 0;

    if (index.isValid()) {
        if (index.column() == 0 && index.internalPointer()) {
            Node *n = (Node *) index.internalPointer();
            ret = n->rowCount();
        }
    } else {
        ret = dirs.count();
    }

    return ret;
}

/****************************************************************************/

/** Implements the model interface.
 *
 * \returns Number of columns.
 */
int Model::columnCount(const QModelIndex &index) const
{
    Q_UNUSED(index);
    return 1;
}

/****************************************************************************/

/** Implements the Model interface.
 */
QModelIndex Model::index(int row, int col, const QModelIndex &parent) const
{
    QModelIndex ret;

    if (row < 0 || col < 0) {
        return ret;
    }

    if (parent.isValid()) {
        Node *n = (Node *) parent.internalPointer();
        ret = createIndex(row, col, n->child(row));
    } else {
        if (row < dirs.count()) {
            ret = createIndex(row, col, dirs[row]);
        }
    }

    return ret;
}

/****************************************************************************/

/** Implements the Model interface.
 */
QModelIndex Model::parent(const QModelIndex &index) const
{
    QModelIndex ret;

    if (index.isValid()) {
        Node *n = (Node *) index.internalPointer();
        Node *p = n->parent();
        if (p) {
            int row;
            Node *pp = p->parent(); // grandparent to get parent row
            if (pp) {
                row = pp->row(p);
            } else {
                Dir *d = dynamic_cast<Dir *>(p);
                row = dirs.indexOf(d);
            }
            ret = createIndex(row, 0, p);
        }
    }

    return ret;
}

/****************************************************************************/

/** Implements the Model interface.
 */
QVariant Model::data(const QModelIndex &index, int role) const
{
    QVariant ret;

    if (index.isValid()) {
        Node *n = (Node *) index.internalPointer();
        ret = n->data(index, role);
    }

    return ret;
}

/****************************************************************************/

/** Implements the Model interface.
 */
QVariant Model::headerData(
        int section,
        Qt::Orientation o,
        int role
        ) const
{
    Q_UNUSED(section);
    Q_UNUSED(o);
    Q_UNUSED(role);
    return QVariant();
}

/****************************************************************************/

/** Implements the Model interface.
 */
Qt::ItemFlags Model::flags(const QModelIndex &index) const
{
    Qt::ItemFlags f;

    if (index.isValid()) {
        f |= Qt::ItemIsEnabled;
        Node *n = (Node *) index.internalPointer();
        f |= n->flags();
    }

    return f;
}

/*****************************************************************************/

QStringList Model::mimeTypes() const
{
    QStringList types;
    types << "application/dls_channel";
    return types;
}

/*****************************************************************************/

QMimeData *Model::mimeData(const QModelIndexList &indexes) const
{
    QMimeData *mimeData = new QMimeData();
    QByteArray encodedData;

    QDataStream stream(&encodedData, QIODevice::WriteOnly);

    /* FIXME storing pointers in MIME data! Come on, you can do better! */
    foreach (QModelIndex index, indexes) {
        if (index.isValid()) {
            Node *n = (Node *) index.internalPointer();
            stream << (quint64) n->channel();
        }
    }

    mimeData->setData("application/dls_channel", encodedData);
    return mimeData;
}

/*****************************************************************************/

Model::Exception::Exception(const QString &what):
    msg(what)
{
}

/****************************************************************************/
