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
#ifndef DOCKDEMOD_H
#define DOCKDEMOD_H

#include <QDockWidget>

namespace Ui {
    class DockDemod;
}


/*! \brief Demodulator dock window.
 *  \ingroup UI
 *
 * This dock widget encapsulates the demodulator delector and the
 * demodulator options. The UI itself is in the dockdemod.ui file.
 *
 * The demodulator options are placed in a QStackedWidget which allows
 * us to show only the options relevant for the current demodulator.
 * Switching between the different options is done automatically when
 * the user selects a new demodulator.
 *
 * This class also provides the signal/slot API necessary to connect
 * the encapsulated widgets to the rest of the application.
 */
class DockDemod : public QDockWidget
{
    Q_OBJECT

public:
    explicit DockDemod(QWidget *parent = 0);
    ~DockDemod();

    void setCurrentDemod(int demod);
    int  currentDemod();

    void setCurrentSideBand(int sideband);
    int  currentSideBand();

signals:
    /*! \brief Signal emitted when new demodulator is selected. */
    void demodSelected(int demod);

    /*! \brief Signal emitted when new SSb side band has been selected. */
    void sidebandSelected(int sideband);

private slots:
    void on_modeSelector_activated(int index);
    void on_sidebandSelector_activated(int index);

private:
    Ui::DockDemod *ui;   /*! The Qt Designer UI. */
};

#endif // DOCKDEMOD_H
