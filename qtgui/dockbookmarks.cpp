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
#include <QDir>
#include <QInputDialog>
#include <QMessageBox>
#include <QColorDialog>
#include <QMenu>
#include <QComboBox>

#include "bookmarks.h"
#include "dockbookmarks.h"
#include "ui_dockbookmarks.h"
#include "qtcolorpicker.h"
#include "dockrxopt.h"

DockBookmarks::DockBookmarks(const QString& cfg_dir, QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::DockBookmarks),
    m_cfg_dir(cfg_dir)
{
    ui->setupUi(this);
    m_bookmarksFile = m_cfg_dir + "/bookmarks.csv";

    bookmarksTableModel = new BookmarksTableModel();

    // Frequency List
    ui->tableViewFrequencyList->setModel(bookmarksTableModel);
    ui->tableViewFrequencyList->setColumnWidth( BookmarksTableModel::COL_NAME,
    ui->tableViewFrequencyList->columnWidth(BookmarksTableModel::COL_NAME)*2 );
    ui->tableViewFrequencyList->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableViewFrequencyList->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableViewFrequencyList->installEventFilter(this);
    connect(ui->tableViewFrequencyList, SIGNAL(activated(const QModelIndex &)), this, SLOT(activated(const QModelIndex &)));
    connect(bookmarksTableModel, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)), this, SLOT(onDataChanged(const QModelIndex &, const QModelIndex &)));

    // Demod Selection in Frequency List Table.
    ComboBoxDelegateModulation* delegate = new ComboBoxDelegateModulation(this);
    ui->tableViewFrequencyList->setItemDelegateForColumn(2, delegate);

    // right click menu
    ui->tableViewFrequencyList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->tableViewFrequencyList, SIGNAL(customContextMenuRequested(const QPoint&)),
        this, SLOT(ShowContextMenu(const QPoint&)));

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
    //printf("Emit newFrequency(%d)\n", (int)info->frequency);
    emit newFrequency(info->frequency);
    //printf("Emit newDemodulation(%s)\n", info->modulation.toStdString().c_str());
    emit newDemodulation(info->modulation);
    emit newFilterBandwidth(-1*info->bandwidth/2, info->bandwidth/2);
}

void DockBookmarks::setNewFrequency(qint64 rx_freq)
{
    //FIXME: Linear search? Can be done in O(log n) with interval trees
    ui->tableViewFrequencyList->clearSelection();
    const int iRowCount = bookmarksTableModel->rowCount();
    for(int row=0; row<iRowCount; ++row)
    {
        BookmarkInfo& info = *(bookmarksTableModel->getBookmarkAtRow(row));
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

void DockBookmarks::on_tagList_cellActivated(int row, int /*column*/)
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
        return DeleteSelectedBookmark();
    }
  }
  return QWidget::eventFilter(object, event);
}

bool DockBookmarks::DeleteSelectedBookmark()
{
    QModelIndexList selected = ui->tableViewFrequencyList->selectionModel()->selectedRows();

    if(selected.empty())
    {
        return true;
    }

    if(QMessageBox::question(this, "Delete bookmark", "Really delete?", QMessageBox::Yes|QMessageBox::No)==QMessageBox::Yes)
    {
        int iIndex = bookmarksTableModel->GetBookmarksIndexForRow(selected.first().row());
        Bookmarks::remove(iIndex);
        Bookmarks::save(m_bookmarksFile);
        bookmarksTableModel->update();
    }
    return true;
}

void DockBookmarks::ShowContextMenu(const QPoint& pos)
{
    QModelIndex index = ui->tableViewFrequencyList->indexAt(pos);

    printf("Show Context Menu %d\n", index.row());

    QMenu* menu=new QMenu(this);
    QAction* actionDelete = new QAction("Delete Bookmark", this);
    menu->addAction(actionDelete);
    connect(actionDelete, SIGNAL(triggered()), this, SLOT(DeleteSelectedBookmark()));
    menu->popup(ui->tableViewFrequencyList->viewport()->mapToGlobal(pos));
}


ComboBoxDelegateModulation::ComboBoxDelegateModulation(QObject *parent)
:QItemDelegate(parent)
{
}

QWidget *ComboBoxDelegateModulation::createEditor(QWidget *parent, const QStyleOptionViewItem &/* option */, const QModelIndex &index) const
{
  QComboBox* comboBox = new QComboBox(parent);
  for(int i = 0; i < DockRxOpt::ModulationStrings.size(); ++i)
  {
      comboBox->addItem(DockRxOpt::ModulationStrings[i]);
  }
  setEditorData(comboBox, index);
  return comboBox;
}

void ComboBoxDelegateModulation::setEditorData(QWidget *editor, const QModelIndex &index) const
{
  QComboBox *comboBox = static_cast<QComboBox*>(editor);
  QString value = index.model()->data(index, Qt::EditRole).toString();
  int iModulation = DockRxOpt::GetEnumForModulationString(value);
  comboBox->setCurrentIndex(iModulation);
}

void ComboBoxDelegateModulation::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
  QComboBox *comboBox = static_cast<QComboBox*>(editor);
  model->setData(index, comboBox->currentText(), Qt::EditRole);
}
