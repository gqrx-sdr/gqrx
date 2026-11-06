/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2024 Gqrx Contributors.
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
#include <QString>

#include "qtgui/docktx.h"
#include "ui_docktx.h"

/* Modulation combo → tx_modulation enum mapping.
 * Must match the combo item order in docktx.ui:
 *   index 0 = NFM, 1 = AM, 2 = USB, 3 = LSB
 * which corresponds to transmitter::tx_modulation values 1,2,3,4. */
static const int s_mod_map[] = { 1, 2, 3, 4 }; /* NFM=1, AM=2, USB=3, LSB=4 */

DockTx::DockTx(QWidget *parent)
    : QDockWidget(parent),
      ui(new Ui::DockTx),
      d_txGainLayout(nullptr)
{
    ui->setupUi(this);

    /* The txGainLayout is defined in the .ui file; grab a reference to it */
    d_txGainLayout = qobject_cast<QGridLayout *>(
        ui->dockWidgetContents->layout()->itemAt(4)->layout());

    /* PTT button style sheet — green by default, red when active */
    ui->pttButton->setStyleSheet(
        "QPushButton { background-color: #3c3; color: white; font-weight: bold; }"
        "QPushButton:checked { background-color: #c33; color: white; font-weight: bold; }"
    );

    /* Keyboard shortcut F7 for PTT */
    ui->pttButton->setShortcut(QKeySequence(Qt::Key_F7));
}

DockTx::~DockTx()
{
    clearTxGainWidgets();
    delete ui;
}

/* -----------------------------------------------------------------------
 * Settings persistence
 * ----------------------------------------------------------------------- */

void DockTx::readSettings(QSettings *settings)
{
    bool ok;

    int mod = settings->value("transmitter/modulation", 0).toInt(&ok);
    if (ok && mod >= 0 && mod < ui->modCombo->count())
        ui->modCombo->setCurrentIndex(mod);

    int mic = settings->value("transmitter/mic_gain_db", 0).toInt(&ok);
    if (ok)
        ui->micGainSlider->setValue(mic);

    double offset = settings->value("transmitter/tx_offset_hz", 0.0).toDouble(&ok);
    if (ok)
        ui->txOffsetSpin->setValue(offset / 1000.0); /* stored as Hz, displayed as kHz */
}

void DockTx::saveSettings(QSettings *settings) const
{
    settings->setValue("transmitter/modulation",   ui->modCombo->currentIndex());
    settings->setValue("transmitter/mic_gain_db",  ui->micGainSlider->value());
    settings->setValue("transmitter/tx_offset_hz", ui->txOffsetSpin->value() * 1000.0);
}

/* -----------------------------------------------------------------------
 * Public interface
 * ----------------------------------------------------------------------- */

void DockTx::setTxEnabled(bool enabled)
{
    ui->dockWidgetContents->setEnabled(enabled);
}

void DockTx::setPttState(bool active)
{
    /* Block signals to avoid re-emitting pttToggled */
    bool blocked = ui->pttButton->blockSignals(true);
    ui->pttButton->setChecked(active);
    ui->pttButton->blockSignals(blocked);
}

int DockTx::modulation() const
{
    int idx = ui->modCombo->currentIndex();
    if (idx >= 0 && idx < 4)
        return s_mod_map[idx];
    return 1; /* default NFM */
}

float DockTx::micGain() const
{
    return static_cast<float>(ui->micGainSlider->value());
}

double DockTx::txOffset() const
{
    return ui->txOffsetSpin->value() * 1000.0; /* kHz → Hz */
}

/* -----------------------------------------------------------------------
 * TX hardware gain stages (device-dependent, populated at runtime)
 * ----------------------------------------------------------------------- */

void DockTx::setTxGainStages(gain_list_t &gains)
{
    clearTxGainWidgets();
    d_gains = gains;

    int row = 0;
    for (const gain_t &g : gains) {
        QString name = QString::fromStdString(g.name);

        QLabel  *lbl = new QLabel(name, this);
        QSlider *sld = new QSlider(Qt::Horizontal, this);
        QLabel  *val = new QLabel(this);

        /* Scale to integer slider steps */
        int steps = (g.step > 0.0) ? static_cast<int>((g.stop - g.start) / g.step) : 100;
        int cur   = (g.step > 0.0) ? static_cast<int>((g.value - g.start) / g.step) : 0;
        sld->setRange(0, steps);
        sld->setValue(cur);
        sld->setObjectName(name);

        double cur_dbl = g.start + cur * g.step;
        val->setText(QString("%1 dB").arg(cur_dbl, 0, 'f', 1));
        val->setMinimumWidth(55);
        val->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

        d_txGainLayout->addWidget(lbl, row, 0);
        d_txGainLayout->addWidget(sld, row, 1);
        d_txGainLayout->addWidget(val, row, 2);

        d_txGainSliders.append(sld);
        d_txGainLabels.append(lbl);
        d_txValueLabels.append(val);

        connect(sld, &QSlider::valueChanged, this, &DockTx::txGainSliderValueChanged);

        ++row;
    }
}

/* -----------------------------------------------------------------------
 * Private slots
 * ----------------------------------------------------------------------- */

void DockTx::on_pttButton_toggled(bool checked)
{
    emit pttToggled(checked);
}

void DockTx::on_modCombo_currentIndexChanged(int index)
{
    if (index >= 0 && index < 4)
        emit modulationChanged(s_mod_map[index]);
}

void DockTx::on_micGainSlider_valueChanged(int value)
{
    updateMicLabel(value);
    emit micGainChanged(static_cast<float>(value));
}

void DockTx::on_txOffsetSpin_valueChanged(double value)
{
    emit txOffsetChanged(value * 1000.0); /* kHz → Hz */
}

void DockTx::txGainSliderValueChanged(int value)
{
    QSlider *sld = qobject_cast<QSlider *>(sender());
    if (!sld)
        return;

    QString name = sld->objectName();
    int idx = d_txGainSliders.indexOf(sld);

    double gain_val = 0.0;
    if (idx >= 0 && idx < static_cast<int>(d_gains.size())) {
        const gain_t &g = d_gains[static_cast<size_t>(idx)];
        gain_val = g.start + value * g.step;
        d_txValueLabels[idx]->setText(QString("%1 dB").arg(gain_val, 0, 'f', 1));
    }

    emit txGainChanged(name, gain_val);
}

/* -----------------------------------------------------------------------
 * Private helpers
 * ----------------------------------------------------------------------- */

void DockTx::clearTxGainWidgets()
{
    for (QSlider *s : d_txGainSliders) delete s;
    for (QLabel  *l : d_txGainLabels)  delete l;
    for (QLabel  *l : d_txValueLabels) delete l;
    d_txGainSliders.clear();
    d_txGainLabels.clear();
    d_txValueLabels.clear();
    d_gains.clear();
}

void DockTx::updateMicLabel(int slider_value)
{
    ui->micValueLabel->setText(QString("%1 dB").arg(slider_value, 3));
}

