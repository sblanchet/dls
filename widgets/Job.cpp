/*****************************************************************************
 *
 * Copyright (C) 2009 - 2019  Florian Pose <fp@igh-essen.com>
 *
 * This file is part of the DLS widget library.
 *
 * The DLS widget library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the License,
 * or (at your option) any later version.
 *
 * The DLS widget library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with the DLS widget library. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 ****************************************************************************/

#include <QDebug>
#include <QUrl>
#include <QApplication>

#include <LibDLS/Job.h>

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

    for (std::list<LibDLS::Channel>::iterator ch = job->channels().begin();
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

QUrl Job::url() const
{
    QUrl u = parent()->url();
    QString path = u.path();
    path += QString("/job%1").arg(job->id());
    u.setPath(path);
    return u;
}

/****************************************************************************/

Channel *Job::findChannel(const QString &name)
{
    for (QList<Channel *>::iterator c = channels.begin();
            c != channels.end(); c++) {
        QString cName((*c)->name());
        if (name != cName) {
            continue;
        }

        return (*c);
    }

    return NULL;
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
                        QString text =
                            QApplication::translate("Job", "Job %1")
                            .arg(job->id());

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
