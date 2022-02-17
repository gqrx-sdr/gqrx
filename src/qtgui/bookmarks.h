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
#ifndef BOOKMARKS_H
#define BOOKMARKS_H

#include <QtGlobal>
#include <QObject>
#include <QString>
#include <QMap>
#include <QList>
#include <QStringList>
#include <QColor>
#include <memory>
#include "receivers/vfo.h"

struct TagInfo
{
    using sptr = std::shared_ptr<TagInfo>;
    QString name;
    QColor color;
    bool active;

    static const QColor DefaultColor;
    static const QString strUntagged;

    TagInfo(QString name)
    {
        active=true;
        this->color=DefaultColor;
        this->name = name;
    }
    static sptr make(QString name = "")
    {
        return std::make_shared<TagInfo>(name);
    }
    bool operator<(const TagInfo &other) const
    {
        return name < other.name;
    }
};

class BookmarkInfo:public vfo_s
{
    public:
#if GNURADIO_VERSION < 0x030900
    typedef boost::shared_ptr<BookmarkInfo> sptr;
#else
    typedef std::shared_ptr<BookmarkInfo> sptr;
#endif
    static sptr make()
    {
        return sptr(new BookmarkInfo());
    }

    BookmarkInfo():vfo_s(),frequency(0)
    {
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
    bool operator==(const BookmarkInfo &other) const
    {
        return frequency == other.frequency;
    }
/*
    void setTags(QString tagString);
    QString getTagString();
    bool hasTags(QString _tags);
    bool hasTags(QStringList _tags);
 */

    const QColor GetColor() const;
    bool IsActive() const;

    qint64  frequency;
    QString name;
    QString modulation;
    QList<TagInfo::sptr> tags;
};

class Bookmarks : public QObject
{
    Q_OBJECT
public:
    // This is a Singleton Class now because you can not send qt-signals from static functions.
    static void create();
    static Bookmarks& Get();

    void add(BookmarkInfo& info);
    void remove(int index);
    void remove(const BookmarkInfo &info);
    bool load();
    bool save();
    int size() { return m_BookmarkList.size(); }
    BookmarkInfo& getBookmark(int i) { return m_BookmarkList[i]; }
    QList<BookmarkInfo> getBookmarksInRange(qint64 low, qint64 high, bool autoAdded = false);
    int find(const BookmarkInfo &info);
    //int lowerBound(qint64 low);
    //int upperBound(qint64 high);

    QList<TagInfo::sptr> getTagList() { return  QList<TagInfo::sptr>(m_TagList); }
    TagInfo::sptr findOrAddTag(QString tagName);
    int getTagIndex(QString tagName);
    bool removeTag(QString tagName);
    bool setTagChecked(QString tagName, bool bChecked);

    void setConfigDir(const QString&);

private:
    Bookmarks(); // Singleton Constructor is private.
    QList<BookmarkInfo>  m_BookmarkList;
    QList<TagInfo::sptr> m_TagList;
    QString              m_bookmarksFile;
    static Bookmarks*    m_pThis;

signals:
    void BookmarksChanged(void);
    void TagListChanged(void);
};

#endif // BOOKMARKS_H
