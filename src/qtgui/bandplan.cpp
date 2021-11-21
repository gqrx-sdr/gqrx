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
#include <QMessageBox>

#include <algorithm>

#include "bandplan.h"

BandPlan* BandPlan::m_pThis = 0;

BandPlan::BandPlan()
{
    connect(this, SIGNAL(BandPlanParseError()), this, SLOT(on_BandPlanParseError()));
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
    m_cfgPath = cfg_dir;

    m_bandPlanFile = cfg_dir + "/bandplan.csv";
    qInfo() << "BandPlan: File is " << m_bandPlanFile;

    if (QFile::exists(m_bandPlanFile))
    {
        QResource v1_resource(":/textfiles/bandplan-v1.csv");
        auto v1_tmp = cfg_dir + "/v1_tmp.csv";
        QFile::copy(v1_resource.absoluteFilePath(), v1_tmp);

        QFile v1_ref_file(v1_tmp);
        if (!v1_ref_file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            qInfo() << "Cannot open v1 ref file";
            return;
        }
        auto v1_ref = v1_ref_file.readAll();
        v1_ref_file.close();

        QFile v1_usr_file(m_bandPlanFile);
        if (!v1_usr_file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            qInfo() << "Cannot open user bandplan file";
            return;
        }
        auto v1_usr = v1_usr_file.readAll();
        v1_usr_file.close();

        if (v1_ref == v1_usr) {
            qInfo() << "BandPlan: Found original v1 file, will remove for upgrade";
            QFile::remove(m_bandPlanFile);
        }

        QFile::remove(v1_tmp);
    }

    if (!QFile::exists(m_bandPlanFile))
    {
        QResource v2_resource(":/textfiles/bandplan-v2.csv");
        QFile::copy(v2_resource.absoluteFilePath(), m_bandPlanFile);
        QFile::setPermissions(m_bandPlanFile, QFile::permissions(m_bandPlanFile) | QFile::WriteOwner);
    }
}

void BandPlan::on_BandPlanParseError()
{
    QMessageBox msgBox;
    msgBox.setText(
                "The bandplan file on disk could not be completely read."
                );
    msgBox.setInformativeText(
                "Choose Restore Defaults to back up and replace your file with a new version.\n\n"
                "Press Cancel to leave your file intact.\n"
                "The bandplan will not show any items until you back up and remove your file from this location: \n\n" + m_bandPlanFile
                );
    msgBox.setStandardButtons(QMessageBox::Cancel | QMessageBox::RestoreDefaults);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    int ret = msgBox.exec();
    switch (ret) {
        case QMessageBox::Cancel:
            // Do nothing
            break;
        case QMessageBox::RestoreDefaults:
            QFile::rename(m_bandPlanFile, m_bandPlanFile + ".backup");
            setConfigDir(m_cfgPath);
            load();
            break;
        default:
            // should never be reached
            break;
    }
}

bool BandPlan::load()
{
    QFile file(m_bandPlanFile);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return false;

    m_BandInfoList.clear();

    int goodlines = 0;
    int badlines = 0;

    while (!file.atEnd())
    {
        QString line = QString::fromUtf8(file.readLine().trimmed());

        QStringList strings = line.split(",");
        if (line.isEmpty() || line.startsWith("#") || line.startsWith(",,") || strings.count() < 9) {
            // qInfo() << "BandPlan: Ignoring Line:" << line;
            badlines++;
            continue;
        }

        BandInfo info;
        bool conv_ok = false;
        info.minFrequency = strings[0].toLongLong(&conv_ok);
        if (!conv_ok) {
            badlines++;
            continue;
        }
        info.maxFrequency = strings[1].toLongLong(&conv_ok);
        if (!conv_ok) {
            badlines++;
            continue;
        }
        info.modulation = strings[2].trimmed();
        if (info.modulation.length() == 0) {
            badlines++;
            continue;
        }
        info.step = strings[3].toInt(&conv_ok);
        if (!conv_ok) {
            badlines++;
            continue;
        }
        auto col = strings[4].trimmed();
        if (col.length() == 0) {
            badlines++;
            continue;
        }
        info.color = QColor(col).toHsl();
        if (info.color.alpha() == 255) {
            info.color.setAlpha(0x90);
        }
        info.name = strings[5].trimmed();
        if (info.name.length() == 0) {
            badlines++;
            continue;
        }
        info.region = strings[6].trimmed();
        if (info.region.length() == 0) {
            badlines++;
            continue;
        }
        info.country = strings[7].trimmed();
        if (info.country.length() == 0) {
            badlines++;
            continue;
        }
        info.use = strings[8].trimmed();
        if (info.use.length() == 0) {
            badlines++;
            continue;
        }

        goodlines++;

        m_BandInfoList.append(info);

        m_filterValues.countries.insert(info.country);
        m_filterValues.modulations.insert(info.modulation);
        m_filterValues.regions.insert(info.region);
        m_filterValues.uses.insert(info.use);
    }
    file.close();

    if (badlines > goodlines || goodlines == 0)
    {
        qInfo() << "BandPlan: parse error; bad lines=" << badlines << " good lines=" << goodlines;
        emit BandPlanParseError();
    }
    else
    {
        emit BandPlanChanged();
    }

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
