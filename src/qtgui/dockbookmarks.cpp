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
#include <cmath>
#include <cstdlib>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>

#include "bookmarks.h"
#include "bookmarkstaglist.h"
#include "dockbookmarks.h"
#include "dockrxopt.h"
#include "qtcolorpicker.h"
#include "ui_dockbookmarks.h"

DockBookmarks::DockBookmarks(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::DockBookmarks)
{
    ui->setupUi(this);

    bookmarksTableModel = new BookmarksTableModel();

    // Frequency List
    ui->tableViewFrequencyList->setModel(bookmarksTableModel);
    ui->tableViewFrequencyList->setColumnWidth(BookmarksTableModel::COL_NAME,
        ui->tableViewFrequencyList->columnWidth(BookmarksTableModel::COL_NAME) * 2);
    ui->tableViewFrequencyList->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableViewFrequencyList->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableViewFrequencyList->installEventFilter(this);

    // Demod Selection in Frequency List Table.
    ComboBoxDelegateModulation* delegateModulation = new ComboBoxDelegateModulation(this);
    ui->tableViewFrequencyList->setItemDelegateForColumn(2, delegateModulation);

    // Bookmarks Context menu
    contextmenu = new QMenu(this);
    // MenuItem Delete
    {
        QAction* action = new QAction("Delete Bookmark", this);
        contextmenu->addAction(action);
        connect(action, SIGNAL(triggered()), this, SLOT(DeleteSelectedBookmark()));
    }
    // MenuItem Add
    {
        actionAddBookmark = new QAction("Add Bookmark", this);
        contextmenu->addAction(actionAddBookmark);
    }
    ui->tableViewFrequencyList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->tableViewFrequencyList, SIGNAL(customContextMenuRequested(const QPoint&)),
        this, SLOT(ShowContextMenu(const QPoint&)));

    // Update GUI
    Bookmarks::Get().load();
    bookmarksTableModel->update();

    m_currentFrequency = 0;
    m_updating = false;

    // TagList
    updateTags();

    connect(ui->tableViewFrequencyList, SIGNAL(activated(const QModelIndex &)),
            this, SLOT(activated(const QModelIndex &)));
    connect(ui->tableViewFrequencyList, SIGNAL(doubleClicked(const QModelIndex &)),
            this, SLOT(doubleClicked(const QModelIndex &)));
    connect(bookmarksTableModel, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)),
            this, SLOT(onDataChanged(const QModelIndex &, const QModelIndex &)));
    connect(&Bookmarks::Get(), SIGNAL(TagListChanged()),
            ui->tableWidgetTagList, SLOT(updateTags()));
    connect(&Bookmarks::Get(), SIGNAL(BookmarksChanged()),
            bookmarksTableModel, SLOT(update()));
}

DockBookmarks::~DockBookmarks()
{
    delete bookmarksTableModel;
    bookmarksTableModel = 0;
}

void DockBookmarks::activated(const QModelIndex & index)
{
    BookmarkInfo *info = bookmarksTableModel->getBookmarkAtRow(index.row());
    emit newBookmarkActivated(info->frequency, info->modulation, info->bandwidth);
}

void DockBookmarks::setNewFrequency(qint64 rx_freq)
{
    ui->tableViewFrequencyList->clearSelection();
    const int iRowCount = bookmarksTableModel->rowCount();
    for (int row = 0; row < iRowCount; ++row)
    {
        BookmarkInfo& info = *(bookmarksTableModel->getBookmarkAtRow(row));
        if (std::abs(rx_freq - info.frequency) <= ((info.bandwidth / 2 ) + 1))
        {
            ui->tableViewFrequencyList->selectRow(row);
            ui->tableViewFrequencyList->scrollTo(ui->tableViewFrequencyList->currentIndex(), QAbstractItemView::EnsureVisible );
            break;
        }
    }
    m_currentFrequency = rx_freq;
}

void DockBookmarks::updateTags()
{
    m_updating = true;
    ui->tableWidgetTagList->updateTags();
    m_updating = false;
}

void DockBookmarks::updateBookmarks()
{
    bookmarksTableModel->update();
}

//Data has been edited
void DockBookmarks::onDataChanged(const QModelIndex&, const QModelIndex &)
{
    updateTags();
    Bookmarks::Get().save();
}

void DockBookmarks::on_tableWidgetTagList_itemChanged(QTableWidgetItem *item)
{
    // we only want to react on changed by the user, not changes by the program itself.
    if(ui->tableWidgetTagList->m_bUpdating) return;

    int col = item->column();
    if (col != 1)
        return;

    QString strText = item->text();
    Bookmarks::Get().setTagChecked(strText, (item->checkState() == Qt::Checked));
}

bool DockBookmarks::eventFilter(QObject* object, QEvent* event)
{
    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent* pKeyEvent = static_cast<QKeyEvent *>(event);
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

    if (selected.empty())
    {
        return true;
    }

    if (QMessageBox::question(this, "Delete bookmark", "Really delete?", QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
    {
        int iIndex = bookmarksTableModel->GetBookmarksIndexForRow(selected.first().row());
        Bookmarks::Get().remove(iIndex);
        bookmarksTableModel->update();
    }
    return true;
}

void DockBookmarks::ShowContextMenu(const QPoint& pos)
{
    contextmenu->popup(ui->tableViewFrequencyList->viewport()->mapToGlobal(pos));
}

void DockBookmarks::doubleClicked(const QModelIndex & index)
{
    if(index.column() == BookmarksTableModel::COL_TAGS)
    {
        changeBookmarkTags(index.row(), index.column());
    }
}

ComboBoxDelegateModulation::ComboBoxDelegateModulation(QObject *parent)
:QItemDelegate(parent)
{
}

QWidget *ComboBoxDelegateModulation::createEditor(QWidget *parent, const QStyleOptionViewItem &/* option */, const QModelIndex &index) const
{
    QComboBox* comboBox = new QComboBox(parent);
    for (int i = 0; i < DockRxOpt::ModulationStrings.size(); ++i)
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

void DockBookmarks::changeBookmarkTags(int row, int /*column*/)
{
    bool ok = false;
    QString tags; // list of tags separated by comma

    int iIdx = bookmarksTableModel->GetBookmarksIndexForRow(row);
    BookmarkInfo& bmi = Bookmarks::Get().getBookmark(iIdx);

    // Create and show the Dialog for a new Bookmark.
    // Write the result into variabe 'tags'.
    {
        QDialog dialog(this);
        dialog.setWindowTitle("Change Bookmark Tags");

        BookmarksTagList* taglist = new BookmarksTagList(&dialog, false);
        taglist->updateTags();
        taglist->setSelectedTags(bmi.tags);
        taglist->DeleteTag(TagInfo::strUntagged);

        QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                              | QDialogButtonBox::Cancel);
        connect(buttonBox, SIGNAL(accepted()), &dialog, SLOT(accept()));
        connect(buttonBox, SIGNAL(rejected()), &dialog, SLOT(reject()));

        QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);
        mainLayout->addWidget(taglist);
        mainLayout->addWidget(buttonBox);

        ok = dialog.exec();
        if (ok)
        {
            tags = taglist->getSelectedTagsAsString();
            // list of selected tags is now in string 'tags'.

            // Change Tags of Bookmark
            QStringList listTags = tags.split(",",QString::SkipEmptyParts);
            bmi.tags.clear();
            if (listTags.size() == 0)
            {
                bmi.tags.append(&Bookmarks::Get().findOrAddTag("")); // "Untagged"
            }
            for (int i = 0; i < listTags.size(); ++i)
            {
                bmi.tags.append(&Bookmarks::Get().findOrAddTag(listTags[i]));
            }
            Bookmarks::Get().save();
        }
    }
}
