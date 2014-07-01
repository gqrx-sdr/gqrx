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
#ifndef BOOKMARKS_H
#define BOOKMARKS_H

#include <QtGlobal>
#include <QString>
#include <QMap>
#include <QList>
#include <QStringList>
#include <QColor>

struct TagInfo
{
    QString name;
    QColor color;
    bool active;

    TagInfo()
    {
        active=true;
        this->color=Qt::lightGray;
    }
};

struct BookmarkInfo
{
    qint64  frequency;
    QString name;
    QString modulation;
    qint64  bandwidth;
    QList<TagInfo*> tags;

    BookmarkInfo()
    {
        this->frequency = 0;
        this->bandwidth = 0;
    }

/*    BookmarkInfo( qint64 frequency, QString name, qint64 bandwidth, QString modulation )
    {
        this->frequency = frequency;
        this->name = name;
        this->modulation = modulation;
        this->bandwidth = bandwidth;
    }
*/
    bool operator<(const BookmarkInfo &other) const
    {
        return frequency < other.frequency;
    }
/*
    void setTags(QString tagString);
    QString getTagString();
    bool hasTags(QString _tags);
    bool hasTags(QStringList _tags);
 */

    const QColor GetColor() const;
    bool IsActive() const;
};

class Bookmarks
{

public:    
    static void add(BookmarkInfo& info);
    static void remove(int index);
    static bool load(QString filename);
    static bool save(QString filename);
    static int size() { return m_BookmarkList.size(); }
    static BookmarkInfo& getBookmark(int i) { return m_BookmarkList[i]; }
    static QList<BookmarkInfo> getBookmarksInRange(qint64 low, qint64 high);
    //static int lowerBound(qint64 low);
    //static int upperBound(qint64 high);

    static QList<TagInfo> getTagList() { return  QList<TagInfo>(m_TagList); }
    static TagInfo& findOrAddTag(QString tagName);
    static int getTagIndex(QString tagName);
    static bool removeTag(QString tagName);

private:
    static QList<BookmarkInfo> m_BookmarkList;
    static QList<TagInfo> m_TagList;


    //friend class BookmarkInfo; //FIXME
};

#endif // BOOKMARKS_H
