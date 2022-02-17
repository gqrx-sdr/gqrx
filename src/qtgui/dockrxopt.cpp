/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
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
#include <QDebug>
#include <QVariant>
#include <QShortcut>
#include <iostream>
#include "dockrxopt.h"
#include "ui_dockrxopt.h"

DockRxOpt::DockRxOpt(qint64 filterOffsetRange, QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::DockRxOpt),
    agc_is_on(true),
    hw_freq_hz(144500000)
{
    ui->setupUi(this);

    ui->modeSelector->addItems(Modulations::Strings);
    freqLockButtonMenu = new QMenu(this);
    // MenuItem Lock all
    {
        QAction* action = new QAction("Lock all", this);
        freqLockButtonMenu->addAction(action);
        connect(action, SIGNAL(triggered()), this, SLOT(menuFreqLockAll()));
    }
    // MenuItem Unlock all
    {
        QAction* action = new QAction("Unlock all", this);
        freqLockButtonMenu->addAction(action);
        connect(action, SIGNAL(triggered()), this, SLOT(menuFreqUnlockAll()));
    }
    ui->freqLockButton->setContextMenuPolicy(Qt::CustomContextMenu);
    squelchButtonMenu = new QMenu(this);
    // MenuItem Auto all
    {
        QAction* action = new QAction("AUTO all", this);
        squelchButtonMenu->addAction(action);
        connect(action, SIGNAL(triggered()), this, SLOT(menuSquelchAutoAll()));
    }
    // MenuItem Reset all
    {
        QAction* action = new QAction("Reset all", this);
        squelchButtonMenu->addAction(action);
        connect(action, SIGNAL(triggered()), this, SLOT(menuSquelchResetAll()));
    }
    ui->autoSquelchButton->setContextMenuPolicy(Qt::CustomContextMenu);

    ui->filterFreq->setup(7, -filterOffsetRange/2, filterOffsetRange/2, 1,
                          FCTL_UNIT_KHZ);
    ui->filterFreq->setFrequency(0);

    // use same slot for filteCombo and filterShapeCombo
    connect(ui->filterShapeCombo, SIGNAL(activated(int)), this, SLOT(on_filterCombo_activated(int)));

    // demodulator options dialog
    demodOpt = new CDemodOptions(this);
    demodOpt->setCurrentPage(CDemodOptions::PAGE_FM_OPT);
    connect(demodOpt, SIGNAL(fmMaxdevSelected(float)), this, SLOT(demodOpt_fmMaxdevSelected(float)));
    connect(demodOpt, SIGNAL(fmEmphSelected(double)), this, SLOT(demodOpt_fmEmphSelected(double)));
    connect(demodOpt, SIGNAL(amDcrToggled(bool)), this, SLOT(demodOpt_amDcrToggled(bool)));
    connect(demodOpt, SIGNAL(cwOffsetChanged(int)), this, SLOT(demodOpt_cwOffsetChanged(int)));
    connect(demodOpt, SIGNAL(amSyncDcrToggled(bool)), this, SLOT(demodOpt_amSyncDcrToggled(bool)));
    connect(demodOpt, SIGNAL(amSyncPllBwSelected(float)), this, SLOT(demodOpt_amSyncPllBwSelected(float)));

    // AGC options dialog
    agcOpt = new CAgcOptions(this);
    connect(agcOpt, SIGNAL(maxGainChanged(int)), this, SLOT(agcOpt_maxGainChanged(int)));
    connect(agcOpt, SIGNAL(targetLevelChanged(int)), this, SLOT(agcOpt_targetLevelChanged(int)));
    connect(agcOpt, SIGNAL(attackChanged(int)), this, SLOT(agcOpt_attackChanged(int)));
    connect(agcOpt, SIGNAL(decayChanged(int)), this, SLOT(agcOpt_decayChanged(int)));
    connect(agcOpt, SIGNAL(hangChanged(int)), this, SLOT(agcOpt_hangChanged(int)));
    connect(agcOpt, SIGNAL(panningChanged(int)), this, SLOT(agcOpt_panningChanged(int)));
    connect(agcOpt, SIGNAL(panningAutoChanged(bool)), this, SLOT(agcOpt_panningAutoChanged(bool)));

    // Noise blanker options
    nbOpt = new CNbOptions(this);
    connect(nbOpt, SIGNAL(thresholdChanged(int,double)), this, SLOT(nbOpt_thresholdChanged(int,double)));

    /* mode setting shortcuts */
    QShortcut *mode_off_shortcut = new QShortcut(QKeySequence(Qt::Key_Exclam), this);
    QShortcut *mode_raw_shortcut = new QShortcut(QKeySequence(Qt::Key_I), this);
    QShortcut *mode_am_shortcut = new QShortcut(QKeySequence(Qt::Key_A), this);
    QShortcut *mode_nfm_shortcut = new QShortcut(QKeySequence(Qt::Key_N), this);
    QShortcut *mode_wfm_mono_shortcut = new QShortcut(QKeySequence(Qt::Key_W), this);
    QShortcut *mode_wfm_stereo_shortcut = new QShortcut(QKeySequence(Qt::SHIFT | Qt::Key_W), this);
    QShortcut *mode_lsb_shortcut = new QShortcut(QKeySequence(Qt::Key_S), this);
    QShortcut *mode_usb_shortcut = new QShortcut(QKeySequence(Qt::SHIFT | Qt::Key_S), this);
    QShortcut *mode_cwl_shortcut = new QShortcut(QKeySequence(Qt::Key_C), this);
    QShortcut *mode_cwu_shortcut = new QShortcut(QKeySequence(Qt::SHIFT | Qt::Key_C), this);
    QShortcut *mode_wfm_oirt_shortcut = new QShortcut(QKeySequence(Qt::Key_O), this);
    QShortcut *mode_am_sync_shortcut = new QShortcut(QKeySequence(Qt::SHIFT | Qt::Key_A), this);

    QObject::connect(mode_off_shortcut, &QShortcut::activated, this, &DockRxOpt::modeOffShortcut);
    QObject::connect(mode_raw_shortcut, &QShortcut::activated, this, &DockRxOpt::modeRawShortcut);
    QObject::connect(mode_am_shortcut, &QShortcut::activated, this, &DockRxOpt::modeAMShortcut);
    QObject::connect(mode_nfm_shortcut, &QShortcut::activated, this, &DockRxOpt::modeNFMShortcut);
    QObject::connect(mode_wfm_mono_shortcut, &QShortcut::activated, this, &DockRxOpt::modeWFMmonoShortcut);
    QObject::connect(mode_wfm_stereo_shortcut, &QShortcut::activated, this, &DockRxOpt::modeWFMstereoShortcut);
    QObject::connect(mode_lsb_shortcut, &QShortcut::activated, this, &DockRxOpt::modeLSBShortcut);
    QObject::connect(mode_usb_shortcut, &QShortcut::activated, this, &DockRxOpt::modeUSBShortcut);
    QObject::connect(mode_cwl_shortcut, &QShortcut::activated, this, &DockRxOpt::modeCWLShortcut);
    QObject::connect(mode_cwu_shortcut, &QShortcut::activated, this, &DockRxOpt::modeCWUShortcut);
    QObject::connect(mode_wfm_oirt_shortcut, &QShortcut::activated, this, &DockRxOpt::modeWFMoirtShortcut);
    QObject::connect(mode_am_sync_shortcut, &QShortcut::activated, this, &DockRxOpt::modeAMsyncShortcut);

    /* squelch shortcuts */
    QShortcut *squelch_reset_shortcut = new QShortcut(QKeySequence(Qt::Key_QuoteLeft), this);
    QShortcut *squelch_auto_shortcut = new QShortcut(QKeySequence(Qt::Key_AsciiTilde), this);

    QObject::connect(squelch_reset_shortcut, &QShortcut::activated, this, &DockRxOpt::on_resetSquelchButton_clicked);
    QObject::connect(squelch_auto_shortcut, &QShortcut::activated, this, &DockRxOpt::on_autoSquelchButton_clicked);

    /* filter width shortcuts */
    QShortcut *filter_narrow_shortcut = new QShortcut(QKeySequence(Qt::Key_Less), this);
    QShortcut *filter_normal_shortcut = new QShortcut(QKeySequence(Qt::Key_Period), this);
    QShortcut *filter_wide_shortcut = new QShortcut(QKeySequence(Qt::Key_Greater), this);

    QObject::connect(filter_narrow_shortcut, &QShortcut::activated, this, &DockRxOpt::filterNarrowShortcut);
    QObject::connect(filter_normal_shortcut, &QShortcut::activated, this, &DockRxOpt::filterNormalShortcut);
    QObject::connect(filter_wide_shortcut, &QShortcut::activated, this, &DockRxOpt::filterWideShortcut);
}

DockRxOpt::~DockRxOpt()
{
    delete ui;
    delete demodOpt;
    delete agcOpt;
    delete nbOpt;
}

/**
 * @brief Set value of channel filter offset selector.
 * @param freq_hz The frequency in Hz
 */
void DockRxOpt::setFilterOffset(qint64 freq_hz)
{
    ui->filterFreq->setFrequency(freq_hz);
}

/**
 * @brief Set filter offset range.
 * @param range_hz The new range in Hz.
 */
void DockRxOpt::setFilterOffsetRange(qint64 range_hz)
{
    int num_digits;

    if (range_hz <= 0)
        return;

    range_hz /= 2;
    if (range_hz < 100e3)
        num_digits = 5;
    else if (range_hz < 1e6)
        num_digits = 6;
    else if (range_hz < 1e7)
        num_digits = 7;
    else if (range_hz < 1e8)
        num_digits = 8;
    else
        num_digits = 9;

    ui->filterFreq->setup(num_digits, -range_hz, range_hz, 1, FCTL_UNIT_KHZ);
}

/**
 * @brief Set new RF frequency
 * @param freq_hz The frequency in Hz
 *
 * RF frequency is the frequency to which the device device is tuned to
 * The actual RX frequency is the sum of the RF frequency and the filter
 * offset.
 */
void DockRxOpt::setHwFreq(qint64 freq_hz)
{
    hw_freq_hz = freq_hz;
    updateHwFreq();
}

/** Update RX frequency label. */
void DockRxOpt::updateHwFreq()
{
    double hw_freq_mhz = hw_freq_hz / 1.0e6;
    ui->hwFreq->setText(QString("%1 MHz").arg(hw_freq_mhz, 11, 'f', 6, ' '));
}

/**
 * Get filter index from filter LO / HI values.
 * @param lo The filter low cut frequency.
 * @param hi The filter high cut frequency.
 *
 * Given filter low and high cut frequencies, this function checks whether the
 * filter settings correspond to one of the presets in filter_preset_table and
 * returns the corresponding index to ui->filterCombo;
 */
unsigned int DockRxOpt::filterIdxFromLoHi(int lo, int hi) const
{
    Modulations::idx mode_index = Modulations::idx(ui->modeSelector->currentIndex());
    return Modulations::FindFilterPreset(mode_index, lo, hi);
}

/**
 * @brief Set filter parameters
 * @param lo Low cutoff frequency in Hz
 * @param hi High cutoff frequency in Hz.
 *
 * This function will automatically select the "User" preset in the
 * combo box.
 */
void DockRxOpt::setFilterParam(int lo, int hi)
{
    int filter_index = filterIdxFromLoHi(lo, hi);

    ui->filterCombo->setCurrentIndex(filter_index);
    if (filter_index == FILTER_PRESET_USER)
    {
        float width_f;
        width_f = fabs((hi-lo)/1000.f);
        ui->filterCombo->setItemText(FILTER_PRESET_USER, QString("User (%1 k)")
                                     .arg((double)width_f));
    }
}

/**
 * @brief Select new filter preset.
 * @param index Index of the new filter preset (0=wide, 1=normal, 2=narrow).
 */
void DockRxOpt::setCurrentFilter(int index)
{
    ui->filterCombo->setCurrentIndex(index);
}

/**
 * @brief Get current filter preset.
 * @param The current filter preset (0=wide, 1=normal, 2=narrow).
 */
int  DockRxOpt::currentFilter() const
{
    return ui->filterCombo->currentIndex();
}

/** Select filter shape */
void DockRxOpt::setCurrentFilterShape(int index)
{
    ui->filterShapeCombo->setCurrentIndex(index);
}

int  DockRxOpt::currentFilterShape() const
{
    return ui->filterShapeCombo->currentIndex();
}

/**
 * @brief Select new demodulator.
 * @param demod Demodulator index corresponding to receiver::demod.
 */
void DockRxOpt::setCurrentDemod(Modulations::idx demod)
{
    if ((demod >= Modulations::MODE_OFF) && (demod < Modulations::MODE_LAST))
    {
        ui->modeSelector->setCurrentIndex(demod);
        updateDemodOptPage(demod);
    }
}

/**
 * @brief Get current demodulator selection.
 * @return The current demodulator corresponding to receiver::demod.
 */
Modulations::idx DockRxOpt::currentDemod() const
{
    return Modulations::idx(ui->modeSelector->currentIndex());
}

QString DockRxOpt::currentDemodAsString()
{
    return Modulations::GetStringForModulationIndex(currentDemod());
}

float DockRxOpt::currentMaxdev() const
{
    return demodOpt->getMaxDev();
}

double DockRxOpt::currentEmph() const
{
    return demodOpt->getEmph();
}

/**
 * @brief Set squelch level.
 * @param level Squelch level in dBFS
 */
void DockRxOpt::setSquelchLevel(double level)
{
    ui->sqlSpinBox->setValue(level);
}

double DockRxOpt::getSqlLevel(void) const
{
    return ui->sqlSpinBox->value();
}

/**
 * @brief Get the current squelch level
 * @returns The current squelch setting in dBFS
 */
double DockRxOpt::currentSquelchLevel() const
{
    return ui->sqlSpinBox->value();
}

int DockRxOpt::getCwOffset() const
{
    return demodOpt->getCwOffset();
}

void DockRxOpt::setCwOffset(int offset)
{
    demodOpt->setCwOffset(offset);
}

/** Get agc settings */
bool DockRxOpt::getAgcOn()
{
    return agc_is_on;
}

void DockRxOpt::setAgcOn(bool on)
{
    if (on)
        setAgcPresetFromParams(getAgcDecay());
    else
        ui->agcPresetCombo->setCurrentIndex(4);
    agc_is_on = on;
}

int DockRxOpt::getAgcTargetLevel()
{
    return agcOpt->targetLevel();
}

void DockRxOpt::setAgcTargetLevel(int level)
{
    agcOpt->setTargetLevel(level);
}

int DockRxOpt::getAgcMaxGain()
{
    return agcOpt->maxGain();
}

void DockRxOpt::setAgcMaxGain(int gain)
{
    agcOpt->setMaxGain(gain);
}

int DockRxOpt::getAgcAttack()
{
    return agcOpt->attack();
}

void DockRxOpt::setAgcAttack(int attack)
{
    agcOpt->setAttack(attack);
}

int DockRxOpt::getAgcDecay()
{
    return agcOpt->decay();
}

void DockRxOpt::setAgcDecay(int decay)
{
    agcOpt->setDecay(decay);
    setAgcOn(agc_is_on);
}

int DockRxOpt::getAgcHang()
{
    return agcOpt->hang();
}

void DockRxOpt::setAgcHang(int hang)
{
    agcOpt->setHang(hang);
}

int  DockRxOpt::getAgcPanning()
{
    return agcOpt->panning();
}

void DockRxOpt::setAgcPanning(int panning)
{
    agcOpt->setPanning(panning);
}

bool DockRxOpt::getAgcPanningAuto()
{
    return agcOpt->panningAuto();
}

void DockRxOpt::setAgcPanningAuto(bool panningAuto)
{
    agcOpt->setPanningAuto(panningAuto);
}

void DockRxOpt::setAgcPresetFromParams(int decay)
{
    if (decay == 100)
        ui->agcPresetCombo->setCurrentIndex(0);
    else if (decay == 500)
        ui->agcPresetCombo->setCurrentIndex(1);
    else if (decay == 2000)
        ui->agcPresetCombo->setCurrentIndex(2);
    else
        ui->agcPresetCombo->setCurrentIndex(3);
}

void DockRxOpt::setAmDcr(bool on)
{
    demodOpt->setAmDcr(on);
}

void DockRxOpt::setAmSyncDcr(bool on)
{
    demodOpt->setAmSyncDcr(on);
}

void DockRxOpt::setAmSyncPllBw(float bw)
{
    demodOpt->setPllBw(bw);
}

void DockRxOpt::setFmMaxdev(float max_hz)
{
    demodOpt->setMaxDev(max_hz);
}

void DockRxOpt::setFmEmph(double tau)
{
    demodOpt->setEmph(tau);
}

void DockRxOpt::setNoiseBlanker(int nbid, bool on, float threshold)
{
    if (nbid == 1)
        ui->nb1Button->setChecked(on);
    else
        ui->nb2Button->setChecked(on);
    nbOpt->setNbThreshold(nbid, threshold);
}

void DockRxOpt::setFreqLock(bool lock)
{
    ui->freqLockButton->setChecked(lock);
}

bool DockRxOpt::getFreqLock()
{
    return ui->freqLockButton->isChecked();
}

/** RX frequency changed through spin box */
void DockRxOpt::on_freqSpinBox_valueChanged(double freq)
{
    emit rxFreqChanged(1.e3 * freq);
}

void DockRxOpt::setRxFreq(qint64 freq_hz)
{
    ui->freqSpinBox->blockSignals(true);
    ui->freqSpinBox->setValue(1.e-3 * (double)freq_hz);
    ui->freqSpinBox->blockSignals(false);
}

void DockRxOpt::setRxFreqRange(qint64 min_hz, qint64 max_hz)
{
    ui->freqSpinBox->blockSignals(true);
    ui->freqSpinBox->setRange(1.e-3 * (double)min_hz, 1.e-3 * (double)max_hz);
    ui->freqSpinBox->blockSignals(false);
}

void DockRxOpt::setResetLowerDigits(bool enabled)
{
    ui->filterFreq->setResetLowerDigits(enabled);
}

void DockRxOpt::setInvertScrolling(bool enabled)
{
    ui->filterFreq->setInvertScrolling(enabled);
}

/**
 * @brief Channel filter offset has changed
 * @param freq The new filter offset in Hz
 *
 * This slot is activated when a new filter offset has been selected either
 * using the mouse or using the keyboard.
 */
void DockRxOpt::on_filterFreq_newFrequency(qint64 freq)
{
    updateHwFreq();

    emit filterOffsetChanged(freq);
}

/**
 * New filter preset selected.
 *
 * Instead of implementing a new signal, we simply emit demodSelected() since
 * demodulator and filter preset are tightly coupled.
 */
void DockRxOpt::on_filterCombo_activated(int index)
{
    Q_UNUSED(index);

    qDebug() << "New filter preset:" << ui->filterCombo->currentText();
    qDebug() << "            shape:" << ui->filterShapeCombo->currentIndex();
    emit demodSelected(Modulations::idx(ui->modeSelector->currentIndex()));
}

/**
 * @brief Mode selector activated.
 * @param New mode selection.
 *
 * This slot is activated when the user selects a new demodulator (mode change).
 * It is connected automatically by the UI constructor, and it emits the demodSelected()
 * signal.
 *
 * Note that the modes listed in the selector are different from those defined by
 * receiver::demod (we want to list LSB/USB separately but they have identical demods).
 */
void DockRxOpt::on_modeSelector_activated(int index)
{
    updateDemodOptPage(Modulations::idx(index));
    emit demodSelected(Modulations::idx(index));
}

void DockRxOpt::updateDemodOptPage(Modulations::idx demod)
{
    // update demodulator option widget
    if (demod == Modulations::MODE_NFM)
        demodOpt->setCurrentPage(CDemodOptions::PAGE_FM_OPT);
    else if (demod == Modulations::MODE_AM)
        demodOpt->setCurrentPage(CDemodOptions::PAGE_AM_OPT);
    else if (demod == Modulations::MODE_CWL || demod == Modulations::MODE_CWU)
        demodOpt->setCurrentPage(CDemodOptions::PAGE_CW_OPT);
    else if (demod == Modulations::MODE_AM_SYNC)
        demodOpt->setCurrentPage(CDemodOptions::PAGE_AMSYNC_OPT);
    else
        demodOpt->setCurrentPage(CDemodOptions::PAGE_NO_OPT);
}

/** Show demodulator options. */
void DockRxOpt::on_modeButton_clicked()
{
    demodOpt->show();
}

/** Show AGC options. */
void DockRxOpt::on_agcButton_clicked()
{
    agcOpt->show();
}

/**
 * @brief Auto-squelch button clicked.
 *
 * This slot is called when the user clicks on the auto-squelch button.
 */
void DockRxOpt::on_autoSquelchButton_clicked()
{
    double newval = sqlAutoClicked(false); // FIXME: We rely on signal only being connected to one slot
    ui->sqlSpinBox->setValue(newval);
}

void DockRxOpt::on_autoSquelchButton_customContextMenuRequested(const QPoint& pos)
{
    squelchButtonMenu->popup(ui->autoSquelchButton->mapToGlobal(pos));
}

void DockRxOpt::menuSquelchAutoAll()
{
    double newval = sqlAutoClicked(true); // FIXME: We rely on signal only being connected to one slot
    ui->sqlSpinBox->setValue(newval);
}

void DockRxOpt::on_resetSquelchButton_clicked()
{
    ui->sqlSpinBox->setValue(-150.0);
}

void DockRxOpt::menuSquelchResetAll()
{
    ui->sqlSpinBox->setValue(-150.0);
    emit sqlResetAllClicked();
}

/** AGC preset has changed. */
void DockRxOpt::on_agcPresetCombo_currentIndexChanged(int index)
{
    CAgcOptions::agc_preset_e preset = (CAgcOptions::agc_preset_e) index;

    switch (preset)
    {
    case CAgcOptions::AGC_FAST:
    case CAgcOptions::AGC_MEDIUM:
    case CAgcOptions::AGC_SLOW:
    case CAgcOptions::AGC_USER:
        if (!agc_is_on)
        {
            emit agcToggled(true);
            agc_is_on = true;
        }
        agcOpt->setPreset(preset);
        break;

    case CAgcOptions::AGC_OFF:
        if (agc_is_on)
        {
            emit agcToggled(false);
            agc_is_on = false;
        }
        agcOpt->setPreset(preset);
        break;

    default:
        qDebug() << "Invalid AGC preset:" << index;
    }
}

/**
 * @brief AGC hang time changed.
 * @param value The new AGC hang time in ms.
 */
void DockRxOpt::agcOpt_hangChanged(int value)
{
    emit agcHangChanged(value);
}

/**
 * @brief AGC target level changed.
 * @param value The new AGC target level in dB.
 */
void DockRxOpt::agcOpt_targetLevelChanged(int value)
{
    emit agcTargetLevelChanged(value);
}

/**
 * @brief AGC attack changed.
 * @param value The new attack rate in ms (tbc).
 */
void DockRxOpt::agcOpt_attackChanged(int value)
{
    emit agcAttackChanged(value);
}

/**
 * @brief AGC decay changed.
 * @param value The new decay rate in ms (tbc).
 */
void DockRxOpt::agcOpt_decayChanged(int value)
{
    emit agcDecayChanged(value);
}

/**
 * @brief AGC maimum gain changed.
 * @param gain The new gain in dB.
 */
void DockRxOpt::agcOpt_maxGainChanged(int gain)
{
    emit agcMaxGainChanged(gain);
}

/**
 * @brief AGC panning changed.
 * @param value The new relative panning position.
 */
void DockRxOpt::agcOpt_panningChanged(int value)
{
    emit agcPanningChanged(value);
}

/**
 * @brief AGC panning auto mode changed.
 * @param value The new auto mode state.
 */
void DockRxOpt::agcOpt_panningAutoChanged(bool value)
{
    emit agcPanningAuto(value);
}

/**
 * @brief Squelch level change.
 * @param value The new squelch level in dB.
 */
void DockRxOpt::on_sqlSpinBox_valueChanged(double value)
{
    emit sqlLevelChanged(value);
}

/**
 * @brief FM deviation changed by user.
 * @param max_dev The new deviation in Hz.
 */
void DockRxOpt::demodOpt_fmMaxdevSelected(float max_dev)
{
    emit fmMaxdevSelected(max_dev);
}

/**
 * @brief FM de-emphasis changed by user.
 * @param tau The new time constant in uS.
 */
void DockRxOpt::demodOpt_fmEmphSelected(double tau)
{
    emit fmEmphSelected(tau);
}

/**
 * @brief AM DC removal toggled by user.
 * @param enabled Whether DCR is enabled or not.
 */
void DockRxOpt::demodOpt_amDcrToggled(bool enabled)
{
    emit amDcrToggled(enabled);
}

void DockRxOpt::demodOpt_cwOffsetChanged(int offset)
{
    emit cwOffsetChanged(offset);
}

/**
 * @brief AM-Sync DC removal toggled by user.
 * @param enabled Whether DCR is enabled or not.
 */
void DockRxOpt::demodOpt_amSyncDcrToggled(bool enabled)
{
    emit amSyncDcrToggled(enabled);
}

/**
 * @brief AM-Sync PLL BW changed by user.
 * @param pll_bw The new PLL BW.
 */
void DockRxOpt::demodOpt_amSyncPllBwSelected(float pll_bw)
{
    emit amSyncPllBwSelected(pll_bw);
}

/** Noise blanker 1 button has been toggled. */
void DockRxOpt::on_nb1Button_toggled(bool checked)
{
    emit noiseBlankerChanged(1, checked, (float) nbOpt->nbThreshold(1));
}

/** Noise blanker 2 button has been toggled. */
void DockRxOpt::on_nb2Button_toggled(bool checked)
{
    emit noiseBlankerChanged(2, checked, (float) nbOpt->nbThreshold(2));
}

void DockRxOpt::on_freqLockButton_clicked()
{
    emit freqLock(ui->freqLockButton->isChecked(), false);
}

void DockRxOpt::on_freqLockButton_customContextMenuRequested(const QPoint& pos)
{
    freqLockButtonMenu->popup(ui->freqLockButton->mapToGlobal(pos));
}

void DockRxOpt::menuFreqLockAll()
{
    emit freqLock(true, true);
    ui->freqLockButton->setChecked(true);
}

void DockRxOpt::menuFreqUnlockAll()
{
    emit freqLock(false, true);
    ui->freqLockButton->setChecked(false);
}

/** Noise blanker threshold has been changed. */
void DockRxOpt::nbOpt_thresholdChanged(int nbid, double value)
{
    if (nbid == 1)
        emit noiseBlankerChanged(nbid, ui->nb1Button->isChecked(), (float) value);
    else
        emit noiseBlankerChanged(nbid, ui->nb2Button->isChecked(), (float) value);
}

void DockRxOpt::on_nbOptButton_clicked()
{
    nbOpt->show();
}

void DockRxOpt::modeOffShortcut() {
    on_modeSelector_activated(Modulations::MODE_OFF);
}

void DockRxOpt::modeRawShortcut() {
    on_modeSelector_activated(Modulations::MODE_RAW);
}

void DockRxOpt::modeAMShortcut() {
    on_modeSelector_activated(Modulations::MODE_AM);
}

void DockRxOpt::modeNFMShortcut() {
    on_modeSelector_activated(Modulations::MODE_NFM);
}

void DockRxOpt::modeWFMmonoShortcut() {
    on_modeSelector_activated(Modulations::MODE_WFM_MONO);
}

void DockRxOpt::modeWFMstereoShortcut() {
    on_modeSelector_activated(Modulations::MODE_WFM_STEREO);
}

void DockRxOpt::modeLSBShortcut() {
    on_modeSelector_activated(Modulations::MODE_LSB);
}

void DockRxOpt::modeUSBShortcut() {
    on_modeSelector_activated(Modulations::MODE_USB);
}

void DockRxOpt::modeCWLShortcut() {
    on_modeSelector_activated(Modulations::MODE_CWL);
}

void DockRxOpt::modeCWUShortcut() {
    on_modeSelector_activated(Modulations::MODE_CWU);
}

void DockRxOpt::modeWFMoirtShortcut() {
    on_modeSelector_activated(Modulations::MODE_WFM_STEREO_OIRT);
}

void DockRxOpt::modeAMsyncShortcut() {
    on_modeSelector_activated(Modulations::MODE_AM_SYNC);
}

void DockRxOpt::filterNarrowShortcut() {
    setCurrentFilter(FILTER_PRESET_NARROW);
    on_filterCombo_activated(FILTER_PRESET_NARROW);
}

void DockRxOpt::filterNormalShortcut() {
    setCurrentFilter(FILTER_PRESET_NORMAL);
    on_filterCombo_activated(FILTER_PRESET_NORMAL);
}

void DockRxOpt::filterWideShortcut() {
    setCurrentFilter(FILTER_PRESET_WIDE);
    on_filterCombo_activated(FILTER_PRESET_WIDE);
}
