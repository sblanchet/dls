/*****************************************************************************
 *
 * $Id$
 *
 ****************************************************************************/

#include <QDebug>
#include <QStringList>
#include <QMimeData>
#include <QUrl>

#include "lib_dir.hpp"

#include "Model.h"
#include "Dir.h"
#include "Channel.h"

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

struct LocalChannel {
    QString dirPath;
    unsigned int jobId;
    QString channelName;
};

QtDls::Channel *Model::getChannel(QUrl url)
{
    if (!url.scheme().isEmpty() && url.scheme() != "file") {
        qWarning() << QString("URL scheme \"%1\" is not supported!")
            .arg(url.scheme());
        return NULL;
    }

    // using local file path
    QString path = url.path();

    /* the jobNNN component can show up multiple times in the URL path. To
     * determine, which one corresponds to the job directory, we must try, if
     * there is an existing dir for each one. If no existing directory was
     * found, we have to search again for each occurrence of jobNNN. */

    QList<LocalChannel> locList;
    QStringList comp = path.split('/');

    for (int i = 0; i < comp.size(); i++) {
        if (!comp[i].startsWith("job")) {
            continue;
        }
        QString rem = comp[i].mid(3);
        LocalChannel loc;
        bool ok;
        loc.jobId = rem.toUInt(&ok, 10);
        if (!ok) {
            continue;
        }
        QStringList dirPathComp = comp.mid(0, i);
        loc.dirPath = dirPathComp.join("/");
        QStringList channelNameComp = comp.mid(i + 1);
        loc.channelName = "/" + channelNameComp.join("/");
        locList.append(loc);
    }

    if (locList.empty()) {
        qWarning() << "Invalid URL:" << url;
        return NULL;
    }

    // try to find an existing local dir with matching path

    for (QList<LocalChannel>::iterator loc = locList.begin();
            loc != locList.end(); loc++) {
        for (QList<Dir *>::iterator d = dirs.begin(); d != dirs.end(); d++) {
            QString dirPath = (*d)->getDir()->path().c_str();
            if (loc->dirPath != dirPath) {
                continue;
            }

            QtDls::Channel *ch =
                (*d)->findChannel(loc->jobId, loc->channelName);
            if (ch) {
                return ch;
            }
        }
    }

    // try to create new dirs for every valid combination

    for (QList<LocalChannel>::iterator loc = locList.begin();
            loc != locList.end(); loc++) {
        LibDLS::Directory *d = new LibDLS::Directory();
        try {
            d->import(loc->dirPath.toUtf8().constData()); // FIXME enc?
        }
        catch (LibDLS::DirectoryException &e) {
            delete d;
            continue;
        }

        Dir *dir = new Dir(d);
        Channel *ch = dir->findChannel(loc->jobId, loc->channelName);

        if (!ch) {
            delete dir;
            continue;
        }

        beginInsertRows(QModelIndex(), dirs.count(), dirs.count());
        dirs.push_back(dir);
        endInsertRows();

        return ch;
    }

    return NULL;
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
    types << "text/uri-list";
    return types;
}

/*****************************************************************************/

QMimeData *Model::mimeData(const QModelIndexList &indexes) const
{
    QMimeData *mimeData = new QMimeData();
    QList<QUrl> urls;

    foreach (QModelIndex index, indexes) {
        if (index.isValid()) {
            Node *n = (Node *) index.internalPointer();
            Channel *c = dynamic_cast<Channel *>(n);
            urls.append(c->url());
        }
    }

    mimeData->setUrls(urls);
    return mimeData;
}

/*****************************************************************************/

Model::Exception::Exception(const QString &what):
    msg(what)
{
}

/****************************************************************************/
