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

#include "dockbandplan.h"
#include "ui_dockbandplan.h"

DockBandplan::DockBandplan(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::DockBandplan),
    m_bandplanLoaded(false)
{
    ui->setupUi(this);

    setupBandplanFilterValues();
}

DockBandplan::~DockBandplan()
{
    delete ui;
}

void DockBandplan::saveSettings(QSettings *settings)
{
    if (!settings)
        return;

    settings->beginGroup("bandplan");

    settings->setValue("enabled", ui->bandPlanCheckboxEnable->isChecked());

    QStringList bpRegions = m_bandplanFilter.regions.values();
    settings->setValue("regions", bpRegions.join("|"));

    QStringList bpCountries = m_bandplanFilter.countries.values();
    settings->setValue("countries", bpCountries.join("|"));

    QStringList bpModulations = m_bandplanFilter.modulations.values();
    settings->setValue("modulations", bpModulations.join("|"));

    QStringList bpUses = m_bandplanFilter.uses.values();
    settings->setValue("uses", bpUses.join("|"));

    settings->endGroup();
}

QSet<QString> loadFilterSelections(QString concatValues, QSet<QString> defaults)
{
    QStringList list;
    if (concatValues.length() > 0) {
        list = concatValues.split("|");
    } else {
        list = defaults.values();
    }
    QSet<QString> next(list.toSet());
    return next;
}

void DockBandplan::readSettings(QSettings *settings)
{
    if (!settings)
        return;

    settings->beginGroup("bandplan");

    const bool enabled = settings->value("enabled", false).toBool();
    ui->bandPlanCheckboxEnable->setChecked(enabled);

    const BandInfoFilter defaultValues = BandPlan::Get().getFilterValues();
    m_bandplanFilter.regions = loadFilterSelections(settings->value("regions").toString(), defaultValues.regions);
    m_bandplanFilter.countries = loadFilterSelections(settings->value("countries").toString(), defaultValues.countries);
    m_bandplanFilter.modulations = loadFilterSelections(settings->value("modulations").toString(), defaultValues.modulations);
    m_bandplanFilter.uses = loadFilterSelections(settings->value("uses").toString(), defaultValues.uses);

    settings->endGroup();

    setupBandplanFilterSelections();
}

void DockBandplan::setupBandplanFilterValues()
{
    const BandInfoFilter bandplanValues = BandPlan::Get().getFilterValues();

    ui->bandPlanModulationsList->addItems(bandplanValues.modulations.values());
    ui->bandPlanRegionsList->addItems(bandplanValues.regions.values());
    ui->bandPlanCountriesList->addItems(bandplanValues.countries.values());
    ui->bandPlanUsesList->addItems(bandplanValues.uses.values());
}

void setWidgetSelections(QListWidget *list, const QSet<QString> &selections)
{
    for (int i = 0; i < list->count(); ++i) {
        auto item = list->item(i);
        const auto isSelected = selections.contains(item->text());
        item->setSelected(isSelected);
    }
}

void DockBandplan::setupBandplanFilterSelections()
{
    setWidgetSelections(ui->bandPlanRegionsList, m_bandplanFilter.regions);
    setWidgetSelections(ui->bandPlanCountriesList, m_bandplanFilter.countries);
    setWidgetSelections(ui->bandPlanModulationsList, m_bandplanFilter.modulations);
    setWidgetSelections(ui->bandPlanUsesList, m_bandplanFilter.uses);
    m_bandplanLoaded = true;
    emit bandPlanChanged(m_bandplanEnabled, m_bandplanFilter);
}


void DockBandplan::on_bandPlanCheckboxEnable_stateChanged(int state)
{
    m_bandplanEnabled = state == 2;
    emit bandPlanChanged(m_bandplanEnabled, m_bandplanFilter);
}

QSet<QString> updateSelection(const QListWidget *list)
{
    const auto selected = list->selectedItems();
    QSet<QString> next;
    for (auto it = selected.cbegin(); it != selected.cend(); ++it) {
        const auto val = *it;
        next.insert(val->text());
    }
    return next;
}

void DockBandplan::on_bandPlanRegionsList_itemSelectionChanged()
{
    if (!m_bandplanLoaded) return;
    m_bandplanFilter.regions = updateSelection(ui->bandPlanRegionsList);
    emit bandPlanChanged(m_bandplanEnabled, m_bandplanFilter);
}

void DockBandplan::on_bandPlanCountriesList_itemSelectionChanged()
{
    if (!m_bandplanLoaded) return;
    m_bandplanFilter.countries = updateSelection(ui->bandPlanCountriesList);
    emit bandPlanChanged(m_bandplanEnabled, m_bandplanFilter);
}

void DockBandplan::on_bandPlanModulationsList_itemSelectionChanged()
{
    if (!m_bandplanLoaded) return;
    m_bandplanFilter.modulations = updateSelection(ui->bandPlanModulationsList);
    emit bandPlanChanged(m_bandplanEnabled, m_bandplanFilter);
}

void DockBandplan::on_bandPlanUsesList_itemSelectionChanged()
{
    if (!m_bandplanLoaded) return;
    m_bandplanFilter.uses = updateSelection(ui->bandPlanUsesList);
    emit bandPlanChanged(m_bandplanEnabled, m_bandplanFilter);
}
