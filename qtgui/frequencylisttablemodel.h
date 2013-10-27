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

#ifndef FREQUENCYLISTTABLEMODEL_H
#define FREQUENCYLISTTABLEMODEL_H

#include <QAbstractTableModel>

class FrequencyListTableModel : public QAbstractTableModel
{
    Q_OBJECT

    QString freqTableDir;

public:
    enum EColumns
    {
        COL_FREQUENCY,
        COL_NAME,
        COL_MODULATION,
        COL_BANDWIDTH
    };

    explicit FrequencyListTableModel(QString dir, QObject *parent = 0);
    
    int rowCount ( const QModelIndex & parent = QModelIndex() ) const;
    int columnCount ( const QModelIndex & parent = QModelIndex() ) const;
    QVariant headerData ( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;
    QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;
    Qt::ItemFlags flags ( const QModelIndex & index ) const;

    void update();

signals:
public slots:
    bool load(QString filename);
};

#endif // FREQUENCYLISTTABLEMODEL_H
