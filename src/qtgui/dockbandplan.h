/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2011-2016 Alexandru Csete OZ9AEC.
 *                2021 Doug Hammond
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
#ifndef DOCKBANDPLAN_H
#define DOCKBANDPLAN_H

#include <QDockWidget>
#include <QListWidget>
#include <QSettings>
#include <QTableWidgetItem>

#include "bandplan.h"

namespace Ui {
    class DockBandplan;
}

/*! \brief Dock widget with Band Plan Settings. */
class DockBandplan : public QDockWidget
{
    Q_OBJECT

public:
    explicit DockBandplan(QWidget *parent = 0);
    ~DockBandplan();

    void saveSettings(QSettings *settings);
    void readSettings(QSettings *settings);

    void setupBandplanFilterValues();
    void setupBandplanFilterSelections();

signals:
    void bandPlanChanged(bool state, const BandInfoFilter &filter);
    void bandPlanItemSelected(qint64 frequency, QString modulation);

private:
    QSet<QString> updateSelection(const QListWidget *list) const;

private slots:
    void on_bandPlanCheckboxEnable_stateChanged(int state);
    void on_bandPlanRegionsList_itemSelectionChanged();
    void on_bandPlanCountriesList_itemSelectionChanged();
    void on_bandPlanModulationsList_itemSelectionChanged();
    void on_bandPlanUsesList_itemSelectionChanged();
    void on_visibleItemList_itemDoubleClicked(QTableWidgetItem *item);

    void on_bandPlanChanged(bool state, const BandInfoFilter &filter);

private:
    Ui::DockBandplan *ui;

    bool           m_bandplanLoaded;
    bool           m_bandplanEnabled;
    BandInfoFilter m_bandplanFilter;
};

#endif // DOCKBANDPLAN_H
