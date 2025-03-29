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

#include "dlg_saveablestring.h"

#include <QDebug>
#include <QLineEdit>

LineEditDelegateSaveableString::LineEditDelegateSaveableString(QObject *parent)
:QStyledItemDelegate(parent)
{

}

#include <QDebug>
QWidget *LineEditDelegateSaveableString::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    auto* editor = QStyledItemDelegate::createEditor(parent, option, index);
    auto* lineedit = qobject_cast<QLineEdit*>(editor);
    if (!lineedit)
    {
        qWarning() << __FUNCTION__ << " : " << "Editor cannot be cast to LineEdit, entry will not be validated";
    }
    else
    {
        lineedit->setValidator(new SaveableStringValidator(lineedit));
    }
    return editor;
}

SaveableStringValidator::SaveableStringValidator(QObject *parent)
:QValidator(parent)
{

}

QValidator::State SaveableStringValidator::validate(QString &input, int &pos) const
{
    if (input.contains(';') || input.contains(','))
    {
        return QValidator::Invalid;
    }
    return QValidator::Acceptable;
}
