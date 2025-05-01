/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2011-2014 Alexandru Csete OZ9AEC.
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
#include <iomanip>
#include <QDebug>
#include <QFile>
#include <QPushButton>
#include <QSettings>
#include <QString>
#include <QtGlobal>
#include <QVariant>



#include <SoapySDR/Device.hpp>

#ifdef WITH_PULSEAUDIO
#include "pulseaudio/pa_device_list.h"
#elif WITH_PORTAUDIO
#include "portaudio/device_list.h"
#elif defined(Q_OS_DARWIN)
#include "osxaudio/device_list.h"
#endif

#include "qtgui/ioconfig.h"
#include "ui_ioconfig.h"


CIoConfig::CIoConfig(QSettings * settings,
                     std::map<QString, QVariant> &devList,
                     QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CIoConfig),
    m_settings(settings),
    m_devList(&devList)
{
    ui->setupUi(this);

    // update input device list
    updateInDev(settings, *m_devList);

    // input rate
    updateInputSampleRates(settings->value("input/sample_rate", 0).toInt());

    // decimation
    int idx = decim2idx(settings->value("input/decimation", 0).toInt());
    ui->decimCombo->setCurrentIndex(idx);
    decimationChanged(idx);

    // Analog bandwidth
    ui->bwSpinBox->setValue(1.0e-6*settings->value("input/bandwidth", 0.0).toDouble());

    // LNB LO
    ui->loSpinBox->setValue(1.0e-6 * settings->value("input/lnb_lo", 0.0).toDouble());

    // Output device
    updateOutDev();

    m_scanButton = new QPushButton("&Device scan", ui->buttonBox);
    ui->buttonBox->addButton(m_scanButton, QDialogButtonBox::ButtonRole::ActionRole);

    // Signals and slots
    connect(this, SIGNAL(accepted()), this, SLOT(saveConfig()));
    connect(ui->inDevCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(inputDeviceSelected(int)));
    connect(ui->inDevEdit, SIGNAL(textChanged(QString)), this, SLOT(inputDevstrChanged(QString)));
    connect(ui->inSrCombo, SIGNAL(editTextChanged(QString)), this, SLOT(inputRateChanged(QString)));
    connect(ui->decimCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(decimationChanged(int)));
    connect(m_scanButton, SIGNAL(clicked(bool)), this, SLOT(onScanButtonClicked()));
}

CIoConfig::~CIoConfig()
{
    delete m_scanButton;
    delete ui;
}

/**
 * @brief get the list of devices
 */
void CIoConfig::getDeviceList(std::map<QString, QVariant> &devList)
{
    QString         devstr;
    QString         devlabel;

    // automatic discovery of FCD does not work on Mac
    // so we do it ourselves
#if defined(Q_OS_DARWIN)
#ifdef WITH_PORTAUDIO
    portaudio_device_list       devices;
    vector<portaudio_device>    inDevList = devices.get_input_devices();
#else
    osxaudio_device_list        devices;
    vector<osxaudio_device>     inDevList = devices.get_input_devices();
#endif
    string this_dev;
    for (auto &device : inDevList)
    {
        this_dev = device.get_name();
        if (this_dev.find("FUNcube Dongle V1.0") != string::npos)
        {
            devstr = "fcd,type=1,device='FUNcube Dongle V1.0'";
            devList.insert(std::pair<QString, QVariant>(QString("FUNcube Dongle V1.0"), QVariant(devstr)));
        }
        else if (this_dev.find("FUNcube Dongle V1_0") != string::npos)      // since OS X 10.11.4
        {
            devstr = "fcd,type=1,device='FUNcube Dongle V1_0'";
            devList.insert(std::pair<QString, QVariant>(QString("FUNcube Dongle V1_0"), QVariant(devstr)));
        }
        else if (this_dev.find("FUNcube Dongle V2.0") != string::npos)
        {
            devstr = "fcd,type=2,device='FUNcube Dongle V2.0'";
            devList.insert(std::pair<QString, QVariant>(QString("FUNcube Dongle V2.0"), QVariant(devstr)));
        }
        else if (this_dev.find("FUNcube Dongle V2_0") != string::npos)      // since OS X 10.11.4
        {
            devstr = "fcd,type=2,device='FUNcube Dongle V2_0'";
            devList.insert(std::pair<QString, QVariant>(QString("FUNcube Dongle V2_0"), QVariant(devstr)));
        }
    }
#endif

    // Get list of input devices discovered by gr-soapy and store them in
    // the device list together with the device descriptor strings
    std::vector<SoapySDR::Kwargs> devs = SoapySDR::Device::enumerate();

    qDebug() << __FUNCTION__ << ": Available input devices:";
    for (const auto &dev : devs)
    {
        // Get the device label
        if (dev.count("label"))
        {
            devlabel = QString::fromStdString(dev.at("label"));
        }
        else
        {
            // Construct a label from driver and serial if label is not available
            QString driver = dev.count("driver") ? QString::fromStdString(dev.at("driver")) : "Unknown";
            QString serial = dev.count("serial") ? QString::fromStdString(dev.at("serial")) : "";
            devlabel = QString("%1 (%2)").arg(driver, serial);
        }

        // Serialize the device arguments
        std::string device_args = SoapySDR::KwargsToString(dev);
        devstr = QString::fromStdString(device_args);

        // Insert into the device list
        devList.insert(std::pair<QString, QVariant>(devlabel, devstr));
        qDebug() << "  " << devlabel;
    }
}

/** Save configuration. */
void CIoConfig::saveConfig()
{
    int         idx;
    int         int_val;
    bool        conv_ok;

    qDebug() << __FUNCTION__;

    idx = ui->outDevCombo->currentIndex();

#if defined(WITH_PULSEAUDIO) || defined(WITH_PORTAUDIO) || defined(Q_OS_DARWIN)
    if (idx > 0)
    {
          qDebug() << "Output device" << idx << ":" << QString(outDevList[idx-1].get_name().c_str());
          m_settings->setValue("output/device", QString(outDevList[idx-1].get_name().c_str()));
    }
#else
    if (idx > 0 || ui->outDevCombo->currentText() != "Default")
    {
        qDebug() << "Output device:" << ui->outDevCombo->currentText();
        m_settings->setValue("output/device", ui->outDevCombo->currentText());
    }
#endif
    else
    {
        m_settings->remove("output/device");
    }

    // input settings
    m_settings->setValue("input/device", ui->inDevEdit->text());  // "OK" button disabled if empty

    qint64 value = (qint64)(ui->bwSpinBox->value()*1.e6);
    if (value)
        m_settings->setValue("input/bandwidth", value);
    else
        m_settings->remove("input/bandwidth");

    value = (qint64)(ui->loSpinBox->value()*1.e6);
    if (value)
        m_settings->setValue("input/lnb_lo", value);
    else
        m_settings->remove("input/lnb_lo");

    int_val = ui->inSrCombo->currentText().toInt(&conv_ok);
    if (conv_ok)
        m_settings->setValue("input/sample_rate", int_val);
    else
        m_settings->remove("input/sample_rate");

    idx = ui->decimCombo->currentIndex();
    int_val = idx2decim(idx);
    if (int_val < 2)
        m_settings->remove("input/decimation");
    else
        m_settings->setValue("input/decimation", int_val);
}


/**
 * @brief Update list of sample rates based on selected device.
 * @param rate The current sample rate from the configuration.
 */
void CIoConfig::updateInputSampleRates(int rate)
{
    ui->inSrCombo->clear();

    if (ui->inDevEdit->text().isEmpty())
        return;

    if (ui->inDevEdit->text().contains("fcd"))
    {
        if (ui->inDevCombo->currentText().contains("V2"))   // V2.0 or V2_0
        {
            ui->inSrCombo->addItem("192000");
        }
        else
        {
            ui->inSrCombo->addItem("96000");
        }
    }
    else if (ui->inDevEdit->text().contains("rtl") || ui->inDevEdit->text().contains("rtl_tcp"))
    {
        ui->inSrCombo->addItem("240000");
        ui->inSrCombo->addItem("300000");
        ui->inSrCombo->addItem("960000");
        ui->inSrCombo->addItem("1152000");
        ui->inSrCombo->addItem("1200000");
        ui->inSrCombo->addItem("1440000");
        ui->inSrCombo->addItem("1600000");
        ui->inSrCombo->addItem("1800000");
        ui->inSrCombo->addItem("1920000");
        ui->inSrCombo->addItem("2400000");
        ui->inSrCombo->addItem("2880000");
        ui->inSrCombo->addItem("3200000");
        if (rate > 0)
        {
            ui->inSrCombo->addItem(QString("%1").arg(rate));
            ui->inSrCombo->setCurrentIndex(12);
        }
        else
        {
            ui->inSrCombo->setCurrentIndex(7);
        }
    }
    else if (ui->inDevEdit->text().contains("uhd"))
    {
        if (rate > 0)
            ui->inSrCombo->addItem(QString("%1").arg(rate));
        ui->inSrCombo->addItem("250000");
        ui->inSrCombo->addItem("500000");
        ui->inSrCombo->addItem("2000000");
        ui->inSrCombo->addItem("4000000");
        ui->inSrCombo->addItem("8000000");
    }
    else if (ui->inDevEdit->text().contains("hackrf"))
    {
        if (rate > 0)
            ui->inSrCombo->addItem(QString("%1").arg(rate));
        ui->inSrCombo->addItem("8000000");
        ui->inSrCombo->addItem("10000000");
        ui->inSrCombo->addItem("12500000");
        ui->inSrCombo->addItem("16000000");
        ui->inSrCombo->addItem("20000000");
    }
    else if (ui->inDevEdit->text().contains("bladerf"))
    {
        if (rate > 0)
            ui->inSrCombo->addItem(QString("%1").arg(rate));
        ui->inSrCombo->addItem("5000000");
        ui->inSrCombo->addItem("8000000");
        ui->inSrCombo->addItem("10000000");
        ui->inSrCombo->addItem("15000000");
        ui->inSrCombo->addItem("20000000");
        ui->inSrCombo->addItem("25000000");
        ui->inSrCombo->addItem("30000000");
        ui->inSrCombo->addItem("35000000");
        ui->inSrCombo->addItem("40000000");
    }
    else if (ui->inDevEdit->text().contains("sdr-iq"))
    {
        ui->inSrCombo->addItem("8138");
        ui->inSrCombo->addItem("16276");
        ui->inSrCombo->addItem("37793");
        ui->inSrCombo->addItem("55556");
        ui->inSrCombo->addItem("111111");
        ui->inSrCombo->addItem("158730");
        ui->inSrCombo->addItem("196078");
        if (rate > 0)
        {
            ui->inSrCombo->insertItem(0, QString("%1").arg(rate));
            ui->inSrCombo->setCurrentIndex(0);
        }
        else
            ui->inSrCombo->setCurrentIndex(4); // select 111.111 kHz
    }
    else if (ui->inDevEdit->text().contains("sdr-ip"))
    {
        ui->inSrCombo->addItem("31250");
        ui->inSrCombo->addItem("32000");
        ui->inSrCombo->addItem("40000");
        ui->inSrCombo->addItem("50000");
        ui->inSrCombo->addItem("62500");
        ui->inSrCombo->addItem("64000");
        ui->inSrCombo->addItem("80000");
        ui->inSrCombo->addItem("100000");
        ui->inSrCombo->addItem("125000");
        ui->inSrCombo->addItem("160000");
        ui->inSrCombo->addItem("200000");
        ui->inSrCombo->addItem("250000");
        ui->inSrCombo->addItem("320000");
        ui->inSrCombo->addItem("400000");
        ui->inSrCombo->addItem("500000");
        ui->inSrCombo->addItem("800000");
        ui->inSrCombo->addItem("1000000");
        ui->inSrCombo->addItem("1600000");
        ui->inSrCombo->addItem("2000000");
        if (rate > 0)
        {
            ui->inSrCombo->insertItem(0, QString("%1").arg(rate));
            ui->inSrCombo->setCurrentIndex(0);
        }
        else
            ui->inSrCombo->setCurrentIndex(11); // select 250 kHz
    }
    else if (ui->inDevEdit->text().contains("netsdr"))
    {
        ui->inSrCombo->addItem("32000");
        ui->inSrCombo->addItem("40000");
        ui->inSrCombo->addItem("50000");
        ui->inSrCombo->addItem("62500");
        ui->inSrCombo->addItem("78125");
        ui->inSrCombo->addItem("80000");
        ui->inSrCombo->addItem("100000");
        ui->inSrCombo->addItem("125000");
        ui->inSrCombo->addItem("156250");
        ui->inSrCombo->addItem("160000");
        ui->inSrCombo->addItem("200000");
        ui->inSrCombo->addItem("250000");
        ui->inSrCombo->addItem("312500");
        ui->inSrCombo->addItem("400000");
        ui->inSrCombo->addItem("500000");
        ui->inSrCombo->addItem("625000");
        ui->inSrCombo->addItem("800000");
        ui->inSrCombo->addItem("1000000");
        ui->inSrCombo->addItem("1250000");
        ui->inSrCombo->addItem("2000000");
        if (rate > 0)
        {
            ui->inSrCombo->insertItem(0, QString("%1").arg(rate));
            ui->inSrCombo->setCurrentIndex(0);
        }
        else
            ui->inSrCombo->setCurrentIndex(11); // select 250 kHz
    }
    else if (ui->inDevEdit->text().contains("cloudiq"))
    {
        ui->inSrCombo->addItem("48000");
        ui->inSrCombo->addItem("61440");
        ui->inSrCombo->addItem("96000");
        ui->inSrCombo->addItem("122880");
        ui->inSrCombo->addItem("240000");
        ui->inSrCombo->addItem("256000");
        ui->inSrCombo->addItem("370120");
        ui->inSrCombo->addItem("495483");
        ui->inSrCombo->addItem("512000");
        ui->inSrCombo->addItem("614400");
        ui->inSrCombo->addItem("1024000");
        ui->inSrCombo->addItem("1228800");
        ui->inSrCombo->addItem("1807058");
        if (rate > 0)
        {
            ui->inSrCombo->insertItem(0, QString("%1").arg(rate));
            ui->inSrCombo->setCurrentIndex(0);
        }
        else
            ui->inSrCombo->setCurrentIndex(4); // select 240 kHz
    }
    else if (ui->inDevEdit->text().contains("airspyhf"))
    {
        ui->inSrCombo->addItem("192000");
        ui->inSrCombo->addItem("256000");
        ui->inSrCombo->addItem("384000");
        ui->inSrCombo->addItem("456000");
        ui->inSrCombo->addItem("768000");
        ui->inSrCombo->addItem("912000");

        if (rate > 0)
        {
            ui->inSrCombo->insertItem(0, QString("%1").arg(rate));
            ui->inSrCombo->setCurrentIndex(0);
        }
        else
            ui->inSrCombo->setCurrentIndex(4); // select 768 kHz
    }
    // NB: must list airspyhf first
    else if (ui->inDevEdit->text().contains("airspy"))
    {
        if (rate > 0)
            ui->inSrCombo->addItem(QString("%1").arg(rate));

        ui->inSrCombo->addItem("2500000");
        ui->inSrCombo->addItem("3000000");
        ui->inSrCombo->addItem("6000000");
        ui->inSrCombo->addItem("10000000");
    }
    else if (ui->inDevEdit->text().contains("redpitaya"))
    {
        ui->inSrCombo->addItem("20000");
        ui->inSrCombo->addItem("50000");
        ui->inSrCombo->addItem("100000");
        ui->inSrCombo->addItem("250000");
        ui->inSrCombo->addItem("500000");
        ui->inSrCombo->addItem("1250000");
        if (rate > 0)
        {
            ui->inSrCombo->insertItem(0, QString("%1").arg(rate));
            ui->inSrCombo->setCurrentIndex(0);
        }
        else
            ui->inSrCombo->setCurrentIndex(3); // select 250 kHz
    }
    else if (ui->inDevEdit->text().contains("sdrplay"))
    {
        ui->inSrCombo->addItem("62500");
        ui->inSrCombo->addItem("125000");
        ui->inSrCombo->addItem("250000");
        ui->inSrCombo->addItem("500000");
        ui->inSrCombo->addItem("1000000");
        ui->inSrCombo->addItem("2000000");
        ui->inSrCombo->addItem("3000000");
        ui->inSrCombo->addItem("4000000");
        ui->inSrCombo->addItem("5000000");
        ui->inSrCombo->addItem("6000000");
        ui->inSrCombo->addItem("7000000");
        ui->inSrCombo->addItem("8000000");
        ui->inSrCombo->addItem("9000000");
        ui->inSrCombo->addItem("10000000");
        if (rate > 0)
        {
            if (rate < 62500)
                rate = 62500;
            if (rate > 10000000)
                rate = 10000000;
            ui->inSrCombo->insertItem(0, QString("%1").arg(rate));
            ui->inSrCombo->setCurrentIndex(0);
        }
        else
            ui->inSrCombo->setCurrentIndex(5); // select 2 MHz
    }
    else if (ui->inDevEdit->text().contains("lime"))
    {
        ui->inSrCombo->addItem("100000");
        ui->inSrCombo->addItem("500000");
        ui->inSrCombo->addItem("1000000");
        ui->inSrCombo->addItem("2500000");
        ui->inSrCombo->addItem("5000000");
        ui->inSrCombo->addItem("10000000");
        ui->inSrCombo->addItem("20000000");
        ui->inSrCombo->addItem("50000000");
        if (rate > 0)
        {
            ui->inSrCombo->insertItem(0, QString("%1").arg(rate));
            ui->inSrCombo->setCurrentIndex(0);
        }
        else
            ui->inSrCombo->setCurrentIndex(4); // select 5 MHz
    }
    else if (ui->inDevEdit->text().contains("plutosdr"))
    {
        ui->inSrCombo->addItem("600000");
        ui->inSrCombo->addItem("1000000");
        ui->inSrCombo->addItem("1500000");
        ui->inSrCombo->addItem("2000000");
        ui->inSrCombo->addItem("3000000");
        ui->inSrCombo->addItem("6000000");
        ui->inSrCombo->addItem("16000000");
        ui->inSrCombo->addItem("20000000");
        ui->inSrCombo->addItem("56000000");
        if (rate > 0)
        {
            ui->inSrCombo->insertItem(0, QString("%1").arg(rate));
            ui->inSrCombo->setCurrentIndex(0);
        }
        else
            ui->inSrCombo->setCurrentIndex(2); // select 2 MHz
    }
    else if (ui->inDevEdit->text().contains("perseus"))
    {
        ui->inSrCombo->addItem("48000");
        ui->inSrCombo->addItem("95000");
        ui->inSrCombo->addItem("96000");
        ui->inSrCombo->addItem("125000");
        ui->inSrCombo->addItem("192000");
        ui->inSrCombo->addItem("250000");
        ui->inSrCombo->addItem("500000");
        ui->inSrCombo->addItem("1000000");
        ui->inSrCombo->addItem("1600000");
        ui->inSrCombo->addItem("2000000");
        if (rate > 0)
        {
            ui->inSrCombo->insertItem(0, QString("%1").arg(rate));
            ui->inSrCombo->setCurrentIndex(0);
        }
        else
            ui->inSrCombo->setCurrentIndex(4); // select 192 kHz
    }
    else
    {
        if (rate > 0)
            ui->inSrCombo->addItem(QString("%1").arg(rate));
    }

    updateDecimations();
}

/**
 * @brief Update available decimations according to the current sample rate.
 *
 * This function will repopulate the decimation selector combo box to only
 * include decimations up to a meaningful maximum value, so that the quadrature
 * rate doesn't get below 48 ksps.
 */
void CIoConfig::updateDecimations(void)
{
    bool        ok;
    int         rate;

    // get current sample rate from combo box
    rate= ui->inSrCombo->currentText().toInt(&ok);
    if (!ok || rate < 0)
        return;

    ui->decimCombo->clear();
    ui->decimCombo->addItem("None", 0);
    if (rate >= 96000)
        ui->decimCombo->addItem("2", 0);
    if (rate >= 192000)
        ui->decimCombo->addItem("4", 0);
    if (rate >= 384000)
        ui->decimCombo->addItem("8", 0);
    if (rate >= 768000)
        ui->decimCombo->addItem("16", 0);
    if (rate >= 1536000)
        ui->decimCombo->addItem("32", 0);
    if (rate >= 3072000)
        ui->decimCombo->addItem("64", 0);
    if (rate >= 6144000)
        ui->decimCombo->addItem("128", 0);
//    if (rate >= 12288000)
//        ui->decimCombo->addItem("256", 0);
//    if (rate >= 24576000)
//        ui->decimCombo->addItem("512", 0);

    decimationChanged(0);
}

void CIoConfig::updateInDev(const QSettings *settings, const std::map<QString, QVariant> &devList)
{
    QString indev = settings->value("input/device", "").toString();
    bool cfgmatch = false; //flag to indicate that device from config was found

    // insert the device list in device combo box
    ui->inDevCombo->clear();
    int i = 0;
    for (auto it = devList.cbegin(); it != devList.cend(); it++, i++)
    {
        auto devstr = (*it).second.toString();
        ui->inDevCombo->addItem((*it).first, devstr);

        // is this the device stored in config?
        if (indev == devstr)
        {
            ui->inDevCombo->setCurrentIndex(i);
            ui->inDevEdit->setText(devstr);
            cfgmatch = true;
        }
    }

    ui->inDevCombo->addItem(tr("Other..."), QVariant(""));

    // If device string from config is not one of the detected devices
    // it could be that device is not plugged in (in which case we select
    // other) or that this is the first time (select the first detected device).
    if (!cfgmatch)
    {
        if (indev.isEmpty())
        {
            // First time config: select the first detected device
            ui->inDevCombo->setCurrentIndex(0);
            ui->inDevEdit->setText(ui->inDevCombo->itemData(0).toString());
            if (ui->inDevEdit->text().isEmpty())
                ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
        }
        else
        {
            // Select other
            ui->inDevCombo->setCurrentIndex(i);
            ui->inDevEdit->setText(indev);
        }
    }
}

void CIoConfig::updateOutDev()
{
    QString outdev = m_settings->value("output/device", "").toString();

    ui->outDevCombo->clear();
    ui->outDevCombo->addItem("Default");

    // get list of audio output devices
#ifdef WITH_PULSEAUDIO
   pa_device_list devices;
   outDevList = devices.get_output_devices();

   qDebug() << __FUNCTION__ << ": Available output devices:";
   for (size_t i = 0; i < outDevList.size(); i++)
   {
       qDebug() << "   " << i << ":" << QString(outDevList[i].get_description().c_str());
       //qDebug() << "     " << QString(outDevList[i].get_name().c_str());
       ui->outDevCombo->addItem(QString(outDevList[i].get_description().c_str()));

       // note that item #i in devlist will be item #(i+1)
       // in combo box due to "default"
       if (outdev == QString(outDevList[i].get_name().c_str()))
           ui->outDevCombo->setCurrentIndex(i+1);
   }
#elif WITH_PORTAUDIO
   portaudio_device_list   devices;

   outDevList = devices.get_output_devices();
   for (size_t i = 0; i < outDevList.size(); i++)
   {
       ui->outDevCombo->addItem(QString(outDevList[i].get_description().c_str()));

       // note that item #i in devlist will be item #(i+1)
       // in combo box due to "default"
       if (outdev == QString(outDevList[i].get_name().c_str()))
           ui->outDevCombo->setCurrentIndex(i+1);
   }
   //ui->outDevCombo->setEditable(true);

#elif defined(Q_OS_DARWIN)
   osxaudio_device_list devices;
   outDevList = devices.get_output_devices();

   qDebug() << __FUNCTION__ << ": Available output devices:";
   for (size_t i = 0; i < outDevList.size(); i++)
   {
       qDebug() << "   " << i << ":" << QString(outDevList[i].get_name().c_str());
       ui->outDevCombo->addItem(QString(outDevList[i].get_name().c_str()));

       // note that item #i in devlist will be item #(i+1)
       // in combo box due to "default"
       if (outdev == QString(outDevList[i].get_name().c_str()))
           ui->outDevCombo->setCurrentIndex(i+1);
   }

#else
   ui->outDevCombo->addItem(m_settings->value("output/device", "Default").toString(),
                            m_settings->value("output/device", "Default").toString());
   ui->outDevCombo->setEditable(true);
#endif // WITH_PULSEAUDIO
}

/**
 * @brief New input device has been selected by the user.
 * @param index The index of the item that has been selected in the combo box.
 */
void CIoConfig::inputDeviceSelected(int index)
{
    qDebug() << "New input device selected:" << index;
    qDebug() << "  Label:" << ui->inDevCombo->itemText(index);
    qDebug() << "  Devstr:" << ui->inDevCombo->itemData(index).toString();

    ui->inDevEdit->setText(ui->inDevCombo->itemData(index).toString());
    updateInputSampleRates(0);
}


/**
 * @brief Input device string has changed
 * @param text THe new device string
 *
 * This slot is activated when the device string in the text edit box has changed
 * either by the user or programmatically. We use this to enable/disable the OK
 * button - we allow OK only if there is some text in the text entry.
 */
void CIoConfig::inputDevstrChanged(const QString &text)
{
    if (text.isEmpty())
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    else
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
}

/**
 * @brief Sample changed, either by selection of direct entry.
 * @param text The new sample rate.
 */
void CIoConfig::inputRateChanged(const QString &text)
{
    (void) text;
    updateDecimations();
}

/**
 * @brief New decimation rate selected.
 * @param index The index of the selected item in the combo box.
 *
 * This function calculates the quadrature rate and updates the sample rate
 * label just below the decimation combo box.
 */
void CIoConfig::decimationChanged(int index)
{
    float       quad_rate;
    int         input_rate;
    int         decim;
    bool        ok;

    decim = idx2decim(index);
    input_rate = ui->inSrCombo->currentText().toInt(&ok);
    if (!ok)
        return;

    quad_rate = (float)input_rate / (float)decim;
    if (quad_rate > 1.e6f)
        ui->sampRateLabel->setText(QString(" %1 Msps").
                                   arg((double)quad_rate * 1.e-6, 0, 'f', 3));
    else
        ui->sampRateLabel->setText(QString(" %1 ksps").
                                   arg((double)quad_rate * 1.e-3, 0, 'f', 3));
}

/**
 * @brief Slot to handle device list updates on m_scanButton clicks
 */
void CIoConfig::onScanButtonClicked()
{
    m_devList->clear();
    CIoConfig::getDeviceList(*m_devList);
    updateInDev(m_settings, *m_devList);
    updateOutDev();
}

/** Convert a combo box index to decimation. */
int CIoConfig::idx2decim(int idx) const
{
    if (idx < 1)
        return 1;

    return (1 << idx);
}

/** Convert a decimation to a combobox index */
int CIoConfig::decim2idx(int decim) const
{
    int         idx;

    if (decim == 0)
        return 0;

    idx = 0;
    while (decim >>= 1)
        ++idx;

    return idx;
}

/** Escape devstr to make some SoapySDR devices work when selected from drop-down */
std::string CIoConfig::escapeDevstr(std::string devstr)
{
    std::stringstream ss1;
    if (devstr.find(" ") == std::string::npos)
        return devstr;
    ss1 << std::quoted(devstr, '\'', '\\');
    return ss1.str();
}
