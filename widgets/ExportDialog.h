/*****************************************************************************
 *
 * $Id$
 *
 * Copyright (C) 2009 - 2013  Florian Pose <fp@igh-essen.com>
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

#ifndef DLS_EXPORT_DIALOG_H
#define DLS_EXPORT_DIALOG_H

#include <QDialog>
#include <QSet>
#include <QDir>

#include "com_time.hpp"

#include "ui_ExportDialog.h"

class QThread;

namespace LibDLS {
    class Data;
    class Export;
}

namespace QtDls {
    class Channel;
}

namespace DLS {

class Graph;
class ExportDialog;

/****************************************************************************/

/** Export working class hero.
 */
class ExportWorker:
    public QObject
{
    Q_OBJECT

    public:
        ExportWorker(QSet<QtDls::Channel *>, COMTime, COMTime);
        ~ExportWorker();

        void setDirectory(QDir d) { dir = d; }
        void setDecimation(unsigned int d) { decimation = d; }
        void addExporter(LibDLS::Export *);

        double progress() const { return totalProgress; }

    public slots:
        void doWork();

    signals:
        void updateProgress();
        void finished();

    private:
        COMTime start;
        COMTime end;
        unsigned int decimation;
        QSet<QtDls::Channel *> channels;
        double totalProgress;
        double channelProgress;
        QList<LibDLS::Export *> exporters;
        QDir dir;

        static int dataCallback(LibDLS::Data *, void *);
        void newData(LibDLS::Data *);
};

/****************************************************************************/

/** Graph data export dialog.
 */
class ExportDialog:
    public QDialog,
    public Ui::ExportDialog
{
    Q_OBJECT

    public:
        ExportDialog(Graph *, QThread *, QSet<QtDls::Channel *>);
        ~ExportDialog();

    private:
        Graph * const graph;
        ExportWorker worker;
        QDir dir;
        COMTime now;

        ExportDialog();

    private slots:
        void accept();
        void reject();
        void updateProgress();
        void workerFinished();
        void on_pushButtonDir_clicked();
};

/****************************************************************************/

} // namespace

#endif
