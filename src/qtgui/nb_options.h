/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           http://gqrx.dk/
 *
 * Copyright 2013 Alexandru Csete OZ9AEC.
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
#ifndef NB_OPTIONS_H
#define NB_OPTIONS_H

#include <QDialog>
#include <QCloseEvent>

namespace Ui {
class CNbOptions;
}

class CNbOptions : public QDialog
{
    Q_OBJECT
    
public:
    explicit CNbOptions(QWidget *parent = 0);
    ~CNbOptions();

    void closeEvent(QCloseEvent *event);

    double nbThreshold(int nbid);

signals:
    void thresholdChanged(int nb, double val);

private slots:
    void on_nb1Threshold_valueChanged(double val);
    void on_nb2Threshold_valueChanged(double val);

private:
    Ui::CNbOptions *ui;
};

#endif // NB_OPTIONS_H
