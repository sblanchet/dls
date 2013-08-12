/*****************************************************************************
 *
 * $Id$
 *
 * Copyright (C) 2012  Florian Pose <fp@igh-essen.com>
 *
 * This file is part of the data logging service (DLS).
 *
 * The DLS is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * The DLS is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with the DLS. If not, see <http://www.gnu.org/licenses/>.
 *
 ****************************************************************************/

#include <QMainWindow>

#include "ui_MainWindow.h"
#include "LogWindow.h"

#include "lib_dir.hpp"

#include "DlsWidgets/Model.h"

/****************************************************************************/

class MainWindow:
    public QMainWindow,
    public Ui::MainWindow
{
    Q_OBJECT

    public:
        MainWindow(QWidget * = 0);
        ~MainWindow();

    private:
        QList<LibDLS::Directory *> dirs;
        QtDls::Model model;
        bool restore;
        QStringList recentFiles;
        QString currentFileName;
        LogWindow logWindow;

        enum { MaxRecentFiles = 10 };
        QAction *recentFileActions[MaxRecentFiles];

        void showEvent(QShowEvent *);
        void closeEvent(QCloseEvent *);
        void addRecentFile(const QString &);
        void updateRecentFileActions();


    private slots:
        void on_actionLoad_triggered();
        void openRecentFile();
        void on_actionSave_triggered();
        void on_actionSaveAs_triggered();
        void on_actionSettings_triggered();
        void on_actionLogWindow_triggered();

        void on_toolButtonNewDir_clicked();

        void loggingCallback(const QString &);
};

/****************************************************************************/
