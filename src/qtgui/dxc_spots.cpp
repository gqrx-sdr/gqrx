/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           http://gqrx.dk/
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
#include "dxc_spots.h"
#include <stdio.h>
#include <wchar.h>

DXCSpots* DXCSpots::m_pThis = 0;

DXCSpots::DXCSpots()
{
}

void DXCSpots::create()
{
    m_pThis = new DXCSpots;
}

DXCSpots& DXCSpots::Get()
{
    return *m_pThis;
}

void DXCSpots::add(DXCSpotInfo &info)
{
    info.time = QTime::currentTime();
    m_DXCSpotList.append(info);
    std::stable_sort(m_DXCSpotList.begin(),m_DXCSpotList.end());
    emit( DXCSpotsChanged() );
}

void DXCSpots::checkSpotTimeout()
{
    for (int i = 0; i < m_DXCSpotList.size(); i++)
    {
        if ( m_DXCSpotTimeout < m_DXCSpotList[i].time.secsTo(QTime::currentTime() ))
        {
            m_DXCSpotList.removeAt(i);
        }
    }
    std::stable_sort(m_DXCSpotList.begin(),m_DXCSpotList.end());
    emit( DXCSpotsChanged() );
}

void DXCSpots::remove(int index)
{
    m_DXCSpotList.removeAt(index);
    emit( DXCSpotsChanged() );
}

QList<DXCSpotInfo> DXCSpots::getDXCSpotsInRange(qint64 low, qint64 high)
{
    DXCSpotInfo info;
    info.frequency=low;
    QList<DXCSpotInfo>::const_iterator lb = qLowerBound(m_DXCSpotList, info);
    info.frequency=high;
    QList<DXCSpotInfo>::const_iterator ub = qUpperBound(m_DXCSpotList, info);

    QList<DXCSpotInfo> found;

    while (lb != ub)
    {
        const DXCSpotInfo& info = *lb;
        //if(info.IsActive())
        {
          found.append(info);
        }
        lb++;
    }

    return found;

}

const QColor DXCSpotInfo::GetColor() const
{
    return DXCSpotInfo::color;
}

