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

#ifndef DOCKFREQTABLE_H
#define DOCKFREQTABLE_H

#include <QDockWidget>
#include "qtgui/frequencylisttablemodel.h"

namespace Ui {
    class DockFreqTable;
}

class DockFreqTable : public QDockWidget
{
    Q_OBJECT

private:
    Ui::DockFreqTable *ui; // ui->tableViewFrequencyList
    QString            m_cfg_dir;   /*!< Default config dir, e.g. XDG_CONFIG_HOME. */

public:
    explicit DockFreqTable(const QString& cfg_dir, QWidget *parent = 0);
    ~DockFreqTable();

    FrequencyListTableModel *frequencyListTableModel;

signals:
    void newFrequency(qint64);

public slots:
    void activated(const QModelIndex & index );
    void setNewFrequency(qint64 rx_freq);
};

#endif // DOCKFREQTABLE_H
