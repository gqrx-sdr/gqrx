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
#pragma once

#include <QDockWidget>
#include <QTableWidgetItem>
#include "qtgui/bookmarkstablemodel.h"
#include <QItemDelegate>

namespace Ui {
    class DockBookmarks;
}

class ComboBoxDelegateModulation : public QItemDelegate
{
Q_OBJECT
public:
  ComboBoxDelegateModulation(QObject *parent = 0);
  QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
  void setEditorData(QWidget *editor, const QModelIndex &index) const;
  void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
};

class DockBookmarks : public QDockWidget
{
    Q_OBJECT

private:
    Ui::DockBookmarks *ui;
    QMenu*             contextmenu;
    qint64             m_currentFrequency;
    bool               m_updating;

    bool eventFilter(QObject* object, QEvent* event);

public:
    explicit DockBookmarks(QWidget *parent = 0);
    ~DockBookmarks();

    // ui->tableViewFrequencyList
    // ui->tableWidgetTagList
    BookmarksTableModel *bookmarksTableModel;
    QAction* actionAddBookmark;

    void updateTags();
    void updateBookmarks();
    void changeBookmarkTags(int row, int /*column*/);

signals:
    void newBookmarkActivated(qint64, QString, int);

public slots:
    void setNewFrequency(qint64 rx_freq);

private slots:
    void activated(const QModelIndex & index );
    void onDataChanged (const QModelIndex & topLeft, const QModelIndex & bottomRight);
    //void on_addButton_clicked();
    //void on_delButton_clicked();
    void on_tableWidgetTagList_itemChanged(QTableWidgetItem* item);
    void ShowContextMenu(const QPoint&pos);
    bool DeleteSelectedBookmark();
    void doubleClicked(const QModelIndex & index);
};
