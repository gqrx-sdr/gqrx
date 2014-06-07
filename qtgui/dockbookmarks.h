/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           http://gqrx.dk/
 *
 * Copyright 2013 Christian Lindner DL2VCL, Stefano Leucci.
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
#ifndef DOCKBOOKMARKS_H
#define DOCKBOOKMARKS_H

#include <QDockWidget>
#include <QTableWidgetItem>
#include "qtgui/bookmarkstablemodel.h"

namespace Ui {
    class DockBookmarks;
}

class DockBookmarks : public QDockWidget
{
    Q_OBJECT

private:
    Ui::DockBookmarks *ui; // ui->tableViewFrequencyList
    QString            m_cfg_dir;   /*!< Default config dir, e.g. XDG_CONFIG_HOME. */
    qint64             m_currentFrequency;
    QString            m_bookmarksFile;
    bool               m_updating;

    bool eventFilter(QObject* object, QEvent* event);

public:
    explicit DockBookmarks(const QString& cfg_dir, QWidget *parent = 0);
    ~DockBookmarks();

    BookmarksTableModel *bookmarksTableModel;

    void updateTags();
    void updateBookmarks();
    QString getBooksmarksFile()
    {
        return m_bookmarksFile;
    }

signals:
    void newFrequency(qint64);
    void newDemodulation(QString);
    void newFilterBandwidth(int, int);

public slots:
    void setNewFrequency(qint64 rx_freq);

private slots:
    void activated(const QModelIndex & index );
    void onDataChanged (const QModelIndex & topLeft, const QModelIndex & bottomRight);
    //void on_addButton_clicked();
    //void on_delButton_clicked();
    void on_tagList_cellActivated(int row, int column);
    void on_tagList_itemChanged(QTableWidgetItem* item);
    void ShowContextMenu(const QPoint&pos);
    bool DeleteSelectedBookmark();
};

#endif // DOCKFREQTABLE_H
