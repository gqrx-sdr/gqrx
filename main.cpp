/* -*- c++ -*- */
/*
 * Copyright 2011 Alexandru Csete OZ9AEC.
 *
 * Gqrx is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * Gqrx is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Gqrx; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */
#include <QtGui/QApplication>
#include <QSettings>
#include <QMessageBox>
#include <QDebug>
#include "qtgui/ioconfig.h"
#include "mainwindow.h"
#include "gqrx.h"


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QCoreApplication::setOrganizationName(GQRX_ORG_NAME);
    QCoreApplication::setOrganizationDomain(GQRX_ORG_DOMAIN);
    QCoreApplication::setApplicationName(GQRX_APP_NAME);
#ifdef QT_NO_DEBUG
    QCoreApplication::setApplicationVersion(QString("%1.%2.%3").
                                            arg(GQRX_VERSION_MAJOR).
                                            arg(GQRX_VERSION_MINOR).
                                            arg(GQRX_VERSION_MICRO));
#else
    QCoreApplication::setApplicationVersion(VERSION);
#endif

    //QSettings test("hello-test.conf", QSettings::IniFormat);
    //test.setValue("system/testField", 5);

    // Check that we have an input device configured
    //QSettings settings(QSettings::IniFormat, QSettings::UserScope,
    //                   GQRX_ORG_NAME, GQRX_APP_NAME);

    /*
    if (!settings.contains("Input/type")) {

        CIoConfig *ioconf = new CIoConfig();
        int confres = ioconf->exec();

        delete ioconf;

        if (confres == QDialog::Rejected) {
            qDebug() << "I/O device configuration cancelled.";

            return 0;
        }
    }
    */

    QString indev = CIoConfig::getFcdDeviceName();
    if (indev.isEmpty())
    {
        QMessageBox::critical(NULL, "Gqrx error",
                              "<b>Funcube Dongle could not be found</b><br><br>"
                              "Please ensure that the Funcube Dongle is plugged in "
                              "and properly configured with the latest firmware, "
                              "then try again.",
                              QMessageBox::Close);

        return 0;
    }

    // We should now have at least an input device configured
    // and MainWindow will pick that up.
    MainWindow w;
    w.show();

    return a.exec();
}
