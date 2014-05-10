#include "bookmarks.h"
#include <Qt>
#include <QFile>
#include <QStringList>
#include <QTextStream>
#include <QString>
#include <QSet>

QList<TagInfo> Bookmarks::m_TagList = QList<TagInfo>();
QList<BookmarkInfo> Bookmarks::m_BookmarkList = QList<BookmarkInfo>();

void Bookmarks::add(BookmarkInfo &info)
{
    m_BookmarkList.append(info);
    qSort(m_BookmarkList);
}

void Bookmarks::remove(int index)
{
    m_BookmarkList.removeAt(index);
}

bool Bookmarks::load(QString filename)
{
    QFile file(filename);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        m_BookmarkList.clear();
        m_TagList.clear();

        while (!file.atEnd())
        {
            QString line = file.readLine().trimmed();

            if(line.isEmpty())
                break;

            if(line.isEmpty() || line.startsWith("#"))
                continue;

            QStringList strings = line.split(",");
            if(strings.count() == 2)
            {
                TagInfo &info = findOrAddTag(strings[0]);
                info.color = QColor(strings[1].trimmed());
            }
            else
            {
                printf("\nBookmarks: Ignoring Line:\n  %s\n", line.toAscii().data());
            }
        }

        while (!file.atEnd())
        {
            QString line = file.readLine().trimmed();
            if(line.isEmpty() || line.startsWith("#"))
                continue;

            QStringList strings = line.split(",");
            if(strings.count() == 5)
            {
                BookmarkInfo info;
                info.frequency  = strings[0].toInt();
                info.name       = strings[1].trimmed();
                info.modulation = strings[2].trimmed();
                info.bandwidth  = strings[3].toInt();
                info.tag        = &findOrAddTag(strings[4]);

                m_BookmarkList.append(info);
            }
            else
            {
                printf("\nBookmarks: Ignoring Line:\n  %s\n", line.toAscii().data());
            }
        }
        file.close();
        qSort(m_BookmarkList);

        return true;
    }
    return false;
}

//FIXME: Commas in names
bool Bookmarks::save(QString filename)
{
    QFile file(filename);
    if(file.open(QFile::WriteOnly | QFile::Truncate | QIODevice::Text))
    {
        QTextStream stream(&file);

        stream << QString("# Tag name").leftJustified(20)+", " + QString(" color") <<endl;

        QSet<TagInfo*> usedTags;
        for(int i=0; i<m_BookmarkList.size(); i++)
            usedTags.insert(m_BookmarkList[i].tag);

        for (QSet<TagInfo*>::iterator i = usedTags.begin(); i != usedTags.end(); i++)
        {
            TagInfo& info = **i;
            stream << info.name.leftJustified(20) + ", " + info.color.name() << endl;
        }

        stream << endl;

        stream << QString("# Frequency").leftJustified(12)+", "+ QString("Name").leftJustified(25)+ ", " +
                QString("Modulation").leftJustified(20)+ ", " + QString("Bandwidth").rightJustified(10) +
                QString("Tags") << endl;

        for(int i=0; i<m_BookmarkList.size(); i++)
        {
            BookmarkInfo& info = m_BookmarkList[i];
            QString  line = QString::number(info.frequency).rightJustified(12)+", "+ info.name.leftJustified(25)
                    + ", " + info.modulation.leftJustified(20)+ ", " + QString::number(info.bandwidth).rightJustified(10)
                    + ", "+ info.tag->name; //info.tags.join("; ");

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

    while(lb!=ub)
    {
        found.append(*lb);
        lb++;
    }

    return found;

}

TagInfo &Bookmarks::findOrAddTag(QString tagName)
{
    tagName = tagName.trimmed();

    if(tagName.isEmpty())
        tagName="Untagged";

    int idx = getTagIndex(tagName);

    if(idx!=-1)
        return m_TagList[idx];

    TagInfo info;
    info.name=tagName;
    m_TagList.append(info);
    return m_TagList.last();
}

bool Bookmarks::removeTag(QString tagName)
{
    tagName = tagName.trimmed();
    int idx = getTagIndex(tagName);

    if(idx!=-1)
        return false;

    m_TagList.removeAt(idx);
    return true;
}

int Bookmarks::getTagIndex(QString tagName)
{
    tagName = tagName.trimmed();
    for(int i=0; i<m_TagList.size(); i++)
    {
        if(m_TagList[i].name==tagName)
            return i;
    }

    return -1;
}

/*
int Bookmarks::lowerBound(qint64 low)
{
    if(m_BookmarkList.isEmpty())
        return 0;

    BookmarkInfo info;
    info.frequency=low;
    QList<BookmarkInfo>::iterator lb = qLowerBound(m_BookmarkList.begin(), m_BookmarkList.end(), info);
    return lb-m_BookmarkList.begin();
}

int Bookmarks::upperBound(qint64 high)
{
    if(m_BookmarkList.isEmpty())
        return 0;

    BookmarkInfo info;
    info.frequency=high;
    QList<BookmarkInfo>::iterator ub = qUpperBound(m_BookmarkList.begin(), m_BookmarkList.end(), info);
    return ub-m_BookmarkList.begin();
}
*/

/*
void BookmarkInfo::setTags(QString tagString)
{
    tags = tagString.split(QRegExp("[,;]"), QString::SkipEmptyParts);
    for(int i=0; i<tags.count(); i++)
        tags[i]=tags[i].trimmed().remove(QRegExp("[\\(\\)]"));

    tags.removeAll("");
    tags.removeDuplicates();
    tags.sort();

    //FIXME: move?
    for(int i=0; i<tags.count(); i++)
        if(!Bookmarks::m_TagList.contains(tags[i]))
            Bookmarks::m_TagList.append(tags[i]);
}

QString BookmarkInfo::getTagString()
{
    return tags.empty()?"(Untagged)":tags.join(", ");
}

bool BookmarkInfo::hasTags(QString tag)
{
    return tags.contains(tag);
}

bool BookmarkInfo::hasTags(QStringList _tags)
{
    for(int i=0; i<_tags.size(); i++)
    {
        if(tags.contains(_tags[i]))
            return true;
    }

    return false;
}
*/
