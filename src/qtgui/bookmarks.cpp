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
#include <Qt>
#include <QFile>
#include <QStringList>
#include <QTextStream>
#include <QString>
#include <QSet>
#include <algorithm>
#include "bookmarks.h"
#include <stdio.h>
#include <wchar.h>

const QColor TagInfo::DefaultColor(Qt::lightGray);
const QString TagInfo::strUntagged("Untagged");
Bookmarks* Bookmarks::m_pThis = 0;

Bookmarks::Bookmarks()
{
     TagInfo tag(TagInfo::strUntagged);
     m_TagList.append(tag);
}

void Bookmarks::create()
{
    m_pThis = new Bookmarks;
}

Bookmarks& Bookmarks::Get()
{
    return *m_pThis;
}

void Bookmarks::setConfigDir(const QString& cfg_dir)
{
    m_bookmarksFile = cfg_dir + "/bookmarks.csv";
    printf("BookmarksFile is %s\n", m_bookmarksFile.toStdString().c_str());
}

void Bookmarks::add(BookmarkInfo &info)
{
    m_BookmarkList.append(info);
    std::stable_sort(m_BookmarkList.begin(),m_BookmarkList.end());
    save();
    emit( BookmarksChanged() );
}

void Bookmarks::remove(int index)
{
    m_BookmarkList.removeAt(index);
    save();
    emit BookmarksChanged();
}

bool Bookmarks::load()
{
    QFile file(m_bookmarksFile);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        m_BookmarkList.clear();
        m_TagList.clear();

        // always create the "Untagged" entry.
        findOrAddTag(TagInfo::strUntagged);

        // Read Tags, until first empty line.
        while (!file.atEnd())
        {
            QString line = QString::fromUtf8(file.readLine().trimmed());

            if(line.isEmpty())
                break;

            if(line.startsWith("#"))
                continue;

            QStringList strings = line.split(";");
            if(strings.count() == 2)
            {
                TagInfo &info = findOrAddTag(strings[0]);
                info.color = QColor(strings[1].trimmed());
            }
            else
            {
                printf("\nBookmarks: Ignoring Line:\n  %s\n", line.toLatin1().data());
            }
        }
        std::sort(m_TagList.begin(),m_TagList.end());

        // Read Bookmarks, after first empty line.
        while (!file.atEnd())
        {
            QString line = QString::fromUtf8(file.readLine().trimmed());
            if(line.isEmpty() || line.startsWith("#"))
                continue;

            QStringList strings = line.split(";");
            if(strings.count() == 5)
            {
                BookmarkInfo info;
                info.frequency  = strings[0].toLongLong();
                info.name       = strings[1].trimmed();
                info.modulation = strings[2].trimmed();
                info.bandwidth  = strings[3].toInt();
                // Multiple Tags may be separated by comma.
                QString strTags = strings[4];
                QStringList TagList = strTags.split(",");
                for(int iTag=0; iTag<TagList.size(); ++iTag)
                {
                  info.tags.append(&findOrAddTag(TagList[iTag].trimmed()));
                }

                m_BookmarkList.append(info);
            }
            else
            {
                printf("\nBookmarks: Ignoring Line:\n  %s\n", line.toLatin1().data());
            }
        }
        file.close();
        std::stable_sort(m_BookmarkList.begin(),m_BookmarkList.end());

        emit BookmarksChanged();
        return true;
    }
    return false;
}

//FIXME: Commas in names
bool Bookmarks::save()
{
    QFile file(m_bookmarksFile);
    if(file.open(QFile::WriteOnly | QFile::Truncate | QIODevice::Text))
    {
        QTextStream stream(&file);

        stream << QString("# Tag name").leftJustified(20) + "; " +
                  QString(" color") << endl;

        QSet<TagInfo*> usedTags;
        for (int iBookmark = 0; iBookmark < m_BookmarkList.size(); iBookmark++)
        {
            BookmarkInfo& info = m_BookmarkList[iBookmark];
            for(int iTag = 0; iTag < info.tags.size(); ++iTag)
            {
              TagInfo& tag = *info.tags[iTag];
              usedTags.insert(&tag);
            }
        }

        for (QSet<TagInfo*>::iterator i = usedTags.begin(); i != usedTags.end(); i++)
        {
            TagInfo& info = **i;
            stream << info.name.leftJustified(20) + "; " + info.color.name() << endl;
        }

        stream << endl;

        stream << QString("# Frequency").leftJustified(12) + "; " +
                  QString("Name").leftJustified(25)+ "; " +
                  QString("Modulation").leftJustified(20) + "; " +
                  QString("Bandwidth").rightJustified(10) + "; " +
                  QString("Tags") << endl;

        for (int i = 0; i < m_BookmarkList.size(); i++)
        {
            BookmarkInfo& info = m_BookmarkList[i];
            QString line = QString::number(info.frequency).rightJustified(12) +
                    "; " + info.name.leftJustified(25) + "; " +
                    info.modulation.leftJustified(20)+ "; " +
                    QString::number(info.bandwidth).rightJustified(10) + "; ";
            for(int iTag = 0; iTag<info.tags.size(); ++iTag)
            {
                TagInfo& tag = *info.tags[iTag];
                if(iTag!=0)
                {
                    line.append(",");
                }
                line.append(tag.name);
            }

            stream << line << endl;
        }

        file.close();
        return true;
    }
    return false;
}

QList<BookmarkInfo> Bookmarks::getBookmarksInRange(qint64 low, qint64 high)
{
    BookmarkInfo info;
    info.frequency=low;
    QList<BookmarkInfo>::const_iterator lb = qLowerBound(m_BookmarkList, info);
    info.frequency=high;
    QList<BookmarkInfo>::const_iterator ub = qUpperBound(m_BookmarkList, info);

    QList<BookmarkInfo> found;

    while (lb != ub)
    {
        const BookmarkInfo& info = *lb;
        //if(info.IsActive())
        {
          found.append(info);
        }
        lb++;
    }

    return found;

}

TagInfo &Bookmarks::findOrAddTag(QString tagName)
{
    tagName = tagName.trimmed();

    if (tagName.isEmpty())
        tagName=TagInfo::strUntagged;

    int idx = getTagIndex(tagName);

    if (idx != -1)
        return m_TagList[idx];

    TagInfo info;
    info.name=tagName;
    m_TagList.append(info);
    emit TagListChanged();
    return m_TagList.last();
}

bool Bookmarks::removeTag(QString tagName)
{
    tagName = tagName.trimmed();

    // Do not delete "Untagged" tag.
    if(tagName.compare(TagInfo::strUntagged, tagName)==0)
        return false;

    int idx = getTagIndex(tagName);
    if (idx == -1)
        return false;

    // Delete Tag from all Bookmarks that use it.
    TagInfo* pTagToDelete = &m_TagList[idx];
    for(int i=0; i<m_BookmarkList.size(); ++i)
    {
        BookmarkInfo& bmi = m_BookmarkList[i];
        for(int t=0; t<bmi.tags.size(); ++t)
        {
            TagInfo* pTag = bmi.tags[t];
            if(pTag == pTagToDelete)
            {
                if(bmi.tags.size()>1) bmi.tags.removeAt(t);
                else bmi.tags[0] = &findOrAddTag(TagInfo::strUntagged);
            }
        }
    }

    // Delete Tag.
    m_TagList.removeAt(idx);

    emit BookmarksChanged();
    emit TagListChanged();

    return true;
}

bool Bookmarks::setTagChecked(QString tagName, bool bChecked)
{
    int idx = getTagIndex(tagName);
    if (idx == -1) return false;
    m_TagList[idx].active = bChecked;
    emit BookmarksChanged();
    emit TagListChanged();
    return true;
}

int Bookmarks::getTagIndex(QString tagName)
{
    tagName = tagName.trimmed();
    for (int i = 0; i < m_TagList.size(); i++)
    {
        if (m_TagList[i].name == tagName)
            return i;
    }

    return -1;
}

const QColor BookmarkInfo::GetColor() const
{
    for(int iTag=0; iTag<tags.size(); ++iTag)
    {
        TagInfo& tag = *tags[iTag];
        if(tag.active)
        {
            return tag.color;
        }
    }
    return TagInfo::DefaultColor;
}

bool BookmarkInfo::IsActive() const
{
    bool bActive = false;
    for(int iTag=0; iTag<tags.size(); ++iTag)
    {
        TagInfo& tag = *tags[iTag];
        if(tag.active)
        {
            bActive = true;
            break;
        }
    }
    return bActive;
}
