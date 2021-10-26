/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2020 Oliver Grossmann.
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
#ifndef DXC_OPTIONS_H
#define DXC_OPTIONS_H

#include <memory>

#include <QCloseEvent>
#include <QShowEvent>
#include <QTcpSocket>
#include <QSettings>

#include <QFrame>

namespace Ui {
class DockDXCluster;
}

class DockDXCluster : public QFrame
{
    Q_OBJECT

public:
    explicit DockDXCluster(QWidget *parent = 0);
    ~DockDXCluster();

    void saveSettings(std::shared_ptr<QSettings> settings);
    void readSettings(std::shared_ptr<QSettings> settings);

private slots:

    void on_pushButton_DXCConnect_clicked();
    void on_pushButton_DXCDisconnect_clicked();
    void connected();
    void disconnected();
    void readyToRead();

private:
    Ui::DockDXCluster *ui;
    QTcpSocket *m_socket;
};

#endif // DXC_OPTIONS_H
