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
#ifndef DOCKINPUT_H
#define DOCKINPUT_H

#include <QDockWidget>

namespace Ui {
    class DockInput;
}

/*! \brief Input device dock window.
 *  \ingroup UI
 *
 * This dock widget encapsulates the input device selector and controls.
 * The UI itself is in the dockinput.ui file.
 *
 * This class also provides the signal/slot API necessary to connect
 * the encapsulated widgets to the rest of the application.
 *
 * \note This is just a hack and it is very likely that this class will change a lot.
 */
class DockInput : public QDockWidget
{
    Q_OBJECT

public:
    explicit DockInput(QWidget *parent = 0);
    ~DockInput();


signals:
    void fcdDcCorrChanged(double dci, double dcq);
    void fcdIqCorrChanged(double gain, double phase);

private slots:
    void on_fcdDciSpinBox_valueChanged(double dci);
    void on_fcdDcqSpinBox_valueChanged(double dcq);
    void on_fcdIqPhaseSpinBox_valueChanged(double phase);
    void on_fcdIqGainSpinBox_valueChanged(double phase);


private:
    Ui::DockInput *ui;
};

#endif // DOCKINPUT_H
