/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2023 Jeff Long.
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
#include <QDir>
#include <QString>

#include "data_controls.h"
#include "ui_data_controls.h"


CDataControls::CDataControls(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CDataControls)
{
    ui->setupUi(this);
    ui->dataIntervalBox->setCurrentIndex(0);
    ui->enableCheckBox->setCheckState(Qt::Unchecked);
    ui->samplesCheckBox->setCheckState(Qt::Unchecked);
    ui->linearFFTCheckBox->setCheckState(Qt::Unchecked);
}

CDataControls::~CDataControls()
{
    delete ui;
}

void CDataControls::on_enableCheckBox_stateChanged(int state)
{
    if (state)
    {
        ui->uriEdit->setEnabled(false);
        ui->uriResetButton->setEnabled(false);
        QString uri = ui->uriEdit->text();
    }
    else {
        ui->uriEdit->setEnabled(true);
        ui->uriResetButton->setEnabled(true);
    }
    emit settingsChanged();
}

void CDataControls::on_dataIntervalBox_currentIndexChanged(int index)
{
    emit settingsChanged();
}

void CDataControls::on_samplesCheckBox_stateChanged(int state)
{
    emit settingsChanged();
}

void CDataControls::on_linearFFTCheckBox_stateChanged(int state)
{
    emit settingsChanged();
}

void CDataControls::on_uriResetButton_clicked()
{
    ui->uriEdit->setText(DEFAULT_URI);
}

void CDataControls::on_uriEdit_textChanged(const QString& text)
{
    if (text.startsWith("tcp://") || text.startsWith("ipc:///"))
        ui->statusLabel->setText("STOPPED");
    else
        ui->statusLabel->setText("Use tcp:// or ipc:///");
}

QString CDataControls::uri()
{
    return ui->uriEdit->text();
}

// Parse and return interval
float CDataControls::interval()
{
    // Use fft display timer
    if (ui->dataIntervalBox->currentIndex() == 0)
        return 0;

    bool ok;
    float fps;
    QString strval = ui->dataIntervalBox->currentText();

    strval.remove(" sec");
    fps = strval.toFloat(&ok);

    return ok ? fps : 0.0;
}

bool CDataControls::outputEnabled()
{
    return (ui->enableCheckBox->checkState());
}

bool CDataControls::samplesEnabled()
{
    return (ui->samplesCheckBox->checkState());
}

bool CDataControls::linearFFTEnabled()
{
    return (ui->linearFFTCheckBox->checkState());
}

void CDataControls::setOutputEnabled(bool enable)
{
    ui->enableCheckBox->setCheckState(enable ? Qt::Checked : Qt::Unchecked);
}

void CDataControls::setStatus(const std::string& status)
{
    ui->statusLabel->setText(status.c_str());
}

/*! \brief Catch window close events. */
void CDataControls::closeEvent(QCloseEvent *event)
{
    hide();
}

/*! \brief Catch window show events. */
void CDataControls::showEvent(QShowEvent * event)
{
    Q_UNUSED(event);
}

void CDataControls::saveSettings(QSettings *settings)
{
    if (!settings)
        return;

    settings->setValue("data/uri", uri());

    if (samplesEnabled())
        settings->setValue("data/samples", true);
    else
        settings->remove("data/samples");

    if (linearFFTEnabled())
        settings->setValue("data/linear_fft", true);
    else
        settings->remove("data/linear_fft");
}

void CDataControls::readSettings(QSettings *settings)
{
    bool x;

    if (!settings)
        return;

    QString uri = settings->value("data/uri", "").toString();
    ui->uriEdit->setText(uri);

    x = settings->value("data/samples", false).toBool();
    ui->samplesCheckBox->setCheckState(x ? Qt::Checked : Qt::Unchecked);

    x = settings->value("data/linear_fft", false).toBool();
    ui->linearFFTCheckBox->setCheckState(x ? Qt::Checked : Qt::Unchecked);
}
