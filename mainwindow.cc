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
#include <QDebug>
#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    /* create receiver object */
    rx = new receiver("hw:1");

    rx->set_rf_freq(144500000.0f);
    rx->set_filter_offset(25000.0);
}

MainWindow::~MainWindow()
{
    delete ui;
    delete rx;
}


void MainWindow::on_rxStartStopButton_toggled(bool checked)
{
    if (checked) {
        /* start receiver and update button label */
        rx->start();
        ui->rxStartStopButton->setText(tr("Stop"));
    }
    else {
        /* stop receiver and update label */
        rx->stop();
        ui->rxStartStopButton->setText(tr("Start"));
    }
}
