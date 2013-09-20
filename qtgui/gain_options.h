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
#ifndef GAIN_OPTIONS_H
#define GAIN_OPTIONS_H

#include <QCloseEvent>
#include <QDialog>

namespace Ui {
class CGainOptions;
}

/*! \brief Adjust individual gain stages. */
class CGainOptions : public QDialog
{
    Q_OBJECT
    
public:
    explicit CGainOptions(QWidget *parent = 0);
    ~CGainOptions();
    
    void closeEvent(QCloseEvent *event);

private:
    Ui::CGainOptions *ui;
};

#endif // GAIN_OPTIONS_H
