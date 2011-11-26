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


/*! \brief Utility function to find device name of the Funcibe Dogle.
 *
 * On linux we read /proc/asound/cards  (should work with ALSA and portaudio?)
 * On Mac: ???
 */
QString CIoConfig::getFcdDeviceName()
{

#ifdef Q_OS_LINUX
    // Format:
    // 0 [VT82xx         ]: HDA-Intel - HDA VIA VT82xx
    //                     HDA VIA VT82xx at 0xbfffc000 irq 17
    // 1 [V10            ]: USB-Audio - FUNcube Dongle V1.0
    //                     Hanlincrest Ltd.          FUNcube Dongle V1.0   at usb-0000:00:10.4-4.1 ...
    //
    // Note that V10 is only displayed if the FCD was plugged in before boot.
    // If FCD is plugged in after boot the text will say "default", so it's
    // safest not to use that. We just search for "USB-Audio - FUNCube Dongle"

    QFile file("/proc/asound/cards");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug() << "Error opening /proc/asound/cards";
        return "";
    }

    // Read all lines into a QByteArray, create a Qstring from it
    // and split the lines into a QStringList
    QStringList data(QString(file.readAll()).split("\n"));

    for (int i = 0; i < data.length(); i+=2)
    {
        if (data[i].contains("USB-Audio - FUNcube Dongle"))
        {
            bool convOk;
            uint devId;

            // extract device number
            devId = data[i].left(2).toUInt(&convOk);

            if (convOk)
            {
                return QString("hw:%1").arg(devId);
            }
            else
            {
                qDebug() << "Error extracting device ID from:" << data[i];
            }
        }
    }

    file.close();
#elif defined(__APPLE__) && defined(__MACH__) // Works for X11 Qt on Mac OS X too
    // TODO
#endif

    return "";
}


/*! \brief Save configuration. */
void CIoConfig::saveConfig()
{
    QSettings settings;

    settings.setValue("input", ui->inDevEdit->text());
    settings.setValue("output", ui->outDevEdit->text());
}
