/*****************************************************************************
 *
 * $Id$
 *
 * Copyright (C) 2012  Florian Pose <fp@igh-essen.com>
 *
 * This file is part of the data logging service (DLS).
 *
 * DLS is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * DLS is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with DLS. If not, see <http://www.gnu.org/licenses/>.
 *
 ****************************************************************************/

#include <iostream>
using namespace std;

#include <QDebug>

#include "MainWindow.h"
#include "modeltest.h"

/****************************************************************************/

MainWindow::MainWindow(QWidget *parent):
    QMainWindow(parent)
{
    setupUi(this);

    new ModelTest(&model);

    dir.import("/vol/projekte/airbus/a4q_airbus-4q-technologie"
            "/messungen/dls_data");
    dir2.import("/vol/projekte/airbus/amb_airbus-messbolzen/messungen"
            "/kalibrierung/dls-data");

    model.addLocalDir(&dir);
    model.addLocalDir(&dir2);

    treeView->setModel(&model);
}

/****************************************************************************/

MainWindow::~MainWindow()
{
}

/****************************************************************************/