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
#include <QFileDialog>

#include "MainWindow.h"
#include "modeltest.h"

/****************************************************************************/

MainWindow::MainWindow(QWidget *parent):
    QMainWindow(parent)
{
    setupUi(this);

    new ModelTest(&model);

    try {
        dir.import("/vol/projekte/airbus/a4q_airbus-4q-technologie"
                "/messungen/dls_data");
        model.addLocalDir(&dir);
    } catch (LibDLS::DirectoryException &e) {
        qWarning() << e.msg.c_str();
    }

    try {
        dir2.import("/vol/projekte/airbus/amb_airbus-messbolzen/messungen"
                "/kalibrierung/dls-data");
        model.addLocalDir(&dir2);
    } catch (LibDLS::DirectoryException &e) {
        qWarning() << e.msg.c_str();
    }

    treeView->setModel(&model);
}

/****************************************************************************/

MainWindow::~MainWindow()
{
}

/****************************************************************************/

void MainWindow::on_toolButtonView1_clicked()
{
    COMTime start, end;
    start.set_date(2012, 8, 28, 2, 34);
    end.set_date(2012, 8, 28, 2, 35, 10);
    dlsGraph->setRange(start, end);
}

/****************************************************************************/

void MainWindow::on_toolButtonNewDir_clicked()
{
    QFileDialog dialog(this);

    QString dir = dialog.getExistingDirectory(this, tr("Open data diretory"),
            "/vol/data/dls_data");

    if (dir.isEmpty()) {
        return;
    }

    try {
        dir3.import(dir.toLocal8Bit().constData());
        model.addLocalDir(&dir3);
    } catch (LibDLS::DirectoryException &e) {
        qWarning() << e.msg.c_str();
    }
}

/****************************************************************************/
