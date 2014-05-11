/* -*- c++ -*- */
/*
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

#include "dockbookmarks.h"
#include "ui_dockbookmarks.h"
#include "applications/gqrx/bookmarks.h"
#include "qtcolorpicker.h"

#include <QDir>
#include <QInputDialog>
#include <QMessageBox>
#include <QColorDialog>

DockBookmarks::DockBookmarks(const QString& cfg_dir, QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::DockBookmarks),
    m_cfg_dir(cfg_dir)
{
    ui->setupUi(this);
    m_bookmarksFile = m_cfg_dir + "/bookmarks.csv";

    bookmarksTableModel = new BookmarksTableModel();

    /*
    // Fill ComboBox
    QDir dir(m_freqTableDir);
    QStringList filters;
    filters << "*.csv";
    dir.setNameFilters(filters);
    QStringList tables = dir.entryList();
    ui->comboBoxSelectFreqTable->addItems(tables);
    */

    // Frequency List
    ui->tableViewFrequencyList->setModel(bookmarksTableModel);
    ui->tableViewFrequencyList->setColumnWidth( BookmarksTableModel::COL_NAME,
    ui->tableViewFrequencyList->columnWidth(BookmarksTableModel::COL_NAME)*2 );
    ui->tableViewFrequencyList->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableViewFrequencyList->setSelectionMode(QAbstractItemView::SingleSelection);

    ui->tableViewFrequencyList->installEventFilter(this);

    connect(ui->tableViewFrequencyList, SIGNAL(activated(const QModelIndex &)), this, SLOT(activated(const QModelIndex &)));
    connect(bookmarksTableModel, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)), this, SLOT(onDataChanged(const QModelIndex &, const QModelIndex &)));

    // Update GUI
    bookmarksTableModel->load(m_bookmarksFile); //ui->comboBoxSelectFreqTable->currentText());

    m_currentFrequency=0;

    m_updating=false;
    updateTags();

    ui->tagList->setColumnWidth(0, 20);
}

DockBookmarks::~DockBookmarks()
{
    delete bookmarksTableModel;
    bookmarksTableModel = 0;
}

void DockBookmarks::activated(const QModelIndex & index )
{
    BookmarkInfo *info = bookmarksTableModel->getBookmarkAtRow(index.row());
    emit newFrequency(info->frequency);
}

void DockBookmarks::setNewFrequency(qint64 rx_freq)
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

void DockBookmarks::updateTags()
{
    m_updating=true;

    QStringList unchecked;
    for(int i=0; i<ui->tagList->rowCount(); i++)
    {
        if(ui->tagList->item(i,1)->checkState()==Qt::Unchecked)
            unchecked.append(ui->tagList->item(i,1)->text());
    }

    QList<TagInfo> newTags = Bookmarks::getTagList();

    ui->tagList->clear();
    ui->tagList->setSortingEnabled(false);
    ui->tagList->setRowCount(newTags.size());
    for(int i=0; i<newTags.count(); i++)
    {
        QTableWidgetItem *item = new QTableWidgetItem(newTags[i].name);
        item->setCheckState(unchecked.contains(newTags[i].name)?Qt::Unchecked:Qt::Checked);
        ui->tagList->setItem(i, 1, item);

        item = new QTableWidgetItem();
        //item->setData(Qt::DisplayRole | Qt::EditRole, Qt::red);
        item->setFlags(Qt::ItemIsEnabled);
        item->setBackgroundColor(newTags[i].color);
        ui->tagList->setItem(i, 0, item);
    }

    ui->tagList->setSortingEnabled(true);

    m_updating=false;
}

void DockBookmarks::updateBookmarks()
{
    bookmarksTableModel->update();
}

//Data has been edited
void DockBookmarks::onDataChanged(const QModelIndex&, const QModelIndex &)
{
    updateTags();
    Bookmarks::save(m_bookmarksFile);
}

/*void DockFreqTable::on_addButton_clicked()
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
        info.tag=&Bookmarks::findOrAddTag("");
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
        Bookmarks::save(m_freqTableDir+ui->comboBoxSelectFreqTable->currentText());
        frequencyListTableModel->update();
    }
}*/

void DockBookmarks::on_comboBoxSelectFreqTable_currentIndexChanged(const QString &text)
{
    bookmarksTableModel->load(text);
    updateTags();
}

void DockBookmarks::on_tagList_cellActivated(int row, int column)
{
    TagInfo &info = Bookmarks::findOrAddTag(ui->tagList->item(row, 1)->text());
    QColor color = QColorDialog::getColor(info.color, this);

    if(!color.isValid())
        return;

    info.color=color;
    updateTags();
    Bookmarks::save(m_bookmarksFile);
}

void DockBookmarks::on_tagList_itemChanged(QTableWidgetItem *item)
{
    if(m_updating || !item->column()==1)
        return;

    TagInfo &info = Bookmarks::findOrAddTag(item->text());
    info.active=item->checkState()==Qt::Checked;

    bookmarksTableModel->update();
    Bookmarks::save(m_bookmarksFile);
}

bool DockBookmarks::eventFilter(QObject* object, QEvent* event)
{
  if (event->type()==QEvent::KeyPress)
  {
    QKeyEvent* pKeyEvent=static_cast<QKeyEvent*>(event);
    if (pKeyEvent->key() == Qt::Key_Delete && ui->tableViewFrequencyList->hasFocus())
    {
        QModelIndexList selected = ui->tableViewFrequencyList->selectionModel()->selectedRows();

        if(selected.empty())
            return true;

        if(QMessageBox::question(this, "Delete bookmark", "Really delete?", QMessageBox::Yes|QMessageBox::No)==QMessageBox::Yes)
        {
            Bookmarks::remove(selected.first().row());
            Bookmarks::save(m_bookmarksFile);
            bookmarksTableModel->update();
        }

      return true;
    }
  }
  return QWidget::eventFilter(object, event);
}
