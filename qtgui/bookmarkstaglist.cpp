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
#include "bookmarkstaglist.h"
#include "bookmarks.h"
#include <QColorDialog>
#include <stdio.h>
#include <QMenu>
#include <QHeaderView>

BookmarksTagList::BookmarksTagList(QWidget *parent, bool bShowUntagged )
    : QTableWidget(parent)
    , m_bUpdating(false)
    , m_bShowUntagged(bShowUntagged)
{
    connect(this, SIGNAL(cellClicked(int,int)),
            this, SLOT(on_cellClicked(int,int)));

    // right click menu
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, SIGNAL(customContextMenuRequested(const QPoint&)),
            this, SLOT(ShowContextMenu(const QPoint&)));

    horizontalHeader()->setVisible(false);
    verticalHeader()->setVisible(false);
    setColumnCount(2);
    setColumnWidth(0, 20);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSortingEnabled(true);
}

void BookmarksTagList::on_cellClicked(int row, int column)
{
    if(column==0)
    {
        changeColor(row, column);
    }
    if(column==1)
    {
        toggleCheckedState(row, column);
    }
}

void BookmarksTagList::changeColor(int row, int /*column*/)
{
    TagInfo &info = Bookmarks::Get().findOrAddTag(item(row, 1)->text());
    QColor color = QColorDialog::getColor(info.color, this);

    if(!color.isValid())
        return;

    info.color=color;
    updateTags();
    Bookmarks::Get().save();
}

void BookmarksTagList::toggleCheckedState(int row, int column)
{
    QTableWidgetItem* p = item(row,column);
    if(p->checkState()==Qt::Unchecked)
    {
        p->setCheckState(Qt::Checked);
    }
    else
    {
        p->setCheckState(Qt::Unchecked);
    }
}

void BookmarksTagList::updateTags()
{
    m_bUpdating = true;

    // Remember which items were unchecked.
    QStringList unchecked;
    for(int i=0; i<rowCount(); i++)
    {
        if(item(i,1)->checkState()==Qt::Unchecked)
            unchecked.append(item(i,1)->text());
    }

    // Get current List of Tags.
    QList<TagInfo> newTags = Bookmarks::Get().getTagList();
    if(!m_bShowUntagged)
    {
        for(int i=0; i<newTags.size(); ++i)
        {
            TagInfo& taginfo = newTags[i];
            if(taginfo.name.compare(TagInfo::strUntagged)==0)
            {
                newTags.removeAt(i);
                break;
            }
        }
    }

    // Rebuild List in GUI.
    clear();
    setSortingEnabled(false);
    setRowCount(0);
    for(int i=0; i<newTags.count(); i++)
    {
        AddTag(newTags[i].name,
                  ( unchecked.contains(newTags[i].name) ? Qt::Unchecked : Qt::Checked ),
                  newTags[i].color);
    }
    setSortingEnabled(true);

    m_bUpdating = false;
}

void BookmarksTagList::setSelectedTagsAsString(const QString& strTags)
{
    QStringList list = strTags.split(",");
    int iRows = rowCount();
    for(int i=0; i<iRows; ++i)
    {
        QTableWidgetItem* pItem = item(i,1);
        QString name = pItem->text();
        bool bChecked = list.contains(name);
        pItem->setCheckState(bChecked ? Qt::Checked : Qt::Unchecked);
    }
    setSortingEnabled(true);
}

void BookmarksTagList::setSelectedTags(QList<TagInfo*> tags)
{
    int iRows = rowCount();
    for(int i=0; i<iRows; ++i)
    {
        QTableWidgetItem* pItem = item(i,1);
        QString name = pItem->text();
        bool bChecked = false;
        for(QList<TagInfo*>::const_iterator it=tags.begin(), itend=tags.end(); it!=itend; ++it)
        {
            TagInfo* pTag = *it;
            if(pTag->name == name) bChecked = true;
        }
        pItem->setCheckState(bChecked ? Qt::Checked : Qt::Unchecked);
    }
    setSortingEnabled(true);
}

QString BookmarksTagList::getSelectedTagsAsString()
{
    QString strResult;

    int iRows = rowCount();
    bool bFirst = true;
    for(int i=0; i<iRows; ++i)
    {
        QTableWidgetItem* pItem = item(i,1);
        if(pItem->checkState() == Qt::Checked)
        {
            if(!bFirst) strResult += ", ";
            strResult += pItem->text();
            bFirst = false;
        }
    }
    return strResult;
}

void BookmarksTagList::ShowContextMenu(const QPoint& pos)
{
    QModelIndex index = indexAt(pos);
    //printf("Show Context Menu %d\n", index.row());
    QMenu* menu=new QMenu(this);

    // Rename currently does not work.
    // The problem is that after the tag name is changed in GUI
    // you can not find the right TagInfo because you dont know
    // the old tag name.
    #if 0
    // MenuItem "Rename"
    {
        QAction* actionRename = new QAction("Rename", this);
        menu->addAction(actionRename);
        connect(actionRename, SIGNAL(triggered()), this, SLOT(RenameSelectedTag()));
    }
    #endif

    // MenuItem "Create new Tag"
    {
        QAction* actionNewTag = new QAction("Create new Tag", this);
        menu->addAction(actionNewTag);
        connect(actionNewTag, SIGNAL(triggered()), this, SLOT(AddNewTag()));
    }

    // Menu "Delete Tag"
    {
        QAction* actionDeleteTag = new QAction("Delete Tag", this);
        menu->addAction(actionDeleteTag);
        connect(actionDeleteTag, SIGNAL(triggered()), this, SLOT(DeleteSelectedTag()));
    }

    // Menu "Select All"
    {
        QAction* action = new QAction("Select All", this);
        menu->addAction(action);
        connect(action, SIGNAL(triggered()), this, SLOT(SelectAll()));
    }

    // Menu "Deselect All"
    {
        QAction* action = new QAction("Deselect All", this);
        menu->addAction(action);
        connect(action, SIGNAL(triggered()), this, SLOT(DeselectAll()));
    }

    menu->popup(viewport()->mapToGlobal(pos));
}

#if 0
bool BookmarksTagList::RenameSelectedTag()
{
    QModelIndexList selected = selectionModel()->selectedRows();

    if(selected.empty())
    {
        return true;
    }

    int iRow = selected.first().row();
    QTableWidgetItem* pItem = item(iRow,1);bUpdating
    editItem(pItem);
    //Bookmarks::Get().save();

    return true;
}
#endif

void BookmarksTagList::AddNewTag()
{
    AddTag("*new*");
    scrollToBottom();
    editItem(item(rowCount()-1, 1));
}

void BookmarksTagList::AddTag(QString name, Qt::CheckState checkstate, QColor color)
{
    int i = rowCount();
    setRowCount(i+1);

    // Column 1
    QTableWidgetItem *item = new QTableWidgetItem(name);
    item->setCheckState(checkstate);
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled);
    setItem(i, 1, item);

    // Column 0
    item = new QTableWidgetItem();
    item->setFlags(Qt::ItemIsEnabled);
    item->setBackgroundColor(color);
    setItem(i, 0, item);
}

void BookmarksTagList::DeleteSelectedTag()
{
    QModelIndexList selected = selectionModel()->selectedRows();
    if(selected.empty())
    {
        return;
    }
    int iRow = selected.first().row();
    QTableWidgetItem* pItem = item(iRow,1);
    QString strTagName = pItem->text();
    DeleteTag(strTagName);
    return;
}

void BookmarksTagList::DeleteTag(const QString& name)
{
    Bookmarks::Get().removeTag(name);
    updateTags();
}

void BookmarksTagList::SelectAll()
{
    int iRows = rowCount();
    for(int i=0; i<iRows; ++i)
    {
        QTableWidgetItem* pItem = item(i,1);
        QString name = pItem->text();
        pItem->setCheckState(Qt::Checked);
    }
}

void BookmarksTagList::DeselectAll()
{
    int iRows = rowCount();
    for(int i=0; i<iRows; ++i)
    {
        QTableWidgetItem* pItem = item(i,1);
        QString name = pItem->text();
        pItem->setCheckState(Qt::Unchecked);
    }
}
