/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2023 0penBrain
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
#ifndef DLG_SAVEABLESTRING_H
#define DLG_SAVEABLESTRING_H

#include <QStyledItemDelegate>
#include <QValidator>

class LineEditDelegateSaveableString : public QStyledItemDelegate
{
Q_OBJECT
public:
  LineEditDelegateSaveableString(QObject *parent = 0);
  QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
  //void setEditorData(QWidget *editor, const QModelIndex &index) const;
  //void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
};

class SaveableStringValidator : public QValidator
{
Q_OBJECT
public:
    SaveableStringValidator(QObject *parent = 0);
    QValidator::State validate(QString &input, int &pos) const;
};

#endif // DLG_SAVEABLESTRING_H
