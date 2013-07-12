/* -*- c++ -*- */
/*
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
#include <QFileDialog>
#include <QPalette>

#include "audio_options.h"
#include "ui_audio_options.h"

CAudioOptions::CAudioOptions(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CAudioOptions)
{
    ui->setupUi(this);

    work_dir = new QDir();

    error_palette = new QPalette();
    error_palette->setColor(QPalette::Text, Qt::red);
}

CAudioOptions::~CAudioOptions()
{
    delete work_dir;
    delete error_palette;
    delete ui;
}


/*! \brief Catch window close events.
 *
 * This method is called when the user closes the audio options dialog
 * window using the window close icon. We catch the event and hide the
 * dialog but keep it around for later use.
 */
void CAudioOptions::closeEvent(QCloseEvent *event)
{
    hide();
    event->ignore();
}

/*! \brief Set initial location of WAV files. */
void CAudioOptions::setRecDir(const QString &dir)
{
    ui->recDirEdit->setText(dir);
}

/*! \brief Slot called when the recordings directory has changed either
 *         because of user input or programmatically.
 */
void CAudioOptions::on_recDirEdit_textChanged(const QString &dir)
{
    if (work_dir->exists(dir))
    {
        ui->recDirEdit->setPalette(QPalette());  // Clear custom color
        emit newRecDirSelected(dir);
    }
    else
    {
        ui->recDirEdit->setPalette(*error_palette);  // indicate error
    }
}

/*! \brief Slot called when the user clicks on the "Select" button. */
void CAudioOptions::on_recDirButton_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select a directory"),
                                                    ui->recDirEdit->text(),
                                                    QFileDialog::ShowDirsOnly |
                                                    QFileDialog::DontResolveSymlinks);

    if (!dir.isNull())
        ui->recDirEdit->setText(dir);
}
