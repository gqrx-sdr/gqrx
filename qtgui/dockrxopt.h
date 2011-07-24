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
#ifndef DOCKRXOPT_H
#define DOCKRXOPT_H

#include <QDockWidget>

namespace Ui {
    class DockRxOpt;
}


/*! \brief Dock window with receiver options.
 *  \ingroup UI
 *
 * This dock widget encapsulates the receiver options. The controls
 * are grouped in a tool box that allows packing many controls in little space.
 * The UI itself is in the dockrxopt.ui file.
 *
 * This class also provides the signal/slot API necessary to connect
 * the encapsulated widgets to the rest of the application.
 */
class DockRxOpt : public QDockWidget
{
    Q_OBJECT

public:
    explicit DockRxOpt(QWidget *parent = 0);
    ~DockRxOpt();

    void setCurrentDemod(int demod);
    int  currentDemod();

    //void setCurrentMaxdev(float max:dev);
    //float currentMaxdev();

    void setCurrentSideBand(int sideband);
    int  currentSideBand();


signals:
    /*! \brief Signal emitted when new demodulator is selected. */
    void demodSelected(int demod);

    /*! \brief Signal emitted when new FM deviation is selected. */
    void fmMaxdevSelected(float max_dev);

    /*! \brief Signal emitted when new FM de-emphasis constant is selected. */
    void fmEmphSelected(double tau);

    /*! \brief Signal emitted when AM DCR status is toggled. */
    void amDcrToggled(bool enabled);

    /*! \brief Signal emitted when new SSB side band has been selected. */
    void sidebandSelected(int sideband);

    /*! \brief Signal emitted when baseband gain has changed. Gain is in dB. */
    void bbGainChanged(float gain);

    /*! \brief Signal emitted when squelch level has changed. Level is in dBFS. */
    void sqlLevelChanged(double level);


private slots:
    void on_modeSelector_activated(int index);
    void on_maxdevSelector_activated(int index);
    void on_emphSelector_activated(int index);
    void on_dcr_toggled(bool checked);
    void on_sidebandSelector_activated(int index);
    void on_bbGainSlider_valueChanged(int value);
    void on_sqlSlider_valueChanged(int value);

private:
    Ui::DockRxOpt *ui;   /*! The Qt designer UI file. */
};

#endif // DOCKRXOPT_H
