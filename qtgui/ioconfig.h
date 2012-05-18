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
#ifndef IOCONFIG_H
#define IOCONFIG_H

#include <QDialog>
#include <QString>
#include <QSettings>

namespace Ui {
    class CIoConfig;
}


/*! \brief Inout/output device configurator. */
class CIoConfig : public QDialog
{
    Q_OBJECT

public:
    explicit CIoConfig(QSettings *settings, QWidget *parent = 0);
    ~CIoConfig();

    static QString getFcdDeviceName();

private slots:
    void saveConfig();

private:
    Ui::CIoConfig *ui;
};

#endif // IOCONFIG_H
