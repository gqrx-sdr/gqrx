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
#include <QDebug>
#include <QVariant>
#include "dockallocationdetails.h"
#include "ui_dockallocationdetails.h"
#include <iostream>

DockAllocationDetails::DockAllocationDetails(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::DockAllocationDetails)
{
    ui->setupUi(this);
    
    initRegionsCombo();

    connect(ui->regionComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(switchcall(const QString&)));
 }

DockAllocationDetails::~DockAllocationDetails()
{
    delete ui;
}

void DockAllocationDetails::switchcall(const QString& text)
{
     //std::cout << "selected value: " << text.toUtf8().constData() << "\n";
     updateBandView();
}

/**
 * Load frequency allocations viewer configuration to settings.
 */
void DockAllocationDetails::readSettings(QSettings *settings)
{
    //std::cout << "AJMAS DockAllocationDetails::readSettings: " << "xxxxx" << "\n";
    QString baseurl = settings->value("allocations/baseurl", "").toString();
    if (!baseurl.isEmpty())
    {
        this->baseurl = baseurl;
    }
    std::cout << "AJMAS DockAllocationDetails::readSettings: " << this->baseurl.toUtf8().constData() << "\n";

}

/** 
 * Save frequency allocations viewer configuration to settings.
 */
void DockAllocationDetails::saveSettings(QSettings *settings)
{
    //std::cout << "AJMAS DockAllocationDetails::saveSettings: " << "xxxxx" << "\n";
}

/**
 * Init the regions combo box
 */
void DockAllocationDetails::initRegionsCombo()
{
    // TODO make this load its values from a URL, via a JSON response
    
//    QString urlStr = QString("%1/rest/tables/?type=json")
//        .arg(this->baseurl);
//    QUrl url = QUrl(urlStr);
//    
//    QNetworkRequest request;
//    request.setUrl(url);
//    
//    QNetworkReply* currentReply = networkManager.get(request);
//    
    //ref: http://blog.mathieu-leplatre.info/access-a-json-webservice-with-qt-c.html
    
    ui->regionComboBox->addItem("ITU Region 1", "itu1");
    ui->regionComboBox->addItem("ITU Region 2", "itu2");
    ui->regionComboBox->addItem("ITU Region 3", "itu3");
    ui->regionComboBox->addItem("Canada", "ca");
    ui->regionComboBox->addItem("United Kingdom", "gb");
    ui->regionComboBox->addItem("USA", "us");
    
}

/**
 * Updated the displayed information. Depends on an internet connection
 */
void DockAllocationDetails::updateBandView ()
{
    // TODO Move this to somewhere more suitable, also accepting a value from the config file
    
    QString url = QString("%1/bandinfo/?lf=%2&uf=%3&region=%4")
        .arg(this->baseurl)
        .arg(QString::number(lf_freq_hz - 20))
        .arg(QString::number(uf_freq_hz + 20))
        .arg(ui->regionComboBox->itemData(ui->regionComboBox->currentIndex()).toString());
    ui->detailWebView->setUrl(QUrl(url));
    
}

/**
 * @brief Set new RF frequency
 * @param freq_hz The frequency in Hz
 *
 * RF frequency is the frequency to which the device device is tuned to
 *
 */
void DockAllocationDetails::setFrequency(qint64 freq_hz)
{
    this->lf_freq_hz = freq_hz;
    this->uf_freq_hz = freq_hz;

    updateBandView();
}
