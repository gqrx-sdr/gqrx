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
#include <QtGlobal>
#include <QSettings>
#include <QFile>
#include <QStringList>
#include <QDebug>
#include "pulseaudio/pa_device_list.h"
#include "qtgui/ioconfig.h"
#include "ui_ioconfig.h"



CIoConfig::CIoConfig(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CIoConfig)
{
    QSettings settings;

    ui->setupUi(this);

    connect(this, SIGNAL(accepted()), this, SLOT(saveConfig()));


#ifdef Q_OS_LINUX
    QString indev = settings.value("input", "hw:1").toString();
    QString outdev = settings.value("output", "pulse").toString();
#elif defined(__APPLE__) && defined(__MACH__) // Works for X11 Qt on Mac OS X too
    QString indev = settings.value("input", "").toString();
    QString outdev = settings.value("output", "").toString();
#endif

    ui->inDevEdit->setText(indev);
    ui->outDevEdit->setText(outdev);

}

CIoConfig::~CIoConfig()
{
    delete ui;
}


/*! \brief Utility function to find device name of the Funcube Dogle.
 *
 * Linux: Get list of pulseaudio sources and search for "FUNcube Dongle" in the description
 * Mac: tbd...
 */
QString CIoConfig::getFcdDeviceName()
{
    QString retval("");

#ifdef Q_OS_LINUX
    pa_device_list devices;
    vector<pa_device> devlist = devices.get_input_devices();
    unsigned int i;

    qDebug() << __FUNCTION__ << ": Available input devices";
    for (i = 0; i < devlist.size(); i++)
    {
        qDebug() << "    " << i << ":" << QString(devlist[i].get_description().c_str());
        if (QString(devlist[i].get_description().c_str()).contains("FUNcube Dongle"))
        {
            retval = QString(devlist[i].get_name().c_str());
        }
    }

#elif defined(__APPLE__) && defined(__MACH__) // Works for X11 Qt on Mac OS X too
    // TODO
#endif

    return retval;
}


/*! \brief Save configuration. */
void CIoConfig::saveConfig()
{
    QSettings settings;

    settings.setValue("input", ui->inDevEdit->text());
    settings.setValue("output", ui->outDevEdit->text());
}
