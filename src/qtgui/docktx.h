/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2026 Gqrx contributors.
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
#ifndef DOCKTX_H
#define DOCKTX_H

#include <QDockWidget>
#include <QList>
#include <QSettings>

namespace Ui {
    class DockTx;
}

class DockTx : public QDockWidget
{
    Q_OBJECT

public:
    explicit DockTx(QWidget *parent = 0);
    ~DockTx();

    void    readSettings(QSettings *settings);
    void    saveSettings(QSettings *settings) const;

    void    setPtt(bool ptt);
    bool    ptt(void) const;
    qint64  offsetHz(void) const;
    int     modIndex(void) const;
    double  power(void) const;
    float   micGain(void) const;
    float   ctcssFreq(void) const;

signals:
    void    pttToggled(bool ptt);
    void    txModeChanged(int mode);
    void    rfPowerChanged(double power);
    void    micGainChanged(float gain_db);
    void    ctcssChanged(float freq);
    void    txOffsetChanged(qint64 offset_hz);

private slots:
    void    on_pttButton_toggled(bool checked);
    void    on_modSelector_currentIndexChanged(int index);
    void    on_powerSlider_valueChanged(int value);
    void    on_micSlider_valueChanged(int value);
    void    on_ctcssCombo_currentIndexChanged(int index);
    void    on_offsetSpinBox_valueChanged(double value);

private:
    Ui::DockTx *ui;
};

#endif // DOCKTX_H
