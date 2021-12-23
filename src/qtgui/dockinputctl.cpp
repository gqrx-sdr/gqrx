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

#include "dockinputctl.h"
#include "ui_dockinputctl.h"

DockInputCtl::DockInputCtl(QWidget * parent) :
    QFrame(parent),
    ui(new Ui::DockInputCtl)
{
    ui->setupUi(this);

    // Grid layout with gain controls (device dependent)
    gainLayout = new QGridLayout();
    gainLayout->setObjectName(QString::fromUtf8("gainLayout"));
    ui->verticalLayout->insertLayout(2, gainLayout);
}

DockInputCtl::~DockInputCtl()
{
    delete ui;
    delete gainLayout;
}

void DockInputCtl::readSettings(std::shared_ptr<QSettings> settings)
{
    qint64  lnb_lo;
    bool    conv_ok;
    bool    bool_val;

    qint64 ppm_corr = settings->value("input/corr_freq", 0).toLongLong(&conv_ok);
    setFreqCorr(((double)ppm_corr)/1.0e6);
    emit freqCorrChanged(ui->freqCorrSpinBox->value());

    setIqSwap(settings->value("input/swap_iq", false).toBool());
    emit iqSwapChanged(ui->iqSwapButton->isChecked());

    setDcCancel(settings->value("input/dc_cancel", false).toBool());
    emit dcCancelChanged(ui->dcCancelButton->isChecked());

    setIqBalance(settings->value("input/iq_balance", false).toBool());
    emit iqBalanceChanged(ui->iqBalanceButton->isChecked());

    bool_val = settings->value("input/ignore_limits", false).toBool();
    setIgnoreLimits(bool_val);
    emit ignoreLimitsChanged(bool_val);

    lnb_lo = settings->value("input/lnb_lo", 0).toLongLong(&conv_ok);
    if (conv_ok)
    {
        setLnbLo(((double)lnb_lo)/1.0e6);
        emit lnbLoChanged(ui->lnbSpinBox->value());
    }

    // Ignore antenna selection if there is only one option
    if (ui->antSelector->count() > 1)
    {
        QString ant = settings->value("input/antenna", "").toString();
        setAntenna(ant);
        emit antennaSelected(ant);
    }

    // gains are stored as a QMap<QString, QVariant(int)>
    // note that we store the integer values, i.e. dB*10
    if (settings->contains("input/gains"))
    {
        QMap <QString, QVariant>    allgains;
        QString     gain_name;
        double      gain_value;

        allgains = settings->value("input/gains").toMap();
        QMapIterator <QString, QVariant> gain_iter(allgains);

        while (gain_iter.hasNext())
        {
            gain_iter.next();

            gain_name = gain_iter.key();
            gain_value = 0.1 * (double)(gain_iter.value().toInt());
            if (setGain(gain_name, gain_value))
                emit gainChanged(gain_name, gain_value);
        }
    }

    bool_val = settings->value("input/hwagc", false).toBool();
    setAgc(bool_val);
    emit autoGainChanged(bool_val);

    // misc GUI settings
    bool_val = settings->value("gui/fctl_reset_digits", true).toBool();
    emit freqCtrlResetChanged(bool_val);
    ui->freqCtrlResetButton->setChecked(bool_val);

    bool_val = settings->value("gui/invert_scrolling", false).toBool();
    emit invertScrollingChanged(bool_val);
    ui->invertScrollingButton->setChecked(bool_val);

    bool_val = settings->value("gui/offset_follow_hw", false).toBool();
    emit offsetFollowsHwChanged(bool_val);
    ui->offsetFollowButton->setChecked(bool_val);
}

void DockInputCtl::saveSettings(std::shared_ptr<QSettings> settings)
{
    qint64 lnb_lo = (qint64)(ui->lnbSpinBox->value()*1.e6);
    if (lnb_lo)
        settings->setValue("input/lnb_lo", lnb_lo);
    else
        settings->remove("input/lnb_lo");

    // gains are stored as a QMap<QString, QVariant(int)>
    // note that we store the integer values, i.e. dB*10
    QMap <QString, QVariant> gains;
    getGains(&gains);
    if (gains.empty())
        settings->remove("input/gains");
    else
        settings->setValue("input/gains", gains);

    qint64 ppm_corr = (qint64)(ui->freqCorrSpinBox->value()*1.e6);
    if (ppm_corr)
        settings->setValue("input/corr_freq", ppm_corr);
    else
        settings->remove("input/corr_freq");

    if (iqSwap())
        settings->setValue("input/swap_iq", true);
    else
        settings->remove("input/swap_iq");

    if (dcCancel())
        settings->setValue("input/dc_cancel", true);
    else
        settings->remove("input/dc_cancel");

    if (iqBalance())
        settings->setValue("input/iq_balance", true);
    else
        settings->remove("input/iq_balance");

    if (ignoreLimits())
        settings->setValue("input/ignore_limits", true);
    else
        settings->remove("input/ignore_limits");

    if (agc())
        settings->setValue("input/hwagc", true);
    else
        settings->remove("input/hwagc");

    // save antenna selection if there is more than one option
    if (ui->antSelector->count() > 1)
        settings->setValue("input/antenna", ui->antSelector->currentText());
    else
        settings->remove("input/antenna");

    // Remember state of freqReset button. Default is checked.
    if (!ui->freqCtrlResetButton->isChecked())
        settings->setValue("gui/fctl_reset_digits", false);
    else
        settings->remove("gui/fctl_reset_digits");

    // Remember state of invert scrolling button. Default is unchecked.
    if (ui->invertScrollingButton->isChecked())
        settings->setValue("gui/invert_scrolling", true);
    else
        settings->remove("gui/invert_scrolling");

    // Remember state of offset follow button. Default is unchecked.
    if (ui->offsetFollowButton->isChecked())
        settings->setValue("gui/offset_follow_hw", true);
    else
        settings->remove("gui/offset_follow_hw");
}

void DockInputCtl::readLnbLoFromSettings(std::shared_ptr<QSettings> settings)
{
    qint64  lnb_lo;
    bool    conv_ok;

    lnb_lo = settings->value("input/lnb_lo", 0).toLongLong(&conv_ok);
    if (conv_ok)
    {
        setLnbLo(((double)lnb_lo) / 1.0e6);
        emit lnbLoChanged(ui->lnbSpinBox->value());
    }
}

void DockInputCtl::setLnbLo(double freq_mhz)
{
    ui->lnbSpinBox->setValue(freq_mhz);
}

double DockInputCtl::lnbLo()
{
    return ui->lnbSpinBox->value();
}

/**
 * @brief Set new value of a specific gain.
 * @param name The name of the gain to change.
 * @param value The new value.
 */
bool DockInputCtl::setGain(QString name, double value)
{
    int gain = -1;
    bool success = false;

    for (int idx = 0; idx < gain_labels.length(); idx++)
    {
        if (gain_labels.at(idx)->text().contains(name))
        {
            gain = (int)(10 * value);
            gain_sliders.at(idx)->setValue(gain);
            success = true;
            break;
        }
    }

    return success;
}

/**
 * @brief Get current gain.
 * @returns The relative gain between 0.0 and 1.0 or -1 if HW AGC is enabled.
 */
double DockInputCtl::gain(QString &name)
{
    double gain = 0.0;

    for (int idx = 0; idx < gain_labels.length(); ++idx)
    {
        if (gain_labels.at(idx)->text() == name)
        {
            gain = 0.1 * (double)gain_sliders.at(idx)->value();
            break;
        }
    }

    return gain;
}

/**
 * Set status of hardware AGC button.
 * @param enabled Whether hardware AGC is enabled or not.
 */
void DockInputCtl::setAgc(bool enabled)
{
    ui->agcButton->setChecked(enabled);
}

/**
 * @brief Get status of hardware AGC button.
 * @return Whether hardware AGC is enabled or not.
 */
bool DockInputCtl::agc()
{
    return ui->agcButton->isChecked();
}


/**
 * Set new frequency correction.
 * @param corr The new frequency correction in PPM.
 */
void DockInputCtl::setFreqCorr(double corr)
{
    ui->freqCorrSpinBox->setValue(corr);
}


/** Get current frequency correction. */
double DockInputCtl::freqCorr()
{
    return ui->freqCorrSpinBox->value();
}

/** Enasble/disable I/Q swapping. */
void DockInputCtl::setIqSwap(bool reversed)
{
    ui->iqSwapButton->setChecked(reversed);
}

/** Get current I/Q swapping. */
bool DockInputCtl::iqSwap(void)
{
    return ui->iqSwapButton->isChecked();
}

/** Enable automatic DC removal. */
void DockInputCtl::setDcCancel(bool enabled)
{
    ui->dcCancelButton->setChecked(enabled);
}

/** Get current DC remove status. */
bool DockInputCtl::dcCancel(void)
{
    return ui->dcCancelButton->isChecked();
}

/** Enable automatic IQ balance. */
void DockInputCtl::setIqBalance(bool enabled)
{
    ui->iqBalanceButton->setChecked(enabled);
}

/** Get current IQ balance status. */
bool DockInputCtl::iqBalance(void)
{
    return ui->iqBalanceButton->isChecked();
}

/** Enasble/disable ignoring hardware limits. */
void DockInputCtl::setIgnoreLimits(bool reversed)
{
    ui->ignoreButton->setChecked(reversed);
}

/** Get current status of whether limits should be ignored or not. */
bool DockInputCtl::ignoreLimits(void)
{
    return ui->ignoreButton->isChecked();
}

/** Populate antenna selector combo box with strings. */
void DockInputCtl::setAntennas(std::vector<std::string> &antennas)
{
    ui->antSelector->clear();
    for (std::vector<std::string>::iterator it = antennas.begin(); it != antennas.end(); ++it)
    {
        ui->antSelector->addItem(QString(it->c_str()));
    }
}

/** Select antenna. */
void DockInputCtl::setAntenna(const QString &antenna)
{
    int index = ui->antSelector->findText(antenna, Qt::MatchExactly);
    if (index != -1)
        ui->antSelector->setCurrentIndex(index);
}

/** Enable/disable resetting lower digits on freqCtrl widgets */
void DockInputCtl::setFreqCtrlReset(bool enabled)
{
    ui->freqCtrlResetButton->setChecked(enabled);
}

/** Enable/disable invert scroll wheel direction */
void DockInputCtl::setInvertScrolling(bool enabled)
{
    ui->invertScrollingButton->setChecked(enabled);
}

void DockInputCtl::setOffsetFollowsHw(bool enabled)
{
    ui->offsetFollowButton->setChecked(enabled);
}

bool DockInputCtl::getOffsetFollowsHw() const
{
    return ui->offsetFollowButton->isChecked();
}

/**
 * Set gain stages.
 * @param gain_list A list containing the gain stages for this device.
 */
void DockInputCtl::setGainStages(gain_list_t &gain_list)
{
    QLabel  *label;
    QSlider *slider;
    QLabel  *value;
    int start, stop, step, gain;

    // ensure that gain lists are empty
    clearWidgets();

    for (unsigned int i = 0; i < gain_list.size(); i++)
    {
        start = (int)(10.0 * gain_list[i].start);
        stop  = (int)(10.0 * gain_list[i].stop);
        step  = (int)(10.0 * gain_list[i].step);
        gain  = (int)(10.0 * gain_list[i].value);

        label = new QLabel(QString("%1 ").arg(gain_list[i].name.c_str()), this);
        label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);

        value = new QLabel(QString(" %1 dB").arg(gain_list[i].value, 0, 'f', 1), this);
        value->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);

        slider = new QSlider(Qt::Horizontal, this);
        slider->setProperty("idx", i);
        slider->setProperty("name", QString(gain_list[i].name.c_str()));
        slider->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
        slider->setRange(start, stop);
        slider->setSingleStep(step);
        slider->setValue(gain);
        if (abs(stop - start) > 10 * step)
            slider->setPageStep(10 * step);

        gainLayout->addWidget(label, i, 0, Qt::AlignLeft);
        gainLayout->addWidget(slider, i, 1);        // setting alignment would force minimum size
        gainLayout->addWidget(value, i, 2, Qt::AlignLeft);

        gain_labels.push_back(label);
        gain_sliders.push_back(slider);
        value_labels.push_back(value);

        connect(slider, SIGNAL(valueChanged(int)), this, SLOT(sliderValueChanged(int)));
    }

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


/**
 * Load all gains from the settings.
 *
 * Can be used for restoring the manual gains after auto-gain has been
 * disabled.
 */
void DockInputCtl::restoreManualGains(void)
{
    QString gain_stage;
    double  gain_value;
    int     i;

    for (i = 0; i < gain_sliders.length(); i++)
    {
        gain_stage = gain_sliders.at(i)->property("name").toString();
        gain_value = 0.1 * (double)gain_sliders.at(i)->value();
        emit gainChanged(gain_stage, gain_value);
    }
}

/** LNB LO value has changed. */
void DockInputCtl::on_lnbSpinBox_valueChanged(double value)
{
    emit lnbLoChanged(value);
}

/** Automatic gain control button has been toggled. */
void DockInputCtl::on_agcButton_toggled(bool checked)
{
    for (int i = 0; i < gain_sliders.length(); ++i)
    {
        gain_sliders.at(i)->setEnabled(!checked);
    }

    emit autoGainChanged(checked);
}

/**
 * Frequency correction changed.
 * @param value The new frequency correction in ppm.
 */
void DockInputCtl::on_freqCorrSpinBox_valueChanged(double value)
{
    emit freqCorrChanged(value);
}

/**
 * I/Q swapping checkbox changed.
 * @param checked True if I/Q swapping is enabled, false otherwise
 */
void DockInputCtl::on_iqSwapButton_toggled(bool checked)
{
    emit iqSwapChanged(checked);
}

/**
 * DC removal checkbox changed.
 * @param checked True if DC removal is enabled, false otherwise
 */
void DockInputCtl::on_dcCancelButton_toggled(bool checked)
{
    emit dcCancelChanged(checked);
}

/**
 * IQ balance checkbox changed.
 * @param checked True if automatic IQ balance is enabled, false otherwise
 */
void DockInputCtl::on_iqBalanceButton_toggled(bool checked)
{
    emit iqBalanceChanged(checked);
}

/*! \brief Ignore hardware limits checkbox changed.
 *  \param checked True if hardware limits should be ignored, false otherwise
 *
 * This option exists to allow experimenting with out-of-spec settings.
 */
void DockInputCtl::on_ignoreButton_toggled(bool checked)
{
    emit ignoreLimitsChanged(checked);
}

/** Antenna selection has changed. */
void DockInputCtl::on_antSelector_currentIndexChanged(const QString &antenna)
{
    emit antennaSelected(antenna);
}

/** Reset box has changed */
void DockInputCtl::on_freqCtrlResetButton_toggled(bool checked)
{
    emit freqCtrlResetChanged(checked);
}

/** Invert scrolling box has changed */
void DockInputCtl::on_invertScrollingButton_toggled(bool checked)
{
    emit invertScrollingChanged(checked);
}

void DockInputCtl::on_offsetFollowButton_toggled(bool checked)
{
    emit offsetFollowsHwChanged(checked);
}

/** Remove all widgets from the lists. */
void DockInputCtl::clearWidgets()
{
    QWidget *widget;

    // sliders
    while (!gain_sliders.isEmpty())
    {
        widget = gain_sliders.takeFirst();
        gainLayout->removeWidget(widget);
        delete widget;
    }

    // labels
    while (!gain_labels.isEmpty())
    {
        widget = gain_labels.takeFirst();
        gainLayout->removeWidget(widget);
        delete widget;
    }

    // value labels
    while (!value_labels.isEmpty())
    {
        widget = value_labels.takeFirst();
        gainLayout->removeWidget(widget);
        delete widget;
    }
}

/**
 * Slot for managing slider value changed signals.
 * @param value The value of the slider.
 *
 * Note. We use the sender() function to find out which slider has emitted the signal.
 */
void DockInputCtl::sliderValueChanged(int value)
{
    QSlider *slider = (QSlider *) sender();
    int idx = slider->property("idx").toInt();

    // convert to discrete value according to step
    if (slider->singleStep()) {
        value = slider->singleStep() * (value / slider->singleStep());
    }

    // convert to double and send signal
    double gain = (double)value / 10.0;
    updateLabel(idx, gain);

    emit gainChanged(slider->property("name").toString(), gain);
}

/**
 * Update value label
 * @param idx The index of the gain
 * @param value The new value
 */
void DockInputCtl::updateLabel(int idx, double value)
{
    QLabel *label = value_labels.at(idx);

    label->setText(QString("%1 dB").arg(value, 0, 'f', 1));
}

/**
 * Get all gains.
 * @param gains Pointer to a map where the gains and their names are stored.
 *
 * This is a private utility function used when storing the settings.
 */
void DockInputCtl::getGains(QMap<QString, QVariant> *gains)
{
    for (int idx = 0; idx < gain_sliders.length(); ++idx)
    {
        gains->insert(gain_sliders.at(idx)->property("name").toString(),
                      QVariant(gain_sliders.at(idx)->value()));
    }
}
