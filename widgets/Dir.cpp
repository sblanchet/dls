/*****************************************************************************
 *
 * $Id$
 *
 ****************************************************************************/

#include <list>

using namespace std;

#include <QDebug>

#include "lib_dir.hpp"

#include "Dir.h"
#include "Job.h"

using namespace QtDls;

/*****************************************************************************/

Dir::Dir(
        LibDLS::Directory *dir
        ):
    Node(NULL),
    dir(dir)
{
    for (list<LibDLS::Job>::iterator j = dir->jobs().begin();
            j != dir->jobs().end(); j++) {
        Job *job = new Job(this, &*j);
        jobs.push_back(job);
    }
}

/****************************************************************************/

Dir::~Dir()
{
}

/****************************************************************************/

int Dir::rowCount() const
{
    return jobs.size();
}

/****************************************************************************/

QVariant Dir::data(const QModelIndex &index, int role) const
{
    QVariant ret;

    switch (index.column()) {
        case 0:
            switch (role) {
                case Qt::DisplayRole:
                    ret = QString("Local directory %1")
                        .arg(dir->path().c_str());
                    break;
            }
            break;
    }

    return ret;
}

/****************************************************************************/

void *Dir::child(int row) const
{
    Node *ret = NULL;

    if (row >= 0 && row < jobs.size()) {
        ret = jobs[row];
    }

    return ret;
}

/****************************************************************************/

int Dir::row(void *n) const
{
    return jobs.indexOf((Job *) n);
}

/****************************************************************************/
