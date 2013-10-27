#ifndef BOOKMARKS_H
#define BOOKMARKS_H

#include <QtGlobal>
#include <QString>
#include <QMap>
#include <QList>


struct BookmarkInfo
{
    qint64  frequency;
    QString name;
    QString modulation;
    qint64  bandwidth;

    BookmarkInfo()
    {
        this->frequency = 0;
        this->bandwidth = 0;
    }

    BookmarkInfo( qint64 frequency, QString name, qint64 bandwidth, QString modulation )
    {
        this->frequency = frequency;
        this->name = name;
        this->modulation = modulation;
        this->bandwidth = bandwidth;
    }

    bool operator<(const BookmarkInfo &other) const
    {
        return frequency < other.frequency;
    }
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

private:
    static QList<BookmarkInfo> m_BookmarkList;
};

#endif // BOOKMARKS_H
