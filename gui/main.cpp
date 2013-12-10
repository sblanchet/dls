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

#include <QApplication>
#include <QLibraryInfo>
#include <QTranslator>
#include <QDebug>

#include "DlsWidgets/Translator.h"

#include "MainWindow.h"

/****************************************************************************/

int main(int argc, char *argv[])
{
    QApplication::setStyle("plastique");

    QCoreApplication::setOrganizationName("EtherLab");
    QCoreApplication::setOrganizationDomain("etherlab.org");
    QCoreApplication::setApplicationName("dlsgui");

    QApplication app(argc, argv);

    // load Qt's own translations
    QTranslator qtTranslator;
    qtTranslator.load("qt_" + QLocale::system().name(),
            QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    app.installTranslator(&qtTranslator);

    DLS::installTranslator();
    DLS::loadTranslation(QLocale::system().name());

    QTranslator translator;
    translator.load(":/.qm/locale/dlsgui_" + QLocale::system().name());
    app.installTranslator(&translator);

    MainWindow mainWin;
    mainWin.show();

    try {
        return app.exec();
    }
    catch(QtDls::Model::Exception &e) {
        qCritical() << "Model exception: " << e.msg;
        return 1;
    }
}

/****************************************************************************/
