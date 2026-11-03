/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
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
#include <QSettings>
#include <QString>

namespace Ui {
    class DockTx;
}

/*! \brief Dock widget with the transmitter controls. */
class DockTx : public QDockWidget
{
    Q_OBJECT

public:
    explicit DockTx(QWidget *parent = 0);
    ~DockTx();

    void    readSettings(QSettings *settings);
    void    saveSettings(QSettings *settings);

    int     txPower() const;
    int     micGain() const;

public slots:
    void    setTransmitting(bool transmitting);
    void    setStatus(const QString &status);

signals:
    void txToggled(bool enabled);        /*!< PTT pressed (true) or released (false). */
    void txPowerChanged(int percent);    /*!< TX power in percent of the gain range. */
    void micGainChanged(float gain_db);  /*!< Microphone gain in dB. */

private slots:
    void on_pttButton_toggled(bool checked);
    void on_powerSlider_valueChanged(int value);
    void on_micSlider_valueChanged(int value);

private:
    Ui::DockTx *ui;     /*! The Qt designer UI file. */
};

#endif // DOCKTX_H
