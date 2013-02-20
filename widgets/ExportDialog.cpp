/*****************************************************************************
 *
 * $Id$
 *
 * Copyright (C) 2012-2013  Florian Pose <fp@igh-essen.com>
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

#include "ExportDialog.h"
#include "Graph.h"

using DLS::ExportDialog;

/****************************************************************************/

/** Constructor.
 */
ExportDialog::ExportDialog(
        Graph *graph
        ):
    QDialog(graph),
    graph(graph)
{
    setupUi(this);

    QString num;
    num.setNum(graph->signalCount());
    labelNumber->setText(num);

    labelBegin->setText(graph->getStart().to_real_time().c_str());
    labelEnd->setText(graph->getEnd().to_real_time().c_str());

    labelDuration->setText(
            graph->getStart().diff_str_to(graph->getEnd()).c_str());
}

/****************************************************************************/

/** Destructor.
 */
ExportDialog::~ExportDialog()
{
}

/****************************************************************************/

void ExportDialog::accept()
{
    done(Accepted);
}

/****************************************************************************/

void ExportDialog::reject()
{
    done(Rejected);
}

/****************************************************************************/
