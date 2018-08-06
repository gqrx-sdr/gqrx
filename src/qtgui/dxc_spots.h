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
#ifndef DXC_SPOTS_H
#define DXC_SPOTS_H

#include <QtGlobal>
#include <QObject>
#include <QString>
#include <QMap>
#include <QList>
#include <QStringList>
#include <QColor>
#include <QTime>

struct DXCSpotInfo
{
    qint64  frequency;
    QString name;
    QTime time;
    QColor color;

    DXCSpotInfo()
    {
        this->frequency = 0;
        this->time = QTime::currentTime();
        this->color=(Qt::lightGray);
    }

    bool operator<(const DXCSpotInfo &other) const
    {
        return frequency < other.frequency;
    }

    const QColor GetColor() const;
};

class DXCSpots : public QObject
{
    Q_OBJECT
public:
    // This is a Singleton Class now because you can not send qt-signals from static functions.
    static void create();
    static DXCSpots& Get();

    void add(DXCSpotInfo& info);
    void checkSpotTimeout();
    void remove(int index);
    void setSpotTimeout(int i) {m_DXCSpotTimeout = i * 60;}
    DXCSpotInfo& getDXCSpot(int i) { return m_DXCSpotList[i]; }
    QList<DXCSpotInfo> getDXCSpotsInRange(qint64 low, qint64 high);
    //int lowerBound(qint64 low);
    //int upperBound(qint64 high);

private:
    DXCSpots(); // Singleton Constructor is private.
    QList<DXCSpotInfo> m_DXCSpotList;
    int m_DXCSpotTimeout;
    static DXCSpots* m_pThis;

signals:
    void DXCSpotsChanged(void);
};

#endif // DXC_SPOTS_H
