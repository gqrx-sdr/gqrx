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
#include <Qt>
#include <QFile>
#include <QStringList>
#include <QTextStream>
#include <QString>
#include <QSet>
#include <algorithm>
#include <iostream>
#include "bookmarks.h"
#include "qtgui/bookmarkstablemodel.h"

const QColor TagInfo::DefaultColor(Qt::lightGray);
const QString TagInfo::strUntagged("Untagged");
Bookmarks* Bookmarks::m_pThis = 0;

Bookmarks::Bookmarks()
{
     TagInfo::sptr tag = TagInfo::make(TagInfo::strUntagged);
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
    std::cout << "BookmarksFile is " << m_bookmarksFile.toStdString() << std::endl;
}

void Bookmarks::add(BookmarkInfo &info)
{
    m_BookmarkList.append(info);
    std::stable_sort(m_BookmarkList.begin(),m_BookmarkList.end());
    save();
}

void Bookmarks::remove(int index)
{
    m_BookmarkList.removeAt(index);
    save();
}

void Bookmarks::remove(const BookmarkInfo &info)
{
    m_BookmarkList.removeOne(info);
    save();
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
                TagInfo::sptr info = findOrAddTag(strings[0]);
                info->color = QColor(strings[1].trimmed());
            }
            else
            {
                std::cout << "Bookmarks: Ignoring Line:" << std::endl;
                std::cout << "  " << line.toStdString() << std::endl;
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
                info.set_demod(Modulations::GetEnumForModulationString(info.modulation));
                int bandwidth  = strings[3].toInt();
                switch(info.get_demod())
                {
                case Modulations::MODE_LSB:
                case Modulations::MODE_CWL:
                    info.set_filter(-100 - bandwidth, -100, bandwidth * 0.2);
                    break;
                case Modulations::MODE_USB:
                case Modulations::MODE_CWU:
                    info.set_filter(100, 100 + bandwidth, bandwidth * 0.2);
                    break;
                default:
                    info.set_filter(-bandwidth / 2, bandwidth / 2, bandwidth * 0.2);
                }
                // Multiple Tags may be separated by comma.
                QString strTags = strings[4];
                QStringList TagList = strTags.split(",");
                for(int iTag=0; iTag<TagList.size(); ++iTag)
                {
                  info.tags.append(findOrAddTag(TagList[iTag].trimmed()));
                }

                m_BookmarkList.append(info);
            }
            else if (strings.count() == BookmarksTableModel::COLUMN_COUNT)
            {
                BookmarkInfo info;
                int i = 0;
                info.frequency  = strings[i++].toLongLong();
                info.name       = strings[i++].trimmed();
                // Multiple Tags may be separated by comma.
                QString strTags = strings[i++];
                QStringList TagList = strTags.split(",");

                for (int iTag = 0; iTag < TagList.size(); ++iTag)
                  info.tags.append(findOrAddTag(TagList[iTag].trimmed()));

                info.set_freq_lock(strings[i++].trimmed() == "true");
                info.modulation = strings[i++].trimmed();
                info.set_demod(Modulations::GetEnumForModulationString(info.modulation));
                info.set_filter_low(strings[i++].toInt());
                info.set_filter_high(strings[i++].toInt());
                info.set_filter_tw(strings[i++].toInt());
                info.set_agc_on(strings[i++].trimmed() == "true");
                info.set_agc_target_level(strings[i++].toInt());
                info.set_agc_manual_gain(strings[i++].toFloat());
                info.set_agc_max_gain(strings[i++].toInt());
                info.set_agc_attack(strings[i++].toInt());
                info.set_agc_decay(strings[i++].toInt());
                info.set_agc_hang(strings[i++].toInt());
                info.set_agc_panning(strings[i++].toInt());
                info.set_agc_panning_auto(strings[i++].trimmed() == "true");
                info.set_cw_offset(strings[i++].toInt());
                info.set_fm_maxdev(strings[i++].toFloat());
                info.set_fm_deemph(1.0e-6f * strings[i++].toFloat());
                info.set_am_dcr(strings[i++].trimmed() == "true");
                info.set_amsync_dcr(strings[i++].trimmed() == "true");
                info.set_amsync_pll_bw(strings[i++].toFloat());
                info.set_nb_on(1, strings[i++].trimmed() == "true");
                info.set_nb_threshold(1, strings[i++].toFloat());
                info.set_nb_on(2, strings[i++].trimmed() == "true");
                info.set_nb_threshold(2, strings[i++].toFloat());
                info.set_audio_rec_dir(strings[i++].trimmed().toStdString());
                info.set_audio_rec_sql_triggered(strings[i++].trimmed() == "true");
                info.set_audio_rec_min_time(strings[i++].toInt());
                info.set_audio_rec_max_gap(strings[i++].toInt());

                m_BookmarkList.append(info);
            }
            else
            {
                std::cout << "Bookmarks: Ignoring Line:" << std::endl;
                std::cout << "  " << line.toStdString() << std::endl;
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
                  QString(" color") << '\n';

        QMap<QString, TagInfo::sptr> usedTags;
        for (int iBookmark = 0; iBookmark < m_BookmarkList.size(); iBookmark++)
        {
            BookmarkInfo& info = m_BookmarkList[iBookmark];
            for (QList<TagInfo::sptr>::iterator iTag = info.tags.begin(); iTag < info.tags.end(); ++iTag)
            {
              usedTags.insert((*iTag)->name, *iTag);
            }
        }

        for (QMap<QString, TagInfo::sptr>::iterator i = usedTags.begin(); i != usedTags.end(); i++)
        {
            TagInfo::sptr info = *i;
            stream << info->name.leftJustified(20) + "; " + info->color.name() << '\n';
        }

        stream << '\n';

        stream << QString("# Frequency").leftJustified(12) + "; " +
                  QString("Name").leftJustified(25) + "; " +
                  QString("Tags").leftJustified(25) + "; " +
                  QString("Autostart").rightJustified(10) + "; " +
                  QString("Modulation").leftJustified(20) + "; " +
                  QString("Filter Low").rightJustified(16) + "; " +
                  QString("Filter High").rightJustified(16) + "; " +
                  QString("Filter TW").rightJustified(16) + "; " +
                  QString("AGC On").rightJustified(10) + "; " +
                  QString("AGC target level").rightJustified(16) + "; " +
                  QString("AGC manual gain").rightJustified(16) + "; " +
                  QString("AGC max gain").rightJustified(16) + "; " +
                  QString("AGC attack").rightJustified(16) + "; " +
                  QString("AGC decay").rightJustified(16) + "; " +
                  QString("AGC hang").rightJustified(16) + "; " +
                  QString("Panning").rightJustified(16) + "; " +
                  QString("Auto panning").rightJustified(16) + "; " +
                  QString("CW offset").rightJustified(16) + "; " +
                  QString("FM max deviation").rightJustified(16) + "; " +
                  QString("FM deemphasis").rightJustified(16) + "; " +
                  QString("AM DCR").rightJustified(16) + "; " +
                  QString("AM SYNC DCR").rightJustified(16) + "; " +
                  QString("AM SYNC PLL BW").rightJustified(16) + "; " +
                  QString("NB1 ON").rightJustified(16) + "; " +
                  QString("NB1 threshold").rightJustified(16) + "; " +
                  QString("NB2 ON").rightJustified(16) + "; " +
                  QString("NB2 threshold").rightJustified(16) + "; " +
                  QString("REC DIR").rightJustified(16) + "; " +
                  QString("REC SQL trig").rightJustified(16) + "; " +
                  QString("REC Min time").rightJustified(16) + "; " +
                  QString("REC Max gap").rightJustified(16)
               << "\n";
        for (int i = 0; i < m_BookmarkList.size(); i++)
        {
            BookmarkInfo& info = m_BookmarkList[i];
            QString line =
                QString::number(info.frequency).rightJustified(12) + "; " +
                info.name.leftJustified(25) + "; ";
            for (int iTag = 0; iTag < info.tags.size(); ++iTag)
            {
                TagInfo::sptr tag = info.tags[iTag];
                if (iTag!=0)
                {
                    line.append(",");
                }
                line.append(tag->name);
            }
            line.append(
                "; " + QVariant(info.get_freq_lock()).toString().rightJustified(16) +
                "; " + info.modulation.leftJustified(20)+
                "; " + QString::number(info.get_filter_low()).rightJustified(16) +
                "; " + QString::number(info.get_filter_high()).rightJustified(16) +
                "; " + QString::number(info.get_filter_tw()).rightJustified(16) +
                "; " + QVariant(info.get_agc_on()).toString().rightJustified(16) +
                "; " + QString::number(info.get_agc_target_level()).rightJustified(16) +
                "; " + QString::number(info.get_agc_manual_gain()).rightJustified(16) +
                "; " + QString::number(info.get_agc_max_gain()).rightJustified(16) +
                "; " + QString::number(info.get_agc_attack()).rightJustified(16) +
                "; " + QString::number(info.get_agc_decay()).rightJustified(16) +
                "; " + QString::number(info.get_agc_hang()).rightJustified(16) +
                "; " + QString::number(info.get_agc_panning()).rightJustified(16) +
                "; " + QVariant(info.get_agc_panning_auto()).toString().rightJustified(16) +
                "; " + QString::number(info.get_cw_offset()).rightJustified(16) +
                "; " + QString::number(info.get_fm_maxdev()).rightJustified(16) +
                "; " + QString::number(1.0e6 * info.get_fm_deemph()).rightJustified(16) +
                "; " + QVariant(info.get_am_dcr()).toString().rightJustified(16) +
                "; " + QVariant(info.get_amsync_dcr()).toString().rightJustified(16) +
                "; " + QString::number(info.get_amsync_pll_bw()).rightJustified(16) +
                "; " + QVariant(info.get_nb_on(1)).toString().rightJustified(16) +
                "; " + QString::number(info.get_nb_threshold(1)).rightJustified(16) +
                "; " + QVariant(info.get_nb_on(2)).toString().rightJustified(16) +
                "; " + QString::number(info.get_nb_threshold(2)).rightJustified(16) +
                "; " + QString::fromStdString(info.get_audio_rec_dir()).rightJustified(16) +
                "; " + QVariant(info.get_audio_rec_sql_triggered()).toString().rightJustified(16) +
                "; " + QString::number(info.get_audio_rec_min_time()).rightJustified(16) +
                "; " + QString::number(info.get_audio_rec_max_gap()).rightJustified(16));
            stream << line << "\n";
        }

        emit BookmarksChanged();
        file.close();
        return true;
    }
    return false;
}

QList<BookmarkInfo> Bookmarks::getBookmarksInRange(qint64 low, qint64 high, bool autoAdded)
{
    BookmarkInfo info;
    info.frequency = low;
    QList<BookmarkInfo>::const_iterator lb = std::lower_bound(m_BookmarkList.begin(), m_BookmarkList.end(), info);
    info.frequency=high;
    QList<BookmarkInfo>::const_iterator ub = std::upper_bound(m_BookmarkList.begin(), m_BookmarkList.end(), info);

    QList<BookmarkInfo> found;

    while (lb != ub)
    {
        const BookmarkInfo& info = *lb;
        if (!autoAdded || lb->get_freq_lock())
        {
          found.append(info);
        }
        lb++;
    }

    return found;

}

int Bookmarks::find(const BookmarkInfo &info)
{
    return m_BookmarkList.indexOf(info);
}

TagInfo::sptr Bookmarks::findOrAddTag(QString tagName)
{
    tagName = tagName.trimmed();

    if (tagName.isEmpty())
        tagName=TagInfo::strUntagged;

    int idx = getTagIndex(tagName);

    if (idx != -1)
        return m_TagList[idx];

    TagInfo::sptr info = TagInfo::make(tagName);
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
    TagInfo::sptr pTagToDelete = m_TagList[idx];
    for(int i=0; i < m_BookmarkList.size(); ++i)
    {
        BookmarkInfo& bmi = m_BookmarkList[i];
        for(int t=0; t<bmi.tags.size(); ++t)
        {
            TagInfo::sptr pTag = bmi.tags[t];
            if(pTag.get() == pTagToDelete.get())
            {
                if(bmi.tags.size()>1) bmi.tags.removeAt(t);
                else bmi.tags[0] = findOrAddTag(TagInfo::strUntagged);
            }
        }
    }

    // Delete Tag.
    m_TagList.removeAt(idx);

    emit TagListChanged();
    save();

    return true;
}

bool Bookmarks::setTagChecked(QString tagName, bool bChecked)
{
    int idx = getTagIndex(tagName);
    if (idx == -1) return false;
    m_TagList[idx]->active = bChecked;
    emit BookmarksChanged();
    emit TagListChanged();
    return true;
}

int Bookmarks::getTagIndex(QString tagName)
{
    tagName = tagName.trimmed();
    for (int i = 0; i < m_TagList.size(); i++)
    {
        if (m_TagList[i]->name == tagName)
            return i;
    }

    return -1;
}

const QColor BookmarkInfo::GetColor() const
{
    for(int iTag=0; iTag<tags.size(); ++iTag)
    {
        TagInfo::sptr tag = tags[iTag];
        if(tag->active)
        {
            return tag->color;
        }
    }
    return TagInfo::DefaultColor;
}

bool BookmarkInfo::IsActive() const
{
    bool bActive = false;
    for(int iTag=0; iTag<tags.size(); ++iTag)
    {
        TagInfo::sptr tag = tags[iTag];
        if(tag->active)
        {
            bActive = true;
            break;
        }
    }
    return bActive;
}
