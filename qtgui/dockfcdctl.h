/* -*- c++ -*- */
/*
 * Copyright 2011-2012 Alexandru Csete OZ9AEC.
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
#ifndef DOCKFCDCTL_H
#define DOCKFCDCTL_H

#include <QDockWidget>

namespace Ui {
    class DockFcdCtl;
}

class DockFcdCtl : public QDockWidget
{
    Q_OBJECT

public:
    explicit DockFcdCtl(QWidget *parent = 0);
    ~DockFcdCtl();

    void  setLnbLo(double freq_mhz);
    double lnbLo();

    void  setLnaGain(float gain);
    float lnaGain();

    void setFreqCorr(int corr);
    int  freqCorr();

    void   setIqGain(double gain);
    double iqGain();

    void   setIqPhase(double phase);
    double iqPhase();

signals:
    void lnaGainChanged(float gain);
    void freqCorrChanged(int value);
    void lnbLoChanged(double freq_mhz);
    void iqCorrChanged(double gain, double phase);

private slots:
    void on_lnbSpinBox_valueChanged(double value);
    void on_lnaComboBox_activated(const QString value_str);
    void on_freqCorrSpinBox_valueChanged(int value);
    void on_iqGainSpinBox_valueChanged(double value);
    void on_iqPhaseSpinBox_valueChanged(double value);

private:
    Ui::DockFcdCtl *ui;
};

#endif // FCDCTL_H
