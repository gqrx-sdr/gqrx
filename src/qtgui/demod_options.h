/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           http://gqrx.dk/
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
#ifndef DEMOD_OPTIONS_H
#define DEMOD_OPTIONS_H

#include <QDialog>
#include <QCloseEvent>

namespace Ui {
    class CDemodOptions;
}


/*! \brief Widget for adjusting demodulator options.
 *
 */
class CDemodOptions : public QDialog
{
    Q_OBJECT

public:

    /*! \brief Index in the QStackedWidget. */
    enum page {
        PAGE_NO_OPT = 0,
        PAGE_FM_OPT = 1,
        PAGE_AM_OPT = 2,
        PAGE_CW_OPT = 3,
        PAGE_NUM    = 4
    };

    explicit CDemodOptions(QWidget *parent = 0);
    ~CDemodOptions();

    void closeEvent(QCloseEvent *event);

    void setCurrentPage(int index);
    int  currentPage() const;

    void setCwOffset(int offset);
    int  getCwOffset(void) const;

    void setMaxDev(float max_dev);
    float getMaxDev(void) const;

    void setEmph(double tau);
    double getEmph(void) const;

signals:
    /*! \brief Signal emitted when new FM deviation is selected. */
    void fmMaxdevSelected(float max_dev);

    /*! \brief Signal emitted when new FM de-emphasis constant is selected. */
    void fmEmphSelected(double tau);

    /*! \brief Signal emitted when AM DCR is toggled. */
    void amDcrToggled(bool enabled);

    /*! \brief CW offset changed. */
    void cwOffsetChanged(int offset);

private slots:
    void on_maxdevSelector_activated(int index);
    void on_emphSelector_activated(int index);
    void on_dcrCheckBox_toggled(bool checked);
    void on_cwOffsetSpin_valueChanged(int value);

private:
    Ui::CDemodOptions *ui;
};

#endif // DEMOD_OPTIONS_H
