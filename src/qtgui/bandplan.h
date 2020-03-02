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
#ifndef BANDPLAN_H
#define BANDPLAN_H

#include <QtGlobal>
#include <QObject>
#include <QString>
#include <QMap>
#include <QList>
#include <QStringList>
#include <QColor>



struct BandInfo
{
    qint64  minFrequency;
    qint64  maxFrequency;
    QString name;
    QString modulation;
    qint64  step;
    QColor  color;

    BandInfo()
    {
        this->minFrequency = 0;
        this->maxFrequency = 0;
        this->step = 1;
    }

    bool operator<(const BandInfo &other) const
    {
        return minFrequency < other.minFrequency;
    }
};

class BandPlan : public QObject
{
    Q_OBJECT
public:
    // This is a Singleton Class now because you can not send qt-signals from static functions.
    static void create();
    static BandPlan& Get();
    bool load();
    int size() { return m_BandInfoList.size(); }
    BandInfo& getBand(int i) { return m_BandInfoList[i]; }
    QList<BandInfo> getBandsInRange(qint64 low, qint64 high);
    QList<BandInfo> getBandsEncompassing(qint64 freq);

    void setConfigDir(const QString&);

private:
    BandPlan(); // Singleton Constructor is private.
    QList<BandInfo>  m_BandInfoList;
    QString          m_bandPlanFile;
    static BandPlan* m_pThis;

signals:
    void BandPlanChanged(void);
};

#endif // BANDPLAN_H
