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
#include <QDebug>

#include "gain_options.h"
#include "ui_gain_options.h"

CGainOptions::CGainOptions(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CGainOptions)
{
    ui->setupUi(this);
}

CGainOptions::~CGainOptions()
{
    delete ui;
}

/*! \brief Catch window close events.
 *
 * This method is called when the user closes the demod options dialog
 * window using the window close icon. We catch the event and hide the
 * dialog but keep it around for later use.
 */
void CGainOptions::closeEvent(QCloseEvent *event)
{
    hide();
    event->ignore();
}

/*! \brief Set gain stages for new device. */
void CGainOptions::setGainStages(gain_list_t &gain_list)
{
    qDebug() << "********************";
    for (gain_list_t::iterator it = gain_list.begin(); it != gain_list.end(); ++it)
    {
        qDebug() << "Gain name:" << QString(it->name.c_str());
        qDebug() << "      min:" << it->start;
        qDebug() << "      max:" << it->stop;
        qDebug() << "     step:" << it->step;
    }
    qDebug() << "********************";
}
