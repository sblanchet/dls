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
#include <QMessageBox>
#include <QUrl>

#include "MainWindow.h"
#include "SettingsDialog.h"

#define MODELTEST 0

#if MODELTEST
#include "modeltest.h"
#endif

/****************************************************************************/

MainWindow::MainWindow(QWidget *parent):
    QMainWindow(parent),
    scriptActions(NULL),
    scriptProcess(this)
{
    setupUi(this);

    connect(dlsGraph, SIGNAL(logMessage(const QString &)),
            this, SLOT(loggingCallback(const QString &)));
    connect(&scriptProcess, SIGNAL(finished(int, QProcess::ExitStatus)),
            this, SLOT(scriptFinished(int, QProcess::ExitStatus)));
    connect(&scriptProcess, SIGNAL(error(QProcess::ProcessError)),
            this, SLOT(scriptError(QProcess::ProcessError)));

    QSettings settings;
    restore = settings.value("RestoreOnStartup", true).toBool();
    recentFiles = settings.value("RecentFiles").toStringList();
    scripts = settings.value("Scripts").toStringList();
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

    if (scripts.size() > 0) {
        scriptActions = new QAction *[scripts.size()];

        for (int i = 0; i < scripts.size(); ++i) {
            scriptActions[i] = new QAction(this);
            scriptActions[i]->setText(scripts[i]);
            scriptActions[i]->setData(scripts[i]);
            connect(scriptActions[i], SIGNAL(triggered()),
                    this, SLOT(execScript()));
            menuScripts->addAction(scriptActions[i]);
        }
    }

    menuScripts->menuAction()->setVisible(scripts.size() > 0);
    updateScriptActions();

#if MODELTEST
    new ModelTest(&model);
#endif

    dlsGraph->setDropModel(&model);

    treeView->setModel(&model);

    if (restore && recentFiles.size() > 0) {
        QString path = recentFiles.front();

        if (dlsGraph->load(path, &model)) {
            currentFileName = path;
            setWindowFilePath(currentFileName);
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

    delete [] scriptActions;
}

/********************** HACK: QTBUG-16507 workaround ************************/

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    QString filePath = windowFilePath();
    if (!filePath.isEmpty()) {
        setWindowFilePath(filePath + "x");
        setWindowFilePath(filePath);
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

    logWindow.close();
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

void MainWindow::updateScriptActions()
{
    bool enabled = scriptProcess.state() == QProcess::NotRunning;

    for (int i = 0; i < scripts.size(); ++i) {
        scriptActions[i]->setEnabled(enabled);
    }
}

/****************************************************************************/

QStringList MainWindow::viewFilters()
{
    QStringList filters;
    filters << tr("DLS Views (*.dlsv)");
    filters << tr("XML files (*.xml)");
    filters << tr("All files (*.*)");
    return filters;
}

/****************************************************************************/

void MainWindow::on_actionNew_triggered()
{
    currentFileName = "";
    setWindowFilePath(currentFileName);
    dlsGraph->clearSections();
}

/****************************************************************************/

void MainWindow::on_actionLoad_triggered()
{
    QFileDialog dialog(this);
    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setDefaultSuffix("dlsv");
    dialog.setNameFilters(viewFilters());
    dialog.selectFile(currentFileName);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    QString path = dialog.selectedFiles()[0];

    if (path.isEmpty()) {
        return;
    }

    if (dlsGraph->load(path, &model)) {
        currentFileName = path;
        setWindowFilePath(currentFileName);
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
        setWindowFilePath(currentFileName);
        addRecentFile(currentFileName);
    }
}

/****************************************************************************/

void MainWindow::on_actionSave_triggered()
{
    QString path;

    if (currentFileName.isEmpty()) {
        QFileDialog dialog(this);
        dialog.setAcceptMode(QFileDialog::AcceptSave);
        dialog.setFileMode(QFileDialog::AnyFile);
        dialog.setDefaultSuffix("dlsv");
        dialog.setNameFilters(viewFilters());

        if (dialog.exec() != QDialog::Accepted) {
            return;
        }

        path = dialog.selectedFiles()[0];
    }
    else {
        path = currentFileName;
    }

    if (dlsGraph->save(path)) {
        currentFileName = path;
        setWindowFilePath(currentFileName);
        addRecentFile(currentFileName);
    }
}

/****************************************************************************/

void MainWindow::on_actionSaveAs_triggered()
{
    QFileDialog dialog(this);
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setDefaultSuffix("dlsv");
    dialog.setNameFilters(viewFilters());
    dialog.selectFile(currentFileName);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    QString path = dialog.selectedFiles()[0];

    if (path.isEmpty()) {
        return;
    }

    if (dlsGraph->save(path)) {
        currentFileName = path;
        setWindowFilePath(currentFileName);
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

void MainWindow::on_actionLogWindow_triggered()
{
    logWindow.show();
}

/****************************************************************************/

void MainWindow::on_toolButtonNewDir_clicked()
{
    QString path = QFileDialog::getExistingDirectory(this,
            tr("Open data diretory"), "/vol/data/dls_data");

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

void MainWindow::loggingCallback(const QString &msg)
{
    logWindow.log(msg);
}

/****************************************************************************/

void MainWindow::execScript()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (!action) {
        return;
    }

    QString path = action->data().toString();
    scriptProcess.start(path);

    if (!scriptProcess.waitForStarted()) {
        return;
    }

    QString yaml;

    yaml += QString("---\nstart: %1\nend: %2i\nchannels:\n")
        .arg(dlsGraph->getStart().to_real_time().c_str())
        .arg(dlsGraph->getEnd().to_real_time().c_str());

    QSet<QUrl> urls = dlsGraph->urls();

    for (QSet<QUrl>::const_iterator u = urls.begin();
            u != urls.end(); u++) {
        yaml += QString("   - url: %1\n").arg(u->toString());
    }

    scriptProcess.write(yaml.toLocal8Bit());
    scriptProcess.closeWriteChannel();

    updateScriptActions();
}

/****************************************************************************/

void MainWindow::scriptFinished(
        int exitCode,
        QProcess::ExitStatus exitStatus
        )
{
#if 0
    QByteArray result = scriptProcess.readAll();
    qDebug() << "finished" << exitCode << exitStatus << result;
#else
    Q_UNUSED(exitCode);
    Q_UNUSED(exitStatus);
#endif

    updateScriptActions();
}

/****************************************************************************/

void MainWindow::scriptError(
        QProcess::ProcessError error
        )
{
    QString msg;

    switch (error) {
        case QProcess::FailedToStart:
            msg = tr("Failed to start process.");
            break;
        case QProcess::Crashed:
            msg = tr("Script crashed.");
            break;
        case QProcess::Timedout:
            msg = tr("Script timed out.");
            break;
        case QProcess::WriteError:
            msg = tr("Failed to write to script.");
            break;
        case QProcess::ReadError:
            msg = tr("Failed to read from script.");
            break;
        default:
            msg = tr("Unknown error.");
            break;
    }

    QMessageBox::critical(this, tr("Script"),
            tr("Failed to execute script: %1").arg(msg));

    updateScriptActions();
}

/****************************************************************************/
