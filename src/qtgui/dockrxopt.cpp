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

#include <iostream>

#include "dockrxopt.h"
#include "ui_dockrxopt.h"

QStringList DockRxOpt::ModulationStrings;

// Lookup table for conversion from old settings
static const int old2new[] = {
    DockRxOpt::MODE_OFF,
    DockRxOpt::MODE_RAW,
    DockRxOpt::MODE_AM,
    DockRxOpt::MODE_NFM,
    DockRxOpt::MODE_WFM_MONO,
    DockRxOpt::MODE_WFM_STEREO,
    DockRxOpt::MODE_LSB,
    DockRxOpt::MODE_USB,
    DockRxOpt::MODE_CWL,
    DockRxOpt::MODE_CWU,
    DockRxOpt::MODE_WFM_STEREO_OIRT,
    DockRxOpt::MODE_AM_SYNC
};

// Filter preset table per mode, preset and lo/hi
static const int filter_preset_table[DockRxOpt::MODE_LAST][3][2] =
{   //     WIDE             NORMAL            NARROW
    {{      0,      0}, {     0,     0}, {     0,     0}},  // MODE_OFF
    {{ -15000,  15000}, { -5000,  5000}, { -1000,  1000}},  // MODE_RAW
    {{ -10000,  10000}, { -5000,  5000}, { -2500,  2500}},  // MODE_AM
    {{ -10000,  10000}, { -5000,  5000}, { -2500,  2500}},  // MODE_AMSYNC
    {{  -4000,   -100}, { -2800,  -100}, { -2400,  -300}},  // MODE_LSB
    {{    100,   4000}, {   100,  2800}, {   300,  2400}},  // MODE_USB
    {{  -1000,   1000}, {  -250,   250}, {  -100,   100}},  // MODE_CWL
    {{  -1000,   1000}, {  -250,   250}, {  -100,   100}},  // MODE_CWU
    {{ -10000,  10000}, { -5000,  5000}, { -2500,  2500}},  // MODE_NFM
    {{-100000, 100000}, {-80000, 80000}, {-60000, 60000}},  // MODE_WFM_MONO
    {{-100000, 100000}, {-80000, 80000}, {-60000, 60000}},  // MODE_WFM_STEREO
    {{-100000, 100000}, {-80000, 80000}, {-60000, 60000}}   // MODE_WFM_STEREO_OIRT
};

DockRxOpt::DockRxOpt(qint64 filterOffsetRange, QWidget *parent) :
    QFrame(parent),
    ui(new Ui::DockRxOpt),
    agc_is_on(true),
    hw_freq_hz(144500000)
{
    ui->setupUi(this);

    if (ModulationStrings.size() == 0)
    {
        // Keep in sync with rxopt_mode_idx and filter_preset_table
        ModulationStrings.append("Demod Off");
        ModulationStrings.append("Raw I/Q");
        ModulationStrings.append("AM");
        ModulationStrings.append("AM-Sync");
        ModulationStrings.append("LSB");
        ModulationStrings.append("USB");
        ModulationStrings.append("CW-L");
        ModulationStrings.append("CW-U");
        ModulationStrings.append("Narrow FM");
        ModulationStrings.append("WFM (mono)");
        ModulationStrings.append("WFM (stereo)");
        ModulationStrings.append("WFM (oirt)");
    }
    ui->modeSelector->addItems(ModulationStrings);

#ifdef Q_OS_LINUX
    ui->modeButton->setMinimumSize(32, 24);
    ui->agcButton->setMinimumSize(32, 24);
    ui->autoSquelchButton->setMinimumSize(32, 24);
    ui->resetSquelchButton->setMinimumSize(32, 24);
    ui->nbOptButton->setMinimumSize(32, 24);
    ui->nb2Button->setMinimumSize(32, 24);
    ui->nb1Button->setMinimumSize(32, 24);
#endif

    connect(ui->actionRemoveDemodulator, SIGNAL(triggered()), this, SIGNAL(remove()));
    connect(ui->actionAddBookmark, SIGNAL(triggered()), this, SIGNAL(bookmark()));
    connect(ui->actionCenterFFT, SIGNAL(triggered()), this, SIGNAL(centerFFT()));

    // use same slot for filterCombo and filterShapeCombo
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
    connect(agcOpt, SIGNAL(gainChanged(int)), this, SLOT(agcOpt_gainChanged(int)));
    connect(agcOpt, SIGNAL(thresholdChanged(int)), this, SLOT(agcOpt_thresholdChanged(int)));
    connect(agcOpt, SIGNAL(decayChanged(int)), this, SLOT(agcOpt_decayChanged(int)));
    connect(agcOpt, SIGNAL(slopeChanged(int)), this, SLOT(agcOpt_slopeChanged(int)));
    connect(agcOpt, SIGNAL(hangChanged(bool)), this, SLOT(agcOpt_hangToggled(bool)));

    // Noise blanker options
    nbOpt = new CNbOptions(this);
    connect(nbOpt, SIGNAL(thresholdChanged(int,double)), this, SLOT(nbOpt_thresholdChanged(int,double)));
}

/**
 * @brief DockRxOpt::setupShortcuts - keyboard shortcuts
 */
void DockRxOpt::setupShortcuts(const size_t idx)
{
    // Remove any existing shortcuts
    removeShortcuts();

    // pre-select demod with a key combo
    int dkey = -1;
    switch (idx) {
    case 0:
        dkey = Qt::CTRL + Qt::Key_1;
        break;
    case 1:
        dkey = Qt::CTRL + Qt::Key_2;
        break;
    case 2:
        dkey = Qt::CTRL + Qt::Key_3;
        break;
    case 3:
        dkey = Qt::CTRL + Qt::Key_4;
        break;
    case 4:
        dkey = Qt::CTRL + Qt::Key_5;
        break;
    case 5:
        dkey = Qt::CTRL + Qt::Key_6;
        break;
    case 6:
        dkey = Qt::CTRL + Qt::Key_7;
        break;
    case 7:
        dkey = Qt::CTRL + Qt::Key_8;
        break;
    case 8:
        dkey = Qt::CTRL + Qt::Key_9;
        break;
    case 9:
        dkey = Qt::CTRL + Qt::Key_0;
        break;
    }

    // Do not set up shortcuts if the demod cannot be pre-selected
    if (dkey < 0) {
        return;
    }

    /* UI Controls */
    QShortcut *ui_properties_shortcut = new QShortcut(QKeySequence(dkey, Qt::Key_P), this);
    QShortcut *ui_focus_offset = new QShortcut(QKeySequence(dkey, Qt::SHIFT + Qt::Key_O), this);
    QShortcut *ui_focus_freq = new QShortcut(QKeySequence(dkey, Qt::Key_F), this);
    shortcutConnections.push_back(QObject::connect(ui_properties_shortcut, &QShortcut::activated, ui->tbProperties, &QToolButton::click));
    shortcutConnections.push_back(QObject::connect(ui_focus_offset, &QShortcut::activated, ui->filterOffset, &CFreqCtrl::setFrequencyFocus));
    shortcutConnections.push_back(QObject::connect(ui_focus_freq, &QShortcut::activated, ui->filterFreq, &CFreqCtrl::setFrequencyFocus));

    /* mode setting shortcuts */
    QShortcut *mode_off_shortcut = new QShortcut(QKeySequence(dkey, Qt::Key_O), this);
    QShortcut *mode_raw_shortcut = new QShortcut(QKeySequence(dkey, Qt::Key_I), this);
    QShortcut *mode_am_shortcut = new QShortcut(QKeySequence(dkey, Qt::Key_A), this);
    QShortcut *mode_nfm_shortcut = new QShortcut(QKeySequence(dkey, Qt::Key_N), this);
    QShortcut *mode_wfm_mono_shortcut = new QShortcut(QKeySequence(dkey, Qt::Key_W), this);
    QShortcut *mode_wfm_stereo_shortcut = new QShortcut(QKeySequence(dkey, Qt::SHIFT + Qt::Key_W), this);
    QShortcut *mode_lsb_shortcut = new QShortcut(QKeySequence(dkey, Qt::Key_S), this);
    QShortcut *mode_usb_shortcut = new QShortcut(QKeySequence(dkey, Qt::SHIFT + Qt::Key_S), this);
    QShortcut *mode_cwl_shortcut = new QShortcut(QKeySequence(dkey, Qt::Key_C), this);
    QShortcut *mode_cwu_shortcut = new QShortcut(QKeySequence(dkey, Qt::SHIFT + Qt::Key_C), this);
    QShortcut *mode_wfm_oirt_shortcut = new QShortcut(QKeySequence(dkey, Qt::Key_O), this);
    QShortcut *mode_am_sync_shortcut = new QShortcut(QKeySequence(dkey, Qt::SHIFT + Qt::Key_A), this);

    shortcutConnections.push_back(QObject::connect(mode_off_shortcut, &QShortcut::activated, this, &DockRxOpt::modeOffShortcut));
    shortcutConnections.push_back(QObject::connect(mode_raw_shortcut, &QShortcut::activated, this, &DockRxOpt::modeRawShortcut));
    shortcutConnections.push_back(QObject::connect(mode_am_shortcut, &QShortcut::activated, this, &DockRxOpt::modeAMShortcut));
    shortcutConnections.push_back(QObject::connect(mode_nfm_shortcut, &QShortcut::activated, this, &DockRxOpt::modeNFMShortcut));
    shortcutConnections.push_back(QObject::connect(mode_wfm_mono_shortcut, &QShortcut::activated, this, &DockRxOpt::modeWFMmonoShortcut));
    shortcutConnections.push_back(QObject::connect(mode_wfm_stereo_shortcut, &QShortcut::activated, this, &DockRxOpt::modeWFMstereoShortcut));
    shortcutConnections.push_back(QObject::connect(mode_lsb_shortcut, &QShortcut::activated, this, &DockRxOpt::modeLSBShortcut));
    shortcutConnections.push_back(QObject::connect(mode_usb_shortcut, &QShortcut::activated, this, &DockRxOpt::modeUSBShortcut));
    shortcutConnections.push_back(QObject::connect(mode_cwl_shortcut, &QShortcut::activated, this, &DockRxOpt::modeCWLShortcut));
    shortcutConnections.push_back(QObject::connect(mode_cwu_shortcut, &QShortcut::activated, this, &DockRxOpt::modeCWUShortcut));
    shortcutConnections.push_back(QObject::connect(mode_wfm_oirt_shortcut, &QShortcut::activated, this, &DockRxOpt::modeWFMoirtShortcut));
    shortcutConnections.push_back(QObject::connect(mode_am_sync_shortcut, &QShortcut::activated, this, &DockRxOpt::modeAMsyncShortcut));

    /* squelch shortcuts */
    QShortcut *squelch_reset_shortcut = new QShortcut(QKeySequence(dkey, Qt::Key_QuoteLeft), this);
    QShortcut *squelch_auto_shortcut = new QShortcut(QKeySequence(dkey, Qt::Key_AsciiTilde), this);

    shortcutConnections.push_back(QObject::connect(squelch_reset_shortcut, &QShortcut::activated, this, &DockRxOpt::on_resetSquelchButton_clicked));
    shortcutConnections.push_back(QObject::connect(squelch_auto_shortcut, &QShortcut::activated, this, &DockRxOpt::on_autoSquelchButton_clicked));

    /* filter width shortcuts */
    QShortcut *filter_narrow_shortcut = new QShortcut(QKeySequence(dkey, Qt::Key_Less), this);
    QShortcut *filter_normal_shortcut = new QShortcut(QKeySequence(dkey, Qt::Key_Period), this);
    QShortcut *filter_wide_shortcut = new QShortcut(QKeySequence(dkey, Qt::Key_Greater), this);

    shortcutConnections.push_back(QObject::connect(filter_narrow_shortcut, &QShortcut::activated, this, &DockRxOpt::filterNarrowShortcut));
    shortcutConnections.push_back(QObject::connect(filter_normal_shortcut, &QShortcut::activated, this, &DockRxOpt::filterNormalShortcut));
    shortcutConnections.push_back(QObject::connect(filter_wide_shortcut, &QShortcut::activated, this, &DockRxOpt::filterWideShortcut));

    /* Bookmark */
    QShortcut *bookmark_shortcut = new QShortcut(QKeySequence(dkey, Qt::CTRL + Qt::SHIFT + Qt::Key_B), this);
    shortcutConnections.push_back(QObject::connect(bookmark_shortcut, &QShortcut::activated, this, &DockRxOpt::bookmark));

    // Store all the shortcut pointers so we can remove them
    shortcuts.push_back(ui_properties_shortcut);
    shortcuts.push_back(ui_focus_offset);
    shortcuts.push_back(ui_focus_freq);
    shortcuts.push_back(mode_off_shortcut);
    shortcuts.push_back(mode_raw_shortcut);
    shortcuts.push_back(mode_am_shortcut);
    shortcuts.push_back(mode_nfm_shortcut);
    shortcuts.push_back(mode_wfm_mono_shortcut);
    shortcuts.push_back(mode_wfm_stereo_shortcut);
    shortcuts.push_back(mode_lsb_shortcut);
    shortcuts.push_back(mode_usb_shortcut);
    shortcuts.push_back(mode_cwl_shortcut);
    shortcuts.push_back(mode_cwu_shortcut);
    shortcuts.push_back(mode_wfm_oirt_shortcut);
    shortcuts.push_back(mode_am_sync_shortcut);
    shortcuts.push_back(squelch_reset_shortcut);
    shortcuts.push_back(squelch_auto_shortcut);
    shortcuts.push_back(filter_narrow_shortcut);
    shortcuts.push_back(filter_normal_shortcut);
    shortcuts.push_back(filter_wide_shortcut);
    shortcuts.push_back(bookmark_shortcut);
}

void DockRxOpt::removeShortcuts()
{
    for (int i = 0; i < shortcutConnections.size(); ++i)
    {
        disconnect(shortcutConnections[i]);
    }
    shortcutConnections.clear();
    for (int i = 0; i < shortcuts.size(); ++i)
    {
        delete shortcuts[i];
    }
    shortcuts.clear();
}

DockRxOpt::~DockRxOpt()
{
    removeShortcuts();

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
//    qInfo() << "DockRxOpt::setFilterOffset" << freq_hz;

    auto afreq_hz = abs(freq_hz);
    FctlUnit unit = FCTL_UNIT_HZ;
    if (afreq_hz > 1e9) unit = FCTL_UNIT_GHZ;
    else if (afreq_hz > 1e6) unit = FCTL_UNIT_MHZ;
    else if (afreq_hz > 1e3) unit = FCTL_UNIT_KHZ;
    ui->filterOffset->setUnit(unit);
    ui->filterOffset->setFrequency(freq_hz);
}

/**
 * @brief Set filter offset range.
 * @param range_hz The new range in Hz.
 */
void DockRxOpt::setFilterOffsetRange(qint64 range_hz)
{
    if (range_hz <= 0)
        return;

    ui->filterOffset->setup(0, -range_hz, range_hz, 1, FCTL_UNIT_KHZ);
}

/**
 * @brief Set new RF frequency
 * @param freq_hz The frequency in Hz
 *
 * RF frequency is the frequency to which the receiver device is tuned to.
 * The actual RX frequency is the sum of the RF frequency and the filter offset.
 */
void DockRxOpt::setHwFreq(qint64 freq_hz, bool maintain_rx_freq)
{
    // qInfo() << "DockRxOpt::setHwFreq" << freq_hz;
    hw_freq_hz = freq_hz;
    updateHwFreq();

    if (maintain_rx_freq) {
        // Change the offset to keep the same output Rx freq
        auto rx_freq = ui->filterFreq->getFrequency();
        auto new_offset = rx_freq - hw_freq_hz;
        ui->filterOffset->setFrequency(new_offset);
    } else {
        setRxFreq(hw_freq_hz + ui->filterOffset->getFrequency());
    }
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
    int mode_index = ui->modeSelector->currentIndex();

    if (lo == filter_preset_table[mode_index][FILTER_PRESET_WIDE][0] &&
        hi == filter_preset_table[mode_index][FILTER_PRESET_WIDE][1])
        return FILTER_PRESET_WIDE;
    else if (lo == filter_preset_table[mode_index][FILTER_PRESET_NORMAL][0] &&
             hi == filter_preset_table[mode_index][FILTER_PRESET_NORMAL][1])
        return FILTER_PRESET_NORMAL;
    else if (lo == filter_preset_table[mode_index][FILTER_PRESET_NARROW][0] &&
             hi == filter_preset_table[mode_index][FILTER_PRESET_NARROW][1])
        return FILTER_PRESET_NARROW;

    return FILTER_PRESET_USER;
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
                                     .arg(width_f));
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
void DockRxOpt::setCurrentDemod(int demod)
{
    if ((demod >= MODE_OFF) && (demod < MODE_LAST))
    {
        ui->modeSelector->setCurrentIndex(demod);
        updateDemodOptPage(demod);
    }
}

/**
 * @brief Get current demodulator selection.
 * @return The current demodulator corresponding to receiver::demod.
 */
int  DockRxOpt::currentDemod() const
{
    return ui->modeSelector->currentIndex();
}

QString DockRxOpt::currentDemodAsString()
{
    return GetStringForModulationIndex(currentDemod());
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
    ui->sMeter->setSqlLevel(level);
}

void DockRxOpt::setSignalLevel(float level)
{
    ui->sMeter->setLevel(level);
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

/** Get filter lo/hi for a given mode and preset */
void DockRxOpt::getFilterPreset(int mode, int preset, int * lo, int * hi) const
{
    if (mode < 0 || mode >= MODE_LAST)
    {
        qInfo() << __func__ << ": Invalid mode:" << mode;
        mode = MODE_AM;
    }
    else if (preset < 0 || preset > 2)
    {
        qInfo() << __func__ << ": Invalid preset:" << preset;
        preset = FILTER_PRESET_NORMAL;
    }
    *lo = filter_preset_table[mode][preset][0];
    *hi = filter_preset_table[mode][preset][1];
}

int DockRxOpt::getCwOffset() const
{
    return demodOpt->getCwOffset();
}

/** Read receiver configuration from settings data. */
void DockRxOpt::readSettings(std::shared_ptr<QSettings> settings, size_t idx)
{
    bool    conv_ok;
    int     int_val;
    double  dbl_val;
    bool    bool_val;

    auto configVersion = settings->value("configversion").toInt(&conv_ok);

    settings->beginGroup("receiver");

    // Migrate v3 settings for 1st demod only
    if (configVersion < 4 && idx == 0)
    {
        QStringList v3Keys({
            "cwoffset",
            "fm_maxdev",
            "fm_deemph",
            "offset",
            "sql_level",
            "agc_threshold",
            "agc_delay",
            "agc_slope",
            "agc_gain",
            "agc_usehang",
            "agc_off",
            "demod",
        });
        for (auto &key : v3Keys)
        {
            if (settings->contains(key)) {
                settings->setValue("0/" + key, settings->value(key));
                settings->remove(key);
            }
        }
    }

    settings->beginGroup(QString("%0").arg(idx));

    settings->beginGroup("ui");
    bool_val = settings->value("hide_properties", false).toBool();
    if (bool_val)
        ui->tbProperties->setChecked(false);
    settings->endGroup(); // ui

    int_val = settings->value("cwoffset", 700).toInt(&conv_ok);
    if (conv_ok)
        demodOpt->setCwOffset(int_val);

    int_val = settings->value("fm_maxdev", 2500).toInt(&conv_ok);
    if (conv_ok)
        demodOpt->setMaxDev(int_val);

    dbl_val = settings->value("fm_deemph", 75).toDouble(&conv_ok);
    if (conv_ok && dbl_val >= 0)
        demodOpt->setEmph(1.0e-6 * dbl_val); // was stored as usec

    qint64 offs = settings->value("offset", 0).toInt(&conv_ok);
    if (conv_ok)
    {
        // qInfo() << "DockRxOpt::readSettings recalls offset" << offs;
        setFilterOffset(offs);
    }

    dbl_val = settings->value("sql_level", 1.0).toDouble(&conv_ok);
    if (conv_ok && dbl_val < 1.0)
        ui->sqlSpinBox->setValue(dbl_val);

    // AGC settings
    int_val = settings->value("agc_threshold", -100).toInt(&conv_ok);
    if (conv_ok)
        agcOpt->setThreshold(int_val);

    int_val = settings->value("agc_decay", 500).toInt(&conv_ok);
    if (conv_ok)
    {
        agcOpt->setDecay(int_val);
        if (int_val == 100)
            ui->agcPresetCombo->setCurrentIndex(0);
        else if (int_val == 500)
            ui->agcPresetCombo->setCurrentIndex(1);
        else if (int_val == 2000)
            ui->agcPresetCombo->setCurrentIndex(2);
        else
            ui->agcPresetCombo->setCurrentIndex(3);
    }

    int_val = settings->value("agc_slope", 0).toInt(&conv_ok);
    if (conv_ok)
        agcOpt->setSlope(int_val);

    int_val = settings->value("agc_gain", 0).toInt(&conv_ok);
    if (conv_ok)
        agcOpt->setGain(int_val);

    agcOpt->setHang(settings->value("agc_usehang", false).toBool());

    if (settings->value("agc_off", false).toBool())
        ui->agcPresetCombo->setCurrentIndex(4);

    int_val = MODE_AM;
    if (settings->contains("demod")) {
        if (configVersion >= 3) {
            int_val = GetEnumForModulationString(settings->value("demod").toString());
        } else {
            int_val = old2new[settings->value("demod").toInt(&conv_ok)];
        }
    }

    setCurrentDemod(int_val);
    emit demodSelected(int_val);

    settings->endGroup(); // idx
    settings->endGroup(); // receiver
}

/** Save receiver configuration to settings. */
void DockRxOpt::saveSettings(std::shared_ptr<QSettings> settings, size_t idx)
{
    int     int_val;

    settings->beginGroup("receiver");
    settings->beginGroup(QString("%0").arg(idx));

    settings->beginGroup("ui");
    if (ui->tbProperties->isChecked())
        settings->remove("hide_properties");
    else
        settings->setValue("hide_properties", true);
    settings->endGroup(); // ui

    settings->setValue("demod", currentDemodAsString());

    int cwofs = demodOpt->getCwOffset();
    if (cwofs == 700)
        settings->remove("cwoffset");
    else
        settings->setValue("cwoffset", cwofs);

    // currently we do not need the decimal
    int_val = (int)demodOpt->getMaxDev();
    if (int_val == 2500)
        settings->remove("fm_maxdev");
    else
        settings->setValue("fm_maxdev", int_val);

    // save as usec
    int_val = (int)(1.0e6 * demodOpt->getEmph());
    if (int_val == 75)
        settings->remove("fm_deemph");
    else
        settings->setValue("fm_deemph", int_val);

    qint64 offs = ui->filterOffset->getFrequency();
    settings->setValue("offset", offs);

    qDebug() << __func__ << "*** FIXME_ SQL on/off";
    //int sql_lvl = double(ui->sqlSlider->value());  // note: dBFS*10 as int
    double sql_lvl = ui->sqlSpinBox->value();
    if (sql_lvl > -150.0)
        settings->setValue("sql_level", sql_lvl);
    else
        settings->remove("sql_level");

    // AGC settings
    int_val = agcOpt->threshold();
    if (int_val != -100)
        settings->setValue("agc_threshold", int_val);
    else
        settings->remove("agc_threshold");

    int_val = agcOpt->decay();
    if (int_val != 500)
        settings->setValue("agc_decay", int_val);
    else
        settings->remove("agc_decay");

    int_val = agcOpt->slope();
    if (int_val != 0)
        settings->setValue("agc_slope", int_val);
    else
        settings->remove("agc_slope");

    int_val = agcOpt->gain();
    if (int_val != 0)
        settings->setValue("agc_gain", int_val);
    else
        settings->remove("agc_gain");

    if (agcOpt->hang())
        settings->setValue("agc_usehang", true);
    else
        settings->remove("agc_usehang");

    // AGC Off
    if (ui->agcPresetCombo->currentIndex() == 4)
        settings->setValue("agc_off", true);
    else
        settings->remove("agc_off");

    settings->endGroup(); // idx
    settings->endGroup(); // receiver
}

void DockRxOpt::setRxFreq(qint64 freq_hz)
{
    // qInfo() << "DockRxOpt::setRxFreq" << freq_hz;
    ui->filterFreq->blockSignals(true);
    FctlUnit unit = FCTL_UNIT_HZ;
    if (freq_hz > 1e9) unit = FCTL_UNIT_GHZ;
    else if (freq_hz > 1e6) unit = FCTL_UNIT_MHZ;
    else if (freq_hz > 1e3) unit = FCTL_UNIT_KHZ;
    ui->filterFreq->setUnit(unit);
    ui->filterFreq->setFrequency(freq_hz);
    ui->filterFreq->blockSignals(false);
    emit rxFreqChanged(freq_hz);
}

void DockRxOpt::setRxFreqRange(qint64 min_hz, qint64 max_hz)
{
    ui->filterFreq->blockSignals(true);
    ui->filterFreq->setup(0, min_hz, max_hz, 1, FCTL_UNIT_HZ);
    ui->filterFreq->blockSignals(false);
}

void DockRxOpt::setResetLowerDigits(bool enabled)
{
    ui->filterOffset->setResetLowerDigits(enabled);
    ui->filterFreq->setResetLowerDigits(enabled);
}

void DockRxOpt::setInvertScrolling(bool enabled)
{
    ui->filterOffset->setInvertScrolling(enabled);
    ui->filterFreq->setInvertScrolling(enabled);
}

/**
 * @brief Channel filter offset has changed
 * @param freq The new filter offset in Hz
 *
 * This slot is activated when a new filter offset has been selected either
 * using the mouse or using the keyboard.
 */
void DockRxOpt::on_filterOffset_newFrequency(qint64 offset)
{
    // qInfo() << "DockRxOpt::on_filterOffset_newFrequency " << freq;
    setRxFreq(hw_freq_hz + offset);
    emit filterOffsetChanged(offset);
}

void DockRxOpt::on_filterFreq_newFrequency(qint64 freq)
{
    ui->filterOffset->setFrequency(freq - hw_freq_hz);
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
    emit demodSelected(ui->modeSelector->currentIndex());
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
    updateDemodOptPage(index);
    emit demodSelected(index);
}

void DockRxOpt::updateDemodOptPage(int demod)
{
    // update demodulator option widget
    if (demod == MODE_NFM)
        demodOpt->setCurrentPage(CDemodOptions::PAGE_FM_OPT);
    else if (demod == MODE_AM)
        demodOpt->setCurrentPage(CDemodOptions::PAGE_AM_OPT);
    else if (demod == MODE_CWL || demod == MODE_CWU)
        demodOpt->setCurrentPage(CDemodOptions::PAGE_CW_OPT);
    else if (demod == MODE_AM_SYNC)
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
    double newval = sqlAutoClicked(); // FIXME: We rely on signal only being connected to one slot
    ui->sqlSpinBox->setValue(newval);
}

void DockRxOpt::on_resetSquelchButton_clicked()
{
    ui->sqlSpinBox->setValue(-150.0);
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
        qInfo() << "Invalid AGC preset:" << index;
    }
}

void DockRxOpt::agcOpt_hangToggled(bool checked)
{
    emit agcHangToggled(checked);
}

/**
 * @brief AGC threshold ("knee") changed.
 * @param value The new AGC threshold in dB.
 */
void DockRxOpt::agcOpt_thresholdChanged(int value)
{
    emit agcThresholdChanged(value);
}

/**
 * @brief AGC slope factor changed.
 * @param value The new slope factor in dB.
 */
void DockRxOpt::agcOpt_slopeChanged(int value)
{
    emit agcSlopeChanged(value);
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
 * @brief AGC manual gain changed.
 * @param gain The new gain in dB.
 */
void DockRxOpt::agcOpt_gainChanged(int gain)
{
    emit agcGainChanged(gain);
}

/**
 * @brief Squelch level change.
 * @param value The new squelch level in dB.
 */
void DockRxOpt::on_sqlSpinBox_valueChanged(double value)
{
    ui->sMeter->setSqlLevel(value);
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

int DockRxOpt::GetEnumForModulationString(QString param)
{
    int iModulation = -1;
    for(int i=0; i<DockRxOpt::ModulationStrings.size(); ++i)
    {
        QString& strModulation = DockRxOpt::ModulationStrings[i];
        if(param.compare(strModulation, Qt::CaseInsensitive)==0)
        {
            iModulation = i;
            break;
        }
    }
    if(iModulation == -1)
    {
        std::cout << "Modulation '" << param.toStdString() << "' is unknown." << std::endl;
        iModulation = MODE_OFF;
    }
    return iModulation;
}

bool DockRxOpt::IsModulationValid(QString strModulation)
{
    return DockRxOpt::ModulationStrings.contains(strModulation, Qt::CaseInsensitive);
}

QString DockRxOpt::GetStringForModulationIndex(int iModulationIndex)
{
    return ModulationStrings[iModulationIndex];
}

void DockRxOpt::modeOffShortcut() {
    on_modeSelector_activated(MODE_OFF);
}

void DockRxOpt::modeRawShortcut() {
    on_modeSelector_activated(MODE_RAW);
}

void DockRxOpt::modeAMShortcut() {
    on_modeSelector_activated(MODE_AM);
}

void DockRxOpt::modeNFMShortcut() {
    on_modeSelector_activated(MODE_NFM);
}

void DockRxOpt::modeWFMmonoShortcut() {
    on_modeSelector_activated(MODE_WFM_MONO);
}

void DockRxOpt::modeWFMstereoShortcut() {
    on_modeSelector_activated(MODE_WFM_STEREO);
}

void DockRxOpt::modeLSBShortcut() {
    on_modeSelector_activated(MODE_LSB);
}

void DockRxOpt::modeUSBShortcut() {
    on_modeSelector_activated(MODE_USB);
}

void DockRxOpt::modeCWLShortcut() {
    on_modeSelector_activated(MODE_CWL);
}

void DockRxOpt::modeCWUShortcut() {
    on_modeSelector_activated(MODE_CWU);
}

void DockRxOpt::modeWFMoirtShortcut() {
    on_modeSelector_activated(MODE_WFM_STEREO_OIRT);
}

void DockRxOpt::modeAMsyncShortcut() {
    on_modeSelector_activated(MODE_AM_SYNC);
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
