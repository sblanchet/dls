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
#include <QSettings>
#include <QCloseEvent>

#include "MainWindow.h"
#include "SettingsDialog.h"

#define MODELTEST 0

#if MODELTEST
#include "modeltest.h"
#endif

/****************************************************************************/

MainWindow::MainWindow(QWidget *parent):
    QMainWindow(parent)
{
    setupUi(this);

    QSettings settings;
    restore = settings.value("RestoreOnStartup", true).toBool();
    recentFiles = settings.value("RecentFiles").toStringList();
    if (settings.contains("WindowHeight") &&
            settings.contains("WindowWidth")) {
        resize(settings.value("WindowWidth").toInt(),
                settings.value("WindowHeight").toInt());
    }
    if (settings.value("WindowMaximized", false).toBool()) {
        setWindowState(windowState() | Qt::WindowMaximized);
    }
    if (settings.contains("SplitterSizes")) {
        QVariantList vl(settings.value("SplitterSizes").toList());
        QList<int> list;
        list << vl[0].toInt() << vl[1].toInt();
        splitter->setSizes(list);
    }

    for (int i = 0; i < MaxRecentFiles; ++i) {
        recentFileActions[i] = new QAction(this);
        recentFileActions[i]->setVisible(false);
        connect(recentFileActions[i], SIGNAL(triggered()),
                this, SLOT(openRecentFile()));
        menuRecentFiles->addAction(recentFileActions[i]);
    }

    updateRecentFileActions();

#if MODELTEST
    new ModelTest(&model);
#endif

    dlsGraph->setDropModel(&model);

    treeView->setModel(&model);

    if (restore && recentFiles.size() > 0) {
        QString path = recentFiles.front();

        if (dlsGraph->load(path, &model)) {
            currentFileName = path;
        }
        else {
            qWarning() << "failed to load" << path;
        }
    }
}

/****************************************************************************/

MainWindow::~MainWindow()
{
    model.clear();

    for (QList<LibDLS::Directory *>::iterator dir = dirs.begin();
            dir != dirs.end(); dir++) {
        delete *dir;
    }
}

/****************************************************************************/

void MainWindow::closeEvent(QCloseEvent *event)
{
    QSettings settings;

    settings.setValue("RestoreOnStartup", restore);
    settings.setValue("RecentFiles", recentFiles);
    settings.setValue("WindowWidth", width());
    settings.setValue("WindowHeight", height());
    settings.setValue("WindowMaximized", isMaximized());
    QVariantList list;
    list << splitter->sizes()[0] << splitter->sizes()[1];
    settings.setValue("SplitterSizes", list);

    event->accept();
}

/****************************************************************************/

void MainWindow::addRecentFile(const QString &path)
{
    recentFiles.removeAll(path);
    recentFiles.prepend(path);
    updateRecentFileActions();
}

/****************************************************************************/

void MainWindow::updateRecentFileActions()
{
    int numRecentFiles = qMin(recentFiles.size(), (int) MaxRecentFiles);

    for (int i = 0; i < numRecentFiles; ++i) {
        QString text = tr("&%1 %2").arg(i + 1).arg(recentFiles[i]);
        recentFileActions[i]->setText(text);
        recentFileActions[i]->setData(recentFiles[i]);
        recentFileActions[i]->setVisible(true);
    }

    for (int j = numRecentFiles; j < MaxRecentFiles; ++j) {
        recentFileActions[j]->setVisible(false);
    }

    menuRecentFiles->setEnabled(numRecentFiles > 0);
}

/****************************************************************************/

void MainWindow::on_actionLoad_triggered()
{
    QFileDialog dialog(this);

    QString path = dialog.getOpenFileName(this, tr("Open view"));

    if (path.isEmpty()) {
        return;
    }

    if (dlsGraph->load(path, &model)) {
        currentFileName = path;
        addRecentFile(currentFileName);
    }
}

/****************************************************************************/

void MainWindow::openRecentFile()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (!action) {
        return;
    }

    QString path = action->data().toString();

    if (dlsGraph->load(path, &model)) {
        currentFileName = path;
        addRecentFile(currentFileName);
    }
}

/****************************************************************************/

void MainWindow::on_actionSave_triggered()
{
    QString path;

    if (currentFileName.isEmpty()) {
        QFileDialog dialog(this);

        path = dialog.getSaveFileName(this, tr("Save view"));

        if (path.isEmpty()) {
            return;
        }
    }
    else {
        path = currentFileName;
    }

    if (dlsGraph->save(path)) {
        currentFileName = path;
        addRecentFile(currentFileName);
    }
}

/****************************************************************************/

void MainWindow::on_actionSaveAs_triggered()
{
    QFileDialog dialog(this);

    QString path = dialog.getSaveFileName(this, tr("Save view"));

    if (path.isEmpty()) {
        return;
    }

    if (dlsGraph->save(path)) {
        currentFileName = path;
        addRecentFile(currentFileName);
    }
}

/****************************************************************************/

void MainWindow::on_actionSettings_triggered()
{
    SettingsDialog dialog(restore, this);

    dialog.exec();
}

/****************************************************************************/

void MainWindow::on_toolButtonNewDir_clicked()
{
    QFileDialog dialog(this);

    QString path = dialog.getExistingDirectory(this, tr("Open data diretory"),
            "/vol/data/dls_data");

    if (path.isEmpty()) {
        return;
    }

    LibDLS::Directory *dir = new LibDLS::Directory();

    try {
        dir->import(path.toLocal8Bit().constData());
        model.addLocalDir(dir);
    } catch (LibDLS::DirectoryException &e) {
        qWarning() << e.msg.c_str();
        delete dir;
    }
}

/****************************************************************************/
