/* -*- c++ -*- */
/*
 * Copyright 2013 Christian Lindner DL2VCL.  
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

#include "dockfreqtable.h"
#include "ui_dockfreqtable.h"
#include "applications/gqrx/bookmarks.h"
#include <QDir>
#include <QInputDialog>
#include <QMessageBox>

DockFreqTable::DockFreqTable(const QString& cfg_dir, QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::DockFreqTable),
    m_cfg_dir(cfg_dir)
{
    ui->setupUi(this);
    m_freqTableDir = m_cfg_dir + "/frequency-list/";
    frequencyListTableModel = new FrequencyListTableModel(m_freqTableDir);

    // Fill ComboBox
    QDir dir(m_freqTableDir);
    QStringList filters;
    filters << "*.csv";
    dir.setNameFilters(filters);
    QStringList tables = dir.entryList();
    ui->comboBoxSelectFreqTable->addItems(tables);
    connect(ui->comboBoxSelectFreqTable, SIGNAL(currentIndexChanged(QString)),
            frequencyListTableModel, SLOT(load(QString)));

    // Frequency List
    ui->tableViewFrequencyList->setModel(frequencyListTableModel);
    ui->tableViewFrequencyList->setColumnWidth( FrequencyListTableModel::COL_NAME,
        ui->tableViewFrequencyList->columnWidth(FrequencyListTableModel::COL_NAME)*2 );
    ui->tableViewFrequencyList->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableViewFrequencyList->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(ui->tableViewFrequencyList, SIGNAL(activated(const QModelIndex &)), this, SLOT(activated(const QModelIndex &)));

    // Update GUI
    frequencyListTableModel->load(ui->comboBoxSelectFreqTable->currentText());

    m_currentFrequency=0;
}

DockFreqTable::~DockFreqTable()
{
    delete frequencyListTableModel;
    frequencyListTableModel = 0;
}


void DockFreqTable::activated(const QModelIndex & index )
{
    qint64 freq = Bookmarks::getBookmark(index.row()).frequency;
    emit newFrequency(freq);
}

void DockFreqTable::setNewFrequency(qint64 rx_freq)
{
    //FIXME: Linear search? Can be done in O(log n) with interval trees
    ui->tableViewFrequencyList->clearSelection();
    for(int row=0; row<Bookmarks::size(); ++row)
    {
        BookmarkInfo& info = Bookmarks::getBookmark(row);
        if( abs(rx_freq - info.frequency) <= ((info.bandwidth/2)+1) )
        {
            ui->tableViewFrequencyList->selectRow(row);
            ui->tableViewFrequencyList->scrollTo( ui->tableViewFrequencyList->currentIndex(), QAbstractItemView::EnsureVisible );
            break;
        }
    }
    m_currentFrequency=rx_freq;
}

void DockFreqTable::on_addButton_clicked()
{
    bool ok=false;
    QString name = QInputDialog::getText(this, "New bookmark", "Bookmark name:", QLineEdit::Normal, "New bookmark", &ok);

    if(ok)
    {
        BookmarkInfo info;
        info.frequency=m_currentFrequency;
        info.bandwidth=0; //FIXME
        info.modulation="Unknown"; //FIXME
        info.name=name;
        Bookmarks::add(info);
        Bookmarks::save(m_freqTableDir+ui->comboBoxSelectFreqTable->currentText());
        frequencyListTableModel->update();
        //FIXME: Update plotter
    }
}

void DockFreqTable::on_delButton_clicked()
{
    QModelIndexList selected = ui->tableViewFrequencyList->selectionModel()->selectedRows();

    if(selected.empty())
        return;

    if(QMessageBox::question(this, "Delete bookmark", "Really delete?", QMessageBox::Yes|QMessageBox::No)==QMessageBox::Yes)
    {
        Bookmarks::remove(selected.first().row());
        frequencyListTableModel->update();
    }
}
