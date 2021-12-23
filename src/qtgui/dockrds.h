/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2011-2013 Alexandru Csete OZ9AEC.
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
#ifndef DOCKRDS_H
#define DOCKRDS_H
#include <QFrame>
#include <QSettings>

namespace Ui {
    class DockRDS;
}


class DockRDS : public QFrame
{
    Q_OBJECT

public:
    explicit DockRDS(QWidget *parent = 0);
    ~DockRDS();

public slots:
    void updateRDS(QString text, int type);
    void showEnabled();
    void showDisabled();
    void setEnabled();
    void setDisabled();
    void setRDSmode(bool cmd);

private:
    void ClearTextFields();

signals:
    void rdsDecoderToggled(bool);
    void rdsPI(QString text);

private slots:
    void on_rdsCheckbox_toggled(bool checked);

private:
    Ui::DockRDS *ui;        /*! The Qt designer UI file. */
};

#endif // DOCKRDS_H
