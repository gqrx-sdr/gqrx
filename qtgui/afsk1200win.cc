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
#include <QDebug>
#include "afsk1200win.h"
#include "ui_afsk1200win.h"


Afsk1200Win::Afsk1200Win(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::Afsk1200Win)
{
    ui->setupUi(this);

    decoder = new CAfsk12(this);

    connect(decoder, SIGNAL(newMessage(QString)), ui->textView, SLOT(appendPlainText(QString)));
}

Afsk1200Win::~Afsk1200Win()
{
    delete decoder;
    delete ui;
}

