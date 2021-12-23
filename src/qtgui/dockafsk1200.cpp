/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
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
#include <QFileDialog>
#include <QMessageBox>

#include "dockafsk1200.h"
#include "ui_dockafsk1200.h"

DockAFSK1200::DockAFSK1200(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::DockAFSK1200)
{
    ui->setupUi(this);

    /* select font for text viewer */
    ui->textView->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

    /* Add right-aligned info button */
    QWidget *spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->toolBar->addWidget(spacer);
    ui->toolBar->addAction(ui->actionInfo);

    /* AFSK1200 decoder */
    decoder = new CAfsk12(this);
    connect(decoder, SIGNAL(newMessage(QString)), ui->textView, SLOT(appendPlainText(QString)));
}

DockAFSK1200::~DockAFSK1200()
{
    qDebug() << "AFSK1200 decoder destroyed.";

    delete decoder;
    delete ui;
}


/*! \brief Process new set of samples. */
void DockAFSK1200::process_samples(float *buffer, int length)
{
    int overlap = 18;
    int i;

    for (i = 0; i < length; i++) {
        tmpbuf.append(buffer[i]);
    }

    decoder->demod(tmpbuf.data(), length);

    /* clear tmpbuf and store "overlap" */
    tmpbuf.clear();
    for (i = length-overlap; i < length; i++) {
        tmpbuf.append(buffer[i]);
    }

}

/*! \brief User clicked on the Clear button. */
void DockAFSK1200::on_actionClear_triggered()
{
    ui->textView->clear();
}

/*! \brief User clicked on the Save button. */
void DockAFSK1200::on_actionSave_triggered()
{
    /* empty text view has blockCount = 1 */
    if (ui->textView->blockCount() < 2) {
        QMessageBox::warning(this, tr("Gqrx error"), tr("Nothing to save."),
                             QMessageBox::Ok, QMessageBox::Ok);
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"),
                                                    QDir::homePath(),
                                                    tr("Text Files (*.txt)"));

    if (fileName.isEmpty()) {
        qDebug() << "Save cancelled by user";
        return;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qInfo() << "Error creating file: " << fileName;
        return;
    }

    QTextStream out(&file);
    out << ui->textView->toPlainText();
    file.close();
}


/*! \brief User clicked Info button. */
void DockAFSK1200::on_actionInfo_triggered()
{
    QMessageBox::about(this, tr("About AFSK1200 Decoder"),
                       tr("<p>Gqrx AFSK1200 Decoder %1</p>"
                          "<p>The Gqrx AFSK1200 decoder taps directly into the SDR signal path "
                          "eliminating the need to mess with virtual or real audio cables. "
                          "It can decode AX.25 packets and displays the decoded packets in a text view.</p>"
                          "<p>The decoder is based on Qtmm, which is available for Linux, Mac and Windows "
                          "at <a href='http://qtmm.sf.net/'>http://qtmm.sf.net</a>.</p>"
                          ).arg(VERSION));

}

void DockAFSK1200::on_cbEnabled_toggled(bool checked)
{
    qDebug() << "DockAFSK1200 cbEnabled" << checked;
    emit afskDecoderToggled(checked);
}
