/*****************************************************************************
 *
 * $Id$
 *
 ****************************************************************************/

#include <QDebug>

#include "lib_job.hpp"

#include "Job.h"
#include "Channel.h"

using namespace QtDls;

/*****************************************************************************/

Job::Job(
        Node *parent,
        LibDLS::Job *job
        ):
    Node(parent),
    job(job)
{
    job->fetch_channels();

    for (list<LibDLS::Channel>::iterator ch = job->channels().begin();
            ch != job->channels().end(); ch++) {
        Channel *c = new Channel(this, &*ch);
        channels.push_back(c);
    }
}

/****************************************************************************/

Job::~Job()
{
}

/****************************************************************************/

int Job::rowCount() const
{
    return channels.size();
}

/****************************************************************************/

QVariant Job::data(const QModelIndex &index, int role) const
{
    QVariant ret;

    switch (index.column()) {
        case 0:
            switch (role) {
                case Qt::DisplayRole: {
                        QString text = QString("Job %1").arg(job->id());

                        QString desc(job->preset().description().c_str());
                        if (!desc.isEmpty()) {
                            text += ", \"" + desc + "\"";
                        }

                        ret = text;
                    }
                    break;
            }
            break;
    }

    return ret;
}

/****************************************************************************/

void *Job::child(int row) const
{
    Q_UNUSED(row);

    Node *ret = NULL;

    if (row >= 0 && row < channels.size()) {
        ret = channels[row];
    }

    return ret;
}

/****************************************************************************/

int Job::row(void *n) const
{
    return channels.indexOf((Channel *) n);
}

/****************************************************************************/
