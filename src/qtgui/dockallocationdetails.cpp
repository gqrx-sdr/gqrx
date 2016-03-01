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
#include <cmath>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QJsonObject>
#include <QJsonArray>

DockAllocationDetails::DockAllocationDetails(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::DockAllocationDetails)
{
    ui->setupUi(this);
    
    connect(ui->regionComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(switchcall(const QString&)));
 }

DockAllocationDetails::~DockAllocationDetails()
{
    delete ui;
}

void DockAllocationDetails::switchcall(const QString& text)
{    
    clearBandView();
    this->allocationsTable = QJsonDocument::fromJson(QString("").toUtf8());
    
    QNetworkAccessManager *networkManager = new QNetworkAccessManager(this);
    QString urlStr = QString("%1/allocations/tables/%2/index.json").arg(this->baseurl).arg(ui->regionComboBox->itemData(ui->regionComboBox->currentIndex()).toString());
    QNetworkRequest request = QNetworkRequest(QUrl(urlStr));
    
    networkManager->get(request);
    
    connect(networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(onAllocationsTableResult(QNetworkReply*)));
}

void DockAllocationDetails::onAllocationsTableResult(QNetworkReply* reply)
{
    
    if (reply->error() != QNetworkReply::NoError)
    {
        // TODO handle error
        return;
    }
    
    QString data = (QString) reply->readAll();
    
    QJsonDocument document = QJsonDocument::fromJson(data.toUtf8());
    this->allocationsTable = document;
    
    updateBandView();
}

/**
 * Load frequency allocations viewer configuration to settings.
 */
void DockAllocationDetails::readSettings(QSettings *settings)
{
    QString baseurl = settings->value("allocations/baseurl", "").toString();
    if (!baseurl.isEmpty())
    {
        this->baseurl = baseurl;
    }

    initRegionsCombo();

}

/** 
 * Save frequency allocations viewer configuration to settings.
 */
void DockAllocationDetails::saveSettings(QSettings *settings)
{
    // Place holder for now
}

/**
 * Init the regions combo box
 */
void DockAllocationDetails::initRegionsCombo()
{
    QNetworkAccessManager *networkManager = new QNetworkAccessManager(this);
    QString urlStr = QString("%1/allocations/tables/index.json").arg(this->baseurl);
    QNetworkRequest request = QNetworkRequest(QUrl(urlStr));
    
    //std::cout << "AJMAS DockAllocationDetails::initRegionsCombo: " << urlStr.toUtf8().constData() << "\n";
    
    networkManager->get(request);

    connect(networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(onRegionListResult(QNetworkReply*)));
    
    ui->regionComboBox->setEnabled(0);
    
}

void DockAllocationDetails::onRegionListResult(QNetworkReply* reply)
{
    if (reply->error() != QNetworkReply::NoError)
    {
        // TODO handle error
        return;
    }
    
    QString data = (QString) reply->readAll();
 
    QJsonDocument document = QJsonDocument::fromJson(data.toUtf8());
    
    if ( document.isArray() )
    {
        QJsonArray regions = document.array();
        
        for(auto&& item: regions)
        {
            const QJsonObject& region = item.toObject();
            ui->regionComboBox->addItem(region["region"].toString(), region["path"].toString());

        }
        
        ui->regionComboBox->setEnabled(1);
        
    }
    else
    {
         // TODO handle error
    }
    
}


void DockAllocationDetails::clearBandView ()
{
    ui->detailsWidget->clear();
}

/**
 * Updated the displayed information. Depends on an internet connection
 */
void DockAllocationDetails::updateBandView ()
{
    
    // update the label displaying the current band
    
    QString lf_freq_human = frequencyToHuman(double(lf_freq_hz));
    QString uf_freq_human = frequencyToHuman(double(uf_freq_hz));
    
    QString textLabel = QString("%1 - %2").arg(lf_freq_human).arg(uf_freq_human);
    ui->bandValueLabel->setText(textLabel);
    
    // clear the view, so we don't find ourselves appending to old data
    clearBandView();

    if (this->allocationsTable.object().contains("tables")) {
        
        QTreeWidgetItem *treeRootItem = new QTreeWidgetItem(ui->detailsWidget);
        treeRootItem->setText(0, "Allocations");
        
        QFont font;
        font.setBold(true);
        treeRootItem->setFont(0, font);
        
        // find the bands allocations in the given range
        QJsonArray allocations = lookupAllocations(lf_freq_hz, uf_freq_hz);
        
        for(auto&& item: allocations)
        {
            const QJsonObject& band = item.toObject();
                        
            QTreeWidgetItem *treeBandItem = new QTreeWidgetItem();
            treeBandItem->setText(0, QString("%1 - %2")
                                  .arg( frequencyToHuman(band["lf"].toDouble()) )
                                  .arg( frequencyToHuman(band["uf"].toDouble()) )
                                  );
            
            treeRootItem->addChild(treeBandItem);
            
            QJsonArray services = band["services"].toArray();
            
            for(auto&& item2: services)
            {
                const QJsonObject& service = item2.toObject();
                QTreeWidgetItem *serviceItem = new QTreeWidgetItem();
                
                // Add the service description, capitalising the primary allocations
                if ( service["cat"] == "p" ) {
                    serviceItem->setText(0, service["desc"].toString().toUpper());
                } else {
                    serviceItem->setText(0, service["desc"].toString());
                }
                                     
                treeBandItem->addChild(serviceItem);
            }
            
        }
        
    }
 
    // Ensure the tree is expanded by default
    
    ui->detailsWidget->expandAll();

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

/**
 * Takes a frequency value in Hz and converts it to a 'human readable'
 * string, such that the unit is value is converted to the closest
 * magnitude and rounded
 *
 * For example, 1000 -> "1 kHz"
 */
QString DockAllocationDetails::frequencyToHuman(double freq_hz)
{
    QString prefix = QString("");
    QStringList units;
    double value = freq_hz;
    units << "" << "k" << "M" << "G" << "T" << "P" << "E" << "Y";
    
    for (int i=units.size()-1; i >=0; i--) {
        if (value >= pow(10, i*3)) {
            prefix = units.at(i);
            value = value / pow(10, i*3);
            break;
        }
    }
    
    return QString("%1 %2Hz").arg(QString::number(value, 'g', 7)).arg(prefix);
}

/**
 * Looks up the allocation bands that cover the lower and upper frequencies that
 * are passed in as parameter.
 */
QJsonArray DockAllocationDetails::lookupAllocations(qint64 lf_freq_hz, qint64 uf_freq_hz)
{
    QJsonArray tables = this->allocationsTable.object()["tables"].toArray();
    QJsonObject table = tables[0].toObject();
    QJsonArray bands = table["bands"].toArray();
    
    QJsonArray filteredBands = QJsonArray();
    
    for(auto&& item: bands)
    {
        const QJsonObject& band = item.toObject();
        if ( band["lf"].toDouble() <= uf_freq_hz && band["uf"].toDouble() >= lf_freq_hz ) {
            filteredBands.append(band);
        }
    }
    
    return filteredBands;
}
