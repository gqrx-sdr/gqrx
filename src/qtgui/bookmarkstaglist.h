/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           http://gqrx.dk/
 *
 * Copyright 2014 Stefano Leucci, Christian Lindner DL2VCL.
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
#ifndef BOOKMARKSTAGLIST_H
#define BOOKMARKSTAGLIST_H

#include <QTableWidget>
#include "bookmarks.h"

/// A QWidget containing the List of Bookmark-Tags.
class BookmarksTagList : public QTableWidget
{
    Q_OBJECT
public:
    explicit BookmarksTagList(QWidget *parent = 0, bool bShowUntagged = true);
    QString getSelectedTagsAsString();
    void setSelectedTagsAsString(const QString& strTags);
    void setSelectedTags(QList<TagInfo*> tags);
    bool m_bUpdating;

private:
    bool m_bShowUntagged;

signals:

public slots:
    void updateTags();
    void on_cellClicked(int row, int column);
    void changeColor(int row, int column);
    void toggleCheckedState(int row, int column);
    void ShowContextMenu(const QPoint& pos);
    //bool RenameSelectedTag();
    void AddNewTag();
    void AddTag(QString name, Qt::CheckState checkstate = Qt::Checked, QColor color = TagInfo::DefaultColor);
    void DeleteSelectedTag();
    void DeleteTag(const QString& name);
    void SelectAll();
    void DeselectAll();
};

#endif // BOOKMARKSTAGLIST_H
