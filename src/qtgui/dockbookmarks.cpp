/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
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
#include <QListWidget>

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
    ui->tableViewFrequencyList->setSortingEnabled(true);
    ui->tableViewFrequencyList->sortByColumn(0, Qt::AscendingOrder);

    // Demod Selection in Frequency List Table.
    ComboBoxDelegateModulation* delegateModulation = new ComboBoxDelegateModulation(this);
    ui->tableViewFrequencyList->setItemDelegateForColumn(BookmarksTableModel::COL_MODULATION, delegateModulation);

    // Bookmarks Context menu
    contextmenu = new QMenu(this);
    // MenuItem Delete
    {
        QAction* action = new QAction("Delete Bookmark", this);
        contextmenu->addAction(action);
        connect(action, SIGNAL(triggered()), this, SLOT(DeleteSelectedBookmark()));
    }
    // MenuItem Tune
    {
        QAction* action = new QAction("Tune", this);
        contextmenu->addAction(action);
        connect(action, SIGNAL(triggered()), this, SLOT(tuneHere()));
    }
    // MenuItem Tune and load
    {
        QAction* action = new QAction("Tune and load settings", this);
        contextmenu->addAction(action);
        connect(action, SIGNAL(triggered()), this, SLOT(tuneAndLoad()));
    }
    // MenuItem New demodulator
    {
        QAction* action = new QAction("New demodulator", this);
        contextmenu->addAction(action);
        connect(action, SIGNAL(triggered()), this, SLOT(newDemod()));
    }
    // MenuItem Add
    {
        actionAddBookmark = new QAction("Add Bookmark", this);
        contextmenu->addAction(actionAddBookmark);
    }
    // MenuItem Select Columns
    {
        QAction* action = new QAction("Select columns...", this);
        contextmenu->addAction(action);
        connect(action, SIGNAL(triggered()), this, SLOT(changeVisibleColumns()));
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
    delete ui;
    delete bookmarksTableModel;
}

void DockBookmarks::activated(const QModelIndex & index)
{
    bool activate = false;
    if (index.column() == BookmarksTableModel::COL_NAME)
        activate = true;
    if (index.column() == BookmarksTableModel::COL_FREQUENCY)
        activate = true;
    if (activate)
    {
        BookmarkInfo *info = bookmarksTableModel->getBookmarkAtRow(index.row());
        emit newBookmarkActivated(*info);
    }
}

void DockBookmarks::setNewFrequency(qint64 rx_freq)
{
    m_currentFrequency = rx_freq;
    BookmarkInfo bi;
    bi.frequency = rx_freq;
    const int iBookmarkIndex = Bookmarks::Get().find(bi);
    if (iBookmarkIndex > 0)
    {
        int iRow = bookmarksTableModel->GetRowForBookmarkIndex(iBookmarkIndex);
        if (iRow > 0)
        {
            ui->tableViewFrequencyList->selectRow(iRow);
            ui->tableViewFrequencyList->scrollTo(ui->tableViewFrequencyList->currentIndex(), QAbstractItemView::EnsureVisible);
            return;
        }
    }
    ui->tableViewFrequencyList->clearSelection();
    return;
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
    // Since Key_Delete can be (is) used as a global shortcut, override the
    // shortcut. Accepting a ShortcutOverride causes the event to be delivered
    // again, but as a KeyPress.
    if (event->type() == QEvent::KeyPress || event->type() == QEvent::ShortcutOverride)
    {
        QKeyEvent* pKeyEvent = static_cast<QKeyEvent *>(event);
        if (pKeyEvent->key() == Qt::Key_Delete)
        {
            if (event->type() == QEvent::ShortcutOverride) {
                event->accept();
            }
            else {
                return DeleteSelectedBookmark();
            }
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

bool DockBookmarks::tuneHere()
{
    QModelIndexList selected = ui->tableViewFrequencyList->selectionModel()->selectedRows();
    if (selected.empty())
        return true;
    BookmarkInfo *info = bookmarksTableModel->getBookmarkAtRow(selected.first().row());
    emit newBookmarkActivated(info->frequency);
    return true;
}

bool DockBookmarks::tuneAndLoad()
{
    QModelIndexList selected = ui->tableViewFrequencyList->selectionModel()->selectedRows();
    if (selected.empty())
        return true;
    BookmarkInfo *info = bookmarksTableModel->getBookmarkAtRow(selected.first().row());
    emit newBookmarkActivated(*info);
    return true;
}

bool DockBookmarks::newDemod()
{
    QModelIndexList selected = ui->tableViewFrequencyList->selectionModel()->selectedRows();
    if (selected.empty())
        return true;
    BookmarkInfo *info = bookmarksTableModel->getBookmarkAtRow(selected.first().row());
    emit newBookmarkActivatedAddDemod(*info);
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
    for (int i = 0; i < Modulations::Strings.size(); ++i)
    {
        comboBox->addItem(Modulations::Strings[i]);
    }
    setEditorData(comboBox, index);
    return comboBox;
}

void ComboBoxDelegateModulation::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QComboBox *comboBox = static_cast<QComboBox*>(editor);
    QString value = index.model()->data(index, Qt::EditRole).toString();
    int iModulation = Modulations::GetEnumForModulationString(value);
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
    QStringList tags;

    int iIdx = bookmarksTableModel->GetBookmarksIndexForRow(row);
    BookmarkInfo& bmi = Bookmarks::Get().getBookmark(iIdx);

    // Create and show the Dialog for a new Bookmark.
    // Write the result into variable 'tags'.
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
            tags = taglist->getSelectedTags();

            // Change Tags of Bookmark
            bmi.tags.clear();
            if (tags.size() == 0)
            {
                bmi.tags.append(Bookmarks::Get().findOrAddTag("")); // "Untagged"
            }
            for (int i = 0; i < tags.size(); ++i)
            {
                bmi.tags.append(Bookmarks::Get().findOrAddTag(tags[i]));
            }
            Bookmarks::Get().save();
        }
    }
}

void DockBookmarks::changeVisibleColumns()
{
    QDialog dialog(this);
    dialog.setWindowTitle("Change Visible Columns");

    QListWidget* colList = new QListWidget(&dialog);
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                            | QDialogButtonBox::Cancel);
    connect(buttonBox, SIGNAL(accepted()), &dialog, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), &dialog, SLOT(reject()));

    QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);
    mainLayout->addWidget(colList);
    mainLayout->addWidget(buttonBox);
    for (int k = 0 ; k < BookmarksTableModel::COLUMN_COUNT ; k++)
    {
        QListWidgetItem* qi = new QListWidgetItem(ui->tableViewFrequencyList->model()->headerData(k, Qt::Horizontal, Qt::DisplayRole).toString(), colList, 0);
        qi->setCheckState(ui->tableViewFrequencyList->isColumnHidden(k) ? Qt::Unchecked : Qt::Checked);
        if (k <= BookmarksTableModel::COL_NAME)
            qi->setFlags(qi->flags() & ~Qt::ItemIsEnabled);
        colList->addItem(qi);
    }

    if (dialog.exec())
    {
        for (int k = 0 ; k < BookmarksTableModel::COLUMN_COUNT ; k++)
            ui->tableViewFrequencyList->setColumnHidden(k, colList->item(k)->checkState() == Qt::Unchecked);
    }
}

void DockBookmarks::saveSettings(QSettings *settings)
{
    QStringList list;
    if (!settings)
        return;

    settings->beginGroup("bookmarks");

    for (int k = 0 ; k < BookmarksTableModel::COLUMN_COUNT ; k++)
        if (ui->tableViewFrequencyList->isColumnHidden(k))
            list.append(ui->tableViewFrequencyList->model()->headerData(k, Qt::Horizontal, Qt::DisplayRole).toString());
    if (list.size() > 0)
        settings->setValue("hidden_columns", list.join(","));
    else
        settings->remove("hidden_columns");
    settings->setValue("splitter_sizes",ui->splitter->saveState());
    settings->endGroup();
}

void DockBookmarks::readSettings(QSettings *settings)
{
    if (!settings)
        return;

    settings->beginGroup("bookmarks");

    QString strval = settings->value("hidden_columns", "").toString();
    QStringList list = strval.split(",");
    for (int k = 0 ; k < BookmarksTableModel::COLUMN_COUNT ; k++)
        ui->tableViewFrequencyList->setColumnHidden(k, list.contains(ui->tableViewFrequencyList->model()->headerData(k, Qt::Horizontal, Qt::DisplayRole).toString()));

    ui->splitter->restoreState(settings->value("splitter_sizes").toByteArray());
    settings->endGroup();
}
