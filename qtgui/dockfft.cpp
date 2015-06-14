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
#include <QString>
#include <QSettings>
#include <QDebug>
#include "dockfft.h"
#include "ui_dockfft.h"


#define DEFAULT_FFT_RATE  25
#define DEFAULT_FFT_SIZE  2048
#define DEFAULT_FFT_SPLIT 35
#define DEFAULT_FFT_AVG   75


DockFft::DockFft(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::DockFft)
{
    ui->setupUi(this);

#ifdef Q_OS_MAC
    // Workaround for Mac, see http://stackoverflow.com/questions/3978889/why-is-qhboxlayout-causing-widgets-to-overlap
    // Might be fixed in Qt 5?
    ui->resetButton->setAttribute(Qt::WA_LayoutUsesWidgetRect);
    ui->centerButton->setAttribute(Qt::WA_LayoutUsesWidgetRect);
    ui->demodButton->setAttribute(Qt::WA_LayoutUsesWidgetRect);
    ui->fillButton->setAttribute(Qt::WA_LayoutUsesWidgetRect);
#endif

    // Add predefined gqrx colors to chooser.
    ui->colorPicker->insertColor(QColor(0xFF,0xFF,0xFF,0xFF), "White");
    ui->colorPicker->insertColor(QColor(0xFA,0xFA,0x7F,0xFF), "Yellow");
    ui->colorPicker->insertColor(QColor(0x97,0xD0,0x97,0xFF), "Green");
    ui->colorPicker->insertColor(QColor(0xFF,0xC8,0xC8,0xFF), "Pink");
    ui->colorPicker->insertColor(QColor(0xB7,0xE0,0xFF,0xFF), "Blue");
    ui->colorPicker->insertColor(QColor(0x7F,0xFA,0xFA,0xFF), "Cyan");
}

DockFft::~DockFft()
{
    delete ui;
}


/*! \brief Get current FFT rate setting.
 *  \return The current FFT rate in frames per second (always non-zero)
 */
int DockFft::fftRate()
{
    bool ok;
    int fps = 10;
    QString strval = ui->fftRateComboBox->currentText();

    strval.remove(" fps");
    fps = strval.toInt(&ok, 10);

    if (!ok)
        qDebug() << "DockFft::fftRate : Could not convert" <<
                    strval << "to number.";
    else
        qDebug() << "New FFT rate:" << fps << "Hz";

    return fps;
}

/*! \brief Select new FFT rate in the combo box.
 *  \param rate The new rate.
 *  \returns The actual FFT rate selected.
 */
int DockFft::setFftRate(int fft_rate)
{
    int idx = -1;
    QString rate_str = QString("%1 fps").arg(fft_rate);

    qDebug() << __func__ << "to" << rate_str;

    idx = ui->fftRateComboBox->findText(rate_str, Qt::MatchExactly);
    if(idx != -1)
        ui->fftRateComboBox->setCurrentIndex(idx);

    return fftRate();
}

/*! \brief Select new FFT size in the combo box.
 *  \param rate The new FFT size.
 *  \returns The actual FFT size selected.
 */
int DockFft::setFftSize(int fft_size)
{
    int idx = -1;
    QString size_str = QString::number(fft_size);

    qDebug() << __func__ << "to" << size_str;

    idx = ui->fftSizeComboBox->findText(size_str, Qt::MatchExactly);
    if(idx != -1)
        ui->fftSizeComboBox->setCurrentIndex(idx);

    return fftSize();
}

/*! \brief Get current FFT rate setting.
 *  \return The current FFT rate in frames per second (always non-zero)
 */
int DockFft::fftSize()
{
    bool ok;
    int fft_size = 10;
    QString strval = ui->fftSizeComboBox->currentText();

    fft_size = strval.toInt(&ok, 10);

    if (!ok)
    {
        qDebug() << __func__ << "could not convert" << strval << "to number.";
    }

    if (fft_size == 0)
    {
        qDebug() << "Somehow we ended up with FFT size = 0; using" << DEFAULT_FFT_SIZE;
        fft_size = DEFAULT_FFT_SIZE;
    }

    return fft_size;
}

/*! \brief Save FFT settings. */
void DockFft::saveSettings(QSettings *settings)
{
    int  intval;

    if (!settings)
        return;

    settings->beginGroup("fft");

    intval = fftSize();
    if (intval != DEFAULT_FFT_SIZE)
        settings->setValue("fft_size", intval);
    else
        settings->remove("fft_size");

    intval = fftRate();
    if (intval != DEFAULT_FFT_RATE)
        settings->setValue("fft_rate", fftRate());
    else
        settings->remove("fft_rate");

    if (ui->fftAvgSlider->value() != DEFAULT_FFT_AVG)
        settings->setValue("averaging", ui->fftAvgSlider->value());
    else
        settings->remove("averaging");

    if (ui->fftSplitSlider->value() != DEFAULT_FFT_SPLIT)
        settings->setValue("split", ui->fftSplitSlider->value());
    else
        settings->remove("split");

    QColor fftColor = ui->colorPicker->currentColor();
    if (fftColor != QColor(0xFF,0xFF,0xFF,0xFF))
        settings->setValue("pandapter_color", fftColor);
    else
        settings->remove("pandapter_color");

    if (ui->fillButton->isChecked())
        settings->setValue("pandapter_fill", true);
    else
        settings->remove("pandapter_fill");

    settings->endGroup();
}

/*! \brief Read FFT settings. */
void DockFft::readSettings(QSettings *settings)
{
    int intval;
    bool bool_val = false;
    bool conv_ok = false;
    QColor color;

    if (!settings)
        return;

    settings->beginGroup("fft");

    intval = settings->value("fft_rate", DEFAULT_FFT_RATE).toInt(&conv_ok);
    if (conv_ok)
        setFftRate(intval);

    intval = settings->value("fft_size", DEFAULT_FFT_SIZE).toInt(&conv_ok);
    if (conv_ok)
        setFftSize(intval);

    intval = settings->value("averaging", DEFAULT_FFT_AVG).toInt(&conv_ok);
    if (conv_ok)
        ui->fftAvgSlider->setValue(intval);

    intval = settings->value("split", DEFAULT_FFT_SPLIT).toInt(&conv_ok);
    if (conv_ok)
        ui->fftSplitSlider->setValue(intval);

    color = settings->value("pandapter_color", QColor(0xFF,0xFF,0xFF,0xFF)).value<QColor>();
    ui->colorPicker->setCurrentColor(color);

    bool_val = settings->value("pandapter_fill", false).toBool();
    ui->fillButton->setChecked(bool_val);

    settings->endGroup();
}

/*! \brief FFT size changed. */
void DockFft::on_fftSizeComboBox_currentIndexChanged(const QString &text)
{
    int value = text.toInt();
    emit fftSizeChanged(value);
}

/*! \brief FFT rate changed. */
void DockFft::on_fftRateComboBox_currentIndexChanged(const QString & text)
{
    int fps = fftRate();
    Q_UNUSED(text);

    emit fftRateChanged(fps);
}

/*! \brief Split between waterfall and pandapter changed.
 *  \param value The percentage of the waterfall.
 */
void DockFft::on_fftSplitSlider_valueChanged(int value)
{
    emit fftSplitChanged(value);
}

/*! \brief FFT filter gain changed. */
void DockFft::on_fftAvgSlider_valueChanged(int value)
{
    float avg = 1.0 - 1.0e-2 * ((float)value);

    emit fftAvgChanged(avg);
}

/*! \brief FFT zoom level changed */
void DockFft::on_fftZoomSlider_valueChanged(int level)
{
    ui->zoomLevelLabel->setText(QString("%1x").arg(level));
    emit fftZoomChanged((float)level);
}

void DockFft::on_resetButton_clicked(void)
{
    emit resetFftZoom();
}

void DockFft::on_centerButton_clicked(void)
{
    emit gotoFftCenter();
}

void DockFft::on_demodButton_clicked(void)
{
    emit gotoDemodFreq();
}

/*! FFT color has changed. */
void DockFft::on_colorPicker_colorChanged(const QColor &color)
{
    emit fftColorChanged(color);
}

/*! FFT plot fill button toggled. */
void DockFft::on_fillButton_toggled(bool checked)
{
    emit fftFillToggled(checked);
}

/*! peakHold button toggled */
void DockFft::on_peakHoldButton_toggled(bool checked)
{
    emit fftPeakHoldToggled(checked);
}

/*! peakDetection button toggled */
void DockFft::on_peakDetectionButton_toggled(bool checked)
{
    emit peakDetectionToggled(checked);
}
