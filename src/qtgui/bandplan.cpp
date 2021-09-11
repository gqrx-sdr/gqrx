/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2020 Dallas Epperson.
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
#include <QDebug>
#include <QFile>
#include <QResource>
#include <QStringList>
#include <QTextStream>
#include <QString>
#include <QSet>
#include <algorithm>

#include "bandplan.h"

BandPlan* BandPlan::m_pThis = 0;

BandPlan::BandPlan()
{

}

void BandPlan::create()
{
    m_pThis = new BandPlan;
}

BandPlan& BandPlan::Get()
{
    return *m_pThis;
}

void BandPlan::setConfigDir(const QString& cfg_dir)
{
    m_bandPlanFile = cfg_dir + "/bandplan.csv";
    qInfo() << "BandPlan: File is " << m_bandPlanFile;

    if (!QFile::exists(m_bandPlanFile))
    {
        QResource resource(":/textfiles/bandplan.csv");
        QFile::copy(resource.absoluteFilePath(), m_bandPlanFile);
        QFile::setPermissions(m_bandPlanFile, QFile::permissions(m_bandPlanFile) | QFile::WriteOwner);
    }
}

bool BandPlan::load()
{
    QFile file(m_bandPlanFile);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return false;

    m_BandInfoList.clear();

    while (!file.atEnd())
    {
        QString line = QString::fromUtf8(file.readLine().trimmed());

        QStringList strings = line.split(",");
        if (line.isEmpty() || line.startsWith("#") || line.startsWith(",,") || strings.count() < 9) {
            // qInfo() << "BandPlan: Ignoring Line:" << line;
            continue;
        }

        BandInfo info;
        info.minFrequency = strings[0].toLongLong();
        info.maxFrequency = strings[1].toLongLong();
        info.modulation   = strings[2].trimmed();
        info.step         = strings[3].toInt();
        info.color        = QColor(strings[4].trimmed());
        info.name         = strings[5].trimmed();
        info.region       = strings[6].trimmed();
        info.country      = strings[7].trimmed();
        info.use          = strings[8].trimmed();

        m_BandInfoList.append(info);

        m_filterValues.countries.insert(info.country);
        m_filterValues.modulations.insert(info.modulation);
        m_filterValues.regions.insert(info.region);
        m_filterValues.uses.insert(info.use);
    }
    file.close();

    emit BandPlanChanged();
    return true;
}

QList<BandInfo> BandPlan::getBandsInRange(const BandInfoFilter &filter, qint64 low, qint64 high)
{
    QList<BandInfo> found;
    for (int i = 0; i < m_BandInfoList.size(); i++) {
        if (!filter.matches(m_BandInfoList[i])) continue;
        if (m_BandInfoList[i].maxFrequency < low) continue;
        if (m_BandInfoList[i].minFrequency > high) continue;
        found.append(m_BandInfoList[i]);
    }
    return found;
}

QList<BandInfo> BandPlan::getBandsEncompassing(const BandInfoFilter &filter, qint64 freq)
{
    QList<BandInfo> found;
    for (int i = 0; i < m_BandInfoList.size(); i++) {
        if (!filter.matches(m_BandInfoList[i])) continue;
        if (m_BandInfoList[i].maxFrequency < freq) continue;
        if (m_BandInfoList[i].minFrequency > freq) continue;
        found.append(m_BandInfoList[i]);
    }
    return found;
}
