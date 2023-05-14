/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2011-2016 Alexandru Csete OZ9AEC.
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
#include <math.h>
#include <QString>
#include <QSettings>
#include <QDebug>
#include <QVariant>
#include "dockfft.h"
#include "ui_dockfft.h"

#define DEFAULT_FFT_MAX_DB     -20
#define DEFAULT_FFT_MIN_DB     -120
#define DEFAULT_FFT_RATE        25
#define DEFAULT_FFT_SIZE        8192
#define DEFAULT_FFT_ZOOM        1
#define DEFAULT_FFT_WINDOW      1       // Hann
#define DEFAULT_PLOT_SCALE      0       // dBFS
#define DEFAULT_PLOT_PER        0       // RBW
#define DEFAULT_WATERFALL_SPAN  0       // Auto
#define DEFAULT_FFT_SPLIT       35
#define DEFAULT_FFT_AVG         25
#define DEFAULT_COLORMAP        "gqrx"

static const QStringList window_strs = {
    "hamming", "hann", "blackman", "rectangular", "kaiser",
    "blackmanharris", "bartlett", "flattop"
};

DockFft::DockFft(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::DockFft)
{
    ui->setupUi(this);

    m_sample_rate = 0.f;
    m_pand_last_modified = false;
    m_actual_frame_rate = 0.f;
    m_frame_dropping = false;

    // Add predefined gqrx colors to chooser.
    ui->colorPicker->insertColor(QColor(0xFF,0xFF,0xFF,0xFF), "White");
    ui->colorPicker->insertColor(QColor(0xFA,0xFA,0x7F,0xFF), "Yellow");
    ui->colorPicker->insertColor(QColor(0x97,0xD0,0x97,0xFF), "Green");
    ui->colorPicker->insertColor(QColor(0xFF,0xC8,0xC8,0xFF), "Pink");
    ui->colorPicker->insertColor(QColor(0xB7,0xE0,0xFF,0xFF), "Blue");
    ui->colorPicker->insertColor(QColor(0x7F,0xFA,0xFA,0xFF), "Cyan");

    ui->cmapComboBox->addItem(tr("Gqrx"), "gqrx");
    ui->cmapComboBox->addItem(tr("Viridis"), "viridis");
    ui->cmapComboBox->addItem(tr("Turbo"), "turbo");
    ui->cmapComboBox->addItem(tr("Plasma"), "plasma");
    ui->cmapComboBox->addItem(tr("Wht Cmp"), "whitehotcompressed");
    ui->cmapComboBox->addItem(tr("Wht Hot"), "whitehot");
    ui->cmapComboBox->addItem(tr("Blk Hot"), "blackhot");

    QFont font;
    QFontMetrics metrics(font);
    QRectF zoomRect = metrics.boundingRect("888888x");
    ui->zoomLevelLabel->setFixedWidth(zoomRect.width());
}
DockFft::~DockFft()
{
    delete ui;
}


/**
 * @brief Get current FFT rate setting.
 * @return The current FFT rate in frames per second (always non-zero)
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

/**
 * @brief Select new FFT rate in the combo box.
 * @param rate The new rate.
 * @returns The actual FFT rate selected.
 */
int DockFft::setFftRate(int fft_rate)
{
    int idx = -1;
    QString rate_str = QString("%1 fps").arg(fft_rate);

    qDebug() << __func__ << "to" << rate_str;

    idx = ui->fftRateComboBox->findText(rate_str, Qt::MatchExactly);
    if(idx != -1)
        ui->fftRateComboBox->setCurrentIndex(idx);

    updateInfoLabels();
    return fftRate();
}

/**
 * @brief Select new FFT size in the combo box.
 * @param rate The new FFT size.
 * @returns The actual FFT size selected.
 */
int DockFft::setFftSize(int fft_size)
{
    int idx = -1;
    QString size_str = QString::number(fft_size);

    qDebug() << __func__ << "to" << size_str;

    idx = ui->fftSizeComboBox->findText(size_str, Qt::MatchExactly);
    if(idx != -1)
        ui->fftSizeComboBox->setCurrentIndex(idx);

    updateInfoLabels();
    return fftSize();
}

void DockFft::setSampleRate(float sample_rate)
{
    if (sample_rate < 0.1f)
        return;

    m_sample_rate = sample_rate;
    updateInfoLabels();
}

/**
 * @brief Get current FFT rate setting.
 * @return The current FFT rate in frames per second (always non-zero)
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

/** Save FFT settings. */
void DockFft::saveSettings(QSettings *settings)
{
    int  intval;
    QString strval;

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

    intval = ui->fftWinComboBox->currentIndex();
    if ((unsigned int)intval > window_strs.size())
        intval = DEFAULT_FFT_WINDOW;
    strval = window_strs[intval];
    settings->setValue("fft_window", strval);

    intval = ui->wfSpanComboBox->currentIndex();
    if (intval != DEFAULT_WATERFALL_SPAN)
        settings->setValue("waterfall_span", intval);
    else
        settings->remove("waterfall_span");

    if (ui->fftAvgSlider->value() != DEFAULT_FFT_AVG)
        settings->setValue("averaging", ui->fftAvgSlider->value());
    else
        settings->remove("averaging");

    intval = ui->plotScaleBox->currentIndex();
    if      (intval == 1) strval = "dbv";
    else if (intval == 2) strval = "dbm";
    else                  strval = "dbfs";  // 0, default
    settings->setValue("plot_y_unit", strval);

    intval = ui->plotPerBox->currentIndex();
    if      (intval == 1) strval = "hz";
    else                  strval = "rbw";  // 0, default
    settings->setValue("plot_x_unit", strval);

    intval = ui->plotModeBox->currentIndex();
    if      (intval == 1) strval = "avg";
    else if (intval == 2) strval = "fill";
    else if (intval == 3) strval = "hist";
    else                  strval = "max";  // 0, default
    settings->setValue("plot_mode", strval);

    intval = ui->wfModeBox->currentIndex();
    if      (intval == 1) strval = "avg";
    else if (intval == 2) strval = "sync";
    else                  strval = "max";  // 0, default
    settings->setValue("waterfall_mode", strval);

    if (ui->fftSplitSlider->value() != DEFAULT_FFT_SPLIT)
        settings->setValue("split", ui->fftSplitSlider->value());
    else
        settings->remove("split");

    QColor fftColor = ui->colorPicker->currentColor();
    if (fftColor != QColor(0xFF,0xFF,0xFF,0xFF))
        settings->setValue("pandapter_color", fftColor);
    else
        settings->remove("pandapter_color");

    if (ui->fillCheckBox->isChecked())
        settings->setValue("pandapter_fill", true);
    else
        settings->remove("pandapter_fill");

    // dB ranges
    intval = ui->plotRangeSlider->minimumValue();
    if (intval == DEFAULT_FFT_MIN_DB)
        settings->remove("pandapter_min_db");
    else
        settings->setValue("pandapter_min_db", intval);

    intval = ui->plotRangeSlider->maximumValue();
    if (intval == DEFAULT_FFT_MAX_DB)
        settings->remove("pandapter_max_db");
    else
        settings->setValue("pandapter_max_db", intval);

    intval = ui->wfRangeSlider->minimumValue();
    if (intval == DEFAULT_FFT_MIN_DB)
        settings->remove("waterfall_min_db");
    else
        settings->setValue("waterfall_min_db", intval);

    intval = ui->wfRangeSlider->maximumValue();
    if (intval == DEFAULT_FFT_MAX_DB)
        settings->remove("waterfall_max_db");
    else
        settings->setValue("waterfall_max_db", intval);

    // pandapter and waterfall ranges locked together
    if (ui->lockCheckBox->isChecked())
        settings->setValue("db_ranges_locked", true);
    else
        settings->remove("db_ranges_locked");

    // Band Plan
    if (ui->bandPlanCheckBox->isChecked())
        settings->setValue("bandplan", true);
    else
        settings->remove("bandplan");

    // Markers
    if (ui->markersCheckBox->isChecked())
        settings->setValue("markers", true);
    else
        settings->remove("markers");

    // Peak
    if (ui->peakDetectCheckBox->isChecked())
        settings->setValue("peak_detect", true);
    else
        settings->remove("peak_detect");

    if (ui->maxHoldCheckBox->isChecked())
        settings->setValue("peak_hold", true);
    else
        settings->remove("peak_hold");

    if (ui->minHoldCheckBox->isChecked())
        settings->setValue("min_hold", true);
    else
        settings->remove("min_hold");

    if (QString::compare(ui->cmapComboBox->currentData().toString(), DEFAULT_COLORMAP))
        settings->setValue("waterfall_colormap", ui->cmapComboBox->currentData().toString());
    else
        settings->remove("waterfall_colormap");

    // FFT Zoom
    if (ui->fftZoomSlider->value() != DEFAULT_FFT_ZOOM)
        settings->setValue("fft_zoom", ui->fftZoomSlider->value());
    else
        settings->remove("fft_zoom");

    settings->endGroup();
}

/** Read FFT settings. */
void DockFft::readSettings(QSettings *settings)
{
    int     intval;
    QString strval;
    int     fft_min, fft_max;
    bool    bool_val = false;
    bool    conv_ok = false;
    QColor  color;
    int     configversion;

    if (!settings)
        return;

    configversion = settings->value("configversion").toInt();

    settings->beginGroup("fft");

    intval = settings->value("fft_rate", DEFAULT_FFT_RATE).toInt(&conv_ok);
    if (conv_ok)
        setFftRate(intval);
    emit fftRateChanged(fftRate());

    intval = settings->value("fft_size", DEFAULT_FFT_SIZE).toInt(&conv_ok);
    if (conv_ok)
        setFftSize(intval);
    emit fftSizeChanged(fftSize());

    if (configversion >= 4) {
        strval = settings->value("fft_window", "hann").toString();
        auto it = std::find(window_strs.begin(), window_strs.end(), strval);
        if (it == window_strs.end())
            intval = DEFAULT_FFT_WINDOW;
        else
            intval = std::distance(window_strs.begin(), it);
        conv_ok = true;
    } else {
        intval = settings->value("fft_window", DEFAULT_FFT_WINDOW).toInt(&conv_ok);
    }
    if (conv_ok)
        ui->fftWinComboBox->setCurrentIndex(intval);

    intval = settings->value("waterfall_span", DEFAULT_WATERFALL_SPAN).toInt(&conv_ok);
    if (conv_ok)
        ui->wfSpanComboBox->setCurrentIndex(intval);

    intval = settings->value("averaging", DEFAULT_FFT_AVG).toInt(&conv_ok);
    if (conv_ok)
        ui->fftAvgSlider->setValue(intval);

    // Plot scale and denominator
    strval = settings->value("plot_y_unit", "dbfs").toString();
    if      (strval == "dbv") intval = 1;
    else if (strval == "dbm") intval = 2;
    else                      intval = 0;  // "dbfs", default
    ui->plotScaleBox->setCurrentIndex(intval);
    int plot_scale = intval;

    strval = settings->value("plot_x_unit", "rbw").toString();
    if      (strval == "hz") intval = 1;
    else                     intval = 0;  // "rbw", default
    ui->plotPerBox->setCurrentIndex(intval);
    // Trigger additional required logic for plot_scale and plot_per
    on_plotScaleBox_currentIndexChanged(plot_scale);

    strval = settings->value("plot_mode", "max").toString();
    if      (strval == "avg")  intval = 1;
    else if (strval == "fill") intval = 2;
    else if (strval == "hist") intval = 3;
    else                       intval = 0;  // "max", default
    ui->plotModeBox->setCurrentIndex(intval);

    strval = settings->value("waterfall_mode", "max").toString();
    if      (strval == "avg")  intval = 1;
    else if (strval == "sync") intval = 2;
    else                       intval = 0;  // "max", default
    ui->wfModeBox->setCurrentIndex(intval);

    intval = settings->value("split", DEFAULT_FFT_SPLIT).toInt(&conv_ok);
    if (conv_ok)
        ui->fftSplitSlider->setValue(intval);

    color = settings->value("pandapter_color", QColor(0xFF,0xFF,0xFF,0xFF)).value<QColor>();
    ui->colorPicker->setCurrentColor(color);

    bool_val = settings->value("pandapter_fill", false).toBool();
    ui->fillCheckBox->setChecked(bool_val);

    // delete old dB settings from config
    if (settings->contains("reference_level"))
        settings->remove("reference_level");

    if (settings->contains("fft_range"))
        settings->remove("fft_range");

    fft_max = settings->value("pandapter_max_db", DEFAULT_FFT_MAX_DB).toInt();
    fft_min = settings->value("pandapter_min_db", DEFAULT_FFT_MIN_DB).toInt();
    setPandapterRange(fft_min, fft_max);
    emit pandapterRangeChanged((float) fft_min, (float) fft_max);

    fft_max = settings->value("waterfall_max_db", DEFAULT_FFT_MAX_DB).toInt();
    fft_min = settings->value("waterfall_min_db", DEFAULT_FFT_MIN_DB).toInt();
    setWaterfallRange(fft_min, fft_max);
    emit waterfallRangeChanged((float) fft_min, (float) fft_max);

    bool_val = settings->value("db_ranges_locked", false).toBool();
    ui->lockCheckBox->setChecked(bool_val);

    bool_val = settings->value("bandplan", false).toBool();
    ui->bandPlanCheckBox->setChecked(bool_val);
    emit bandPlanChanged(bool_val);

    bool_val = settings->value("markers", false).toBool();
    ui->markersCheckBox->setChecked(bool_val);
    emit markersChanged(bool_val);

    bool_val = settings->value("peak_detect", false).toBool();
    ui->peakDetectCheckBox->setChecked(bool_val);
    emit peakDetectToggled(bool_val);

    bool_val = settings->value("peak_hold", false).toBool();
    ui->maxHoldCheckBox->setChecked(bool_val);
    emit fftMaxHoldToggled(bool_val);

    bool_val = settings->value("min_hold", false).toBool();
    ui->minHoldCheckBox->setChecked(bool_val);
    emit fftMinHoldToggled(bool_val);

    QString cmap = settings->value("waterfall_colormap", "gqrx").toString();
    ui->cmapComboBox->setCurrentIndex(ui->cmapComboBox->findData(cmap));

    // FFT Zoom
    intval = settings->value("fft_zoom", DEFAULT_FFT_ZOOM).toInt(&conv_ok);
    if (conv_ok)
        ui->fftZoomSlider->setValue(intval);

    settings->endGroup();
}

void DockFft::setPandapterRange(float min, float max)
{
    ui->plotRangeSlider->blockSignals(true);
    ui->plotRangeSlider->setValues((int) min, (int) max);
    if (ui->lockCheckBox->isChecked())
        ui->wfRangeSlider->setValues((int) min, (int) max);
    m_pand_last_modified = true;
    ui->plotRangeSlider->blockSignals(false);
}

void DockFft::setWaterfallRange(float min, float max)
{
    ui->wfRangeSlider->blockSignals(true);
    ui->wfRangeSlider->setValues((int) min, (int) max);
    if (ui->lockCheckBox->isChecked())
        ui->plotRangeSlider->setValues((int) min, (int) max);
    m_pand_last_modified = false;
    ui->wfRangeSlider->blockSignals(false);
}

void DockFft::setZoomLevel(float level)
{
    ui->fftZoomSlider->blockSignals(true);
    ui->fftZoomSlider->setValue((int) level);
    ui->zoomLevelLabel->setText(QString("%1x").arg((int) level));
    ui->fftZoomSlider->blockSignals(false);
}

void DockFft::setMarkersEnabled(bool enable)
{
    ui->markersCheckBox->setCheckState(enable ? Qt::Checked : Qt::Unchecked);
}

void DockFft::setActualFrameRate(float rate, bool dropping)
{
    if (dropping) {
        ui->rateLabel->setText(">>> Rate");
        ui->rateLabel->setStyleSheet("QLabel { background-color : red; }");
    }
    else {
        ui->rateLabel->setText("Rate");
        ui->rateLabel->setStyleSheet("");
    }
}

/** FFT size changed. */
void DockFft::on_fftSizeComboBox_currentIndexChanged(int index)
{
    int value = ui->fftSizeComboBox->itemText(index).toInt();
    emit fftSizeChanged(value);
    updateInfoLabels();
}

/** FFT rate changed. */
void DockFft::on_fftRateComboBox_currentIndexChanged(int index)
{
    int fps = fftRate();
    Q_UNUSED(index);

    emit fftRateChanged(fps);
    updateInfoLabels();
}

void DockFft::on_fftWinComboBox_currentIndexChanged(int index)
{
    emit fftWindowChanged(index);
}

static const quint64 wf_span_table[] =
{
    0,              // Auto
    1*60*1000,      // 1 minute
    2*60*1000,      // 2 minutes
    5*60*1000,      // 5 minutes
    10*60*1000,     // 10 minutes
    15*60*1000,     // 15 minutes
    20*60*1000,     // 20 minutes
    30*60*1000,     // 30 minutes
    1*60*60*1000,   // 1 hour
    2*60*60*1000,   // 2 hours
    5*60*60*1000,   // 5 hours
    10*60*60*1000,  // 10 hours
    16*60*60*1000,  // 16 hours
    24*60*60*1000,  // 24 hours
    48*60*60*1000   // 48 hours
};

/** Waterfall time span changed. */
void DockFft::on_wfSpanComboBox_currentIndexChanged(int index)
{
    if (index < 0 || index > 14)
        return;

    emit wfSpanChanged(wf_span_table[index]);
}

/** Set waterfall time resolution. */
void DockFft::setWfResolution(quint64 msec_per_line)
{
    float res = 1.0e-3f * (float)msec_per_line;

    ui->wfResLabel->setText(QString("Res: %1 s").arg(res, 0, 'f', 2));
}

/**
 * @brief Split between waterfall and pandapter changed.
 * @param value The percentage of the waterfall.
 */
void DockFft::on_fftSplitSlider_valueChanged(int value)
{
    emit fftSplitChanged(value);
}

/** FFT filter gain changed. */
void DockFft::on_fftAvgSlider_valueChanged(int value)
{
    // Limit avg to < 1.0 here, since dockfft knows the max int value of the
    // slider.
    const float v = value;
    const float x = ui->fftAvgSlider->maximum();
    const float limit = 1.0f - 1.0f/x;
    const float avg = 1.0f - limit * v / x;

    emit fftAvgChanged(avg);
}

/** FFT zoom level changed */
void DockFft::on_fftZoomSlider_valueChanged(int level)
{
    ui->zoomLevelLabel->setText(QString("%1x").arg(level));
    emit fftZoomChanged((float)level);
}

void DockFft::on_wfModeBox_currentIndexChanged(int index)
{
    emit waterfallModeChanged(index);
}

void DockFft::on_plotModeBox_currentIndexChanged(int index)
{
    emit plotModeChanged(index);
}

void DockFft::on_plotScaleBox_currentIndexChanged(int index)
{
    // For dBFS and V (index 0, 1) "per" is N/A
    ui->plotPerSlashLabel->setVisible(index != 0);
    ui->plotPerBox->setVisible(index != 0);
    if (index == 1)
        ui->plotPerBox->setItemText(1, "√Hz");
    else if (index == 2)
        ui->plotPerBox->setItemText(1, "Hz");
    emit plotScaleChanged(index, ui->plotPerBox->currentIndex() == 1);
}

void DockFft::on_plotPerBox_currentIndexChanged(int index)
{
    emit plotScaleChanged(ui->plotScaleBox->currentIndex(), index == 1);
}

void DockFft::on_plotRangeSlider_valuesChanged(int min, int max)
{
    if (ui->lockCheckBox->isChecked())
        ui->wfRangeSlider->setValues(min, max);

    m_pand_last_modified = true;
    emit pandapterRangeChanged((float) min, (float) max);
}

void DockFft::on_wfRangeSlider_valuesChanged(int min, int max)
{
    if (ui->lockCheckBox->isChecked())
        ui->plotRangeSlider->setValues(min, max);

    m_pand_last_modified = false;
    emit waterfallRangeChanged((float) min, (float) max);
}

void DockFft::on_resetButton_clicked(void)
{
    ui->zoomLevelLabel->setText(QString("1x"));
    ui->fftZoomSlider->setValue(0);
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

/** FFT color has changed. */
void DockFft::on_colorPicker_colorChanged(const QColor &color)
{
    emit fftColorChanged(color);
}

/** FFT plot fill button toggled. */
void DockFft::on_fillCheckBox_stateChanged(int state)
{
    emit fftFillToggled(state == Qt::Checked);
}

/** peakHold button toggled */
void DockFft::on_maxHoldCheckBox_stateChanged(int state)
{
    emit fftMaxHoldToggled(state == Qt::Checked);
}

/** minHold button toggled */
void DockFft::on_minHoldCheckBox_stateChanged(int state)
{
    emit fftMinHoldToggled(state == Qt::Checked);
}

/** peakDetection button toggled */
void DockFft::on_peakDetectCheckBox_stateChanged(int state)
{
    emit peakDetectToggled(state == Qt::Checked);
}

void DockFft::on_bandPlanCheckBox_stateChanged(int state)
{
    emit bandPlanChanged(state == Qt::Checked);
}

void DockFft::on_markersCheckBox_stateChanged(int state)
{
    emit markersChanged(state == Qt::Checked);
}

/** lock button toggled */
void DockFft::on_lockCheckBox_stateChanged(int state)
{
    if (state == Qt::Checked)
    {
        if (m_pand_last_modified)
        {
            int min = ui->plotRangeSlider->minimumValue();
            int max = ui->plotRangeSlider->maximumValue();
            ui->wfRangeSlider->setPositions(min, max);
        }
        else
        {
            int min = ui->wfRangeSlider->minimumValue();
            int max = ui->wfRangeSlider->maximumValue();
            ui->plotRangeSlider->setPositions(min, max);
        }
    }
}

void DockFft::on_cmapComboBox_currentIndexChanged(int index)
{
    Q_UNUSED(index);
    emit wfColormapChanged(ui->cmapComboBox->currentData().toString());
}

/** Update RBW and FFT overlab labels */
void DockFft::updateInfoLabels(void)
{
    float   interval_ms;
    float   interval_samples;
    float   size;
    float   rbw;
    float   ovr;
    int     rate;

    if (m_sample_rate == 0.f)
        return;

    size = fftSize();

    rbw = m_sample_rate / size;
    if (rbw < 1.e3f)
        ui->fftRbwLabel->setText(QString("RBW: %1 Hz").arg(rbw, 0, 'f', 1));
    else if (rbw < 1.e6f)
        ui->fftRbwLabel->setText(QString("RBW: %1 kHz").arg(1.e-3f * rbw, 0, 'f', 1));
    else
        ui->fftRbwLabel->setText(QString("RBW: %1 MHz").arg(1.e-6f * rbw, 0, 'f', 1));

    rate = fftRate();
    if (rate == 0)
        ovr = 0;
    else
    {
        interval_ms = 1000 / rate;
        interval_samples = m_sample_rate * (interval_ms / 1000.0f);
        if (interval_samples >= size)
            ovr = 0;
        else
            ovr = 100 * (1.f - interval_samples / size);
    }
    ui->fftOvrLabel->setText(QString("Overlap: %1%").arg(ovr, 0, 'f', 0));
}
