#include "bookmarks.h"
#include <Qt>
#include <QFile>
#include <QStringList>
#include <QTextStream>

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
        while (!file.atEnd())
        {
            QString line = file.readLine().trimmed();
            if(line.count()>0 && line.left(1).compare("#") != 0)
            {
                QStringList strings = line.split(",");
                if(strings.count() == 4)
                {
                    BookmarkInfo info;
                    info.frequency  = strings[0].toInt();
                    info.name       = strings[1].trimmed();
                    info.modulation = strings[2].trimmed();
                    info.bandwidth  = strings[3].toInt();
                    m_BookmarkList.append(info);
                }
                else
                {
                    printf("\nBookmarks: Ignoring Line:\n  %s\n", line.toAscii().data());
                }
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
        stream << QString("# Frequency").leftJustified(12)+", "+ QString("Name").leftJustified(25)+ ", " +
                QString("Modulation").leftJustified(20)+ ", " + QString("Bandwidth").rightJustified(10) << endl;

        for(int i=0; i<m_BookmarkList.size(); i++)
        {
            BookmarkInfo& info = m_BookmarkList[i];
            QString  line = QString::number(info.frequency).rightJustified(12)+", "+ info.name.leftJustified(25)
                    + ", " + info.modulation.leftJustified(20)+ ", " + QString::number(info.bandwidth).rightJustified(10);

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
