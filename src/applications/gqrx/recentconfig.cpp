/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           http://gqrx.dk/
 *
 * Copyright 2020 Markus Kolb.
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

#include <QDebug>
#include <QDir>
#include <QTextStream>

#include "recentconfig.h"

#define MENU_FILENAME_LEN 15
// MENU_MAX_ENTRIES is limited by action shortcut numbering
#define MENU_MAX_ENTRIES 9

RecentConfig::RecentConfig(const QString &configDir, QMenu *menu) : QObject()
{
    this->menu = menu;
    cfgfiles = QSharedPointer<QVector<QFileInfo>>(new QVector<QFileInfo>());

    configFile = QFileInfo(QDir(configDir), RECENT_CONFIG_FILENAME);

#ifndef QT_NO_DEBUG_OUTPUT
    if (loadRecentConfig())
        qDebug() << "Read file " << configFile.canonicalFilePath();
    else if (configFile.canonicalFilePath().isEmpty())
        qDebug() << "No file " << configFile.absoluteFilePath();
    else
        qDebug() << "Read failed of file " << configFile.absoluteFilePath();
#else
    loadRecentConfig();
#endif

    connect(this, &RecentConfig::configLoaded, this, &RecentConfig::onConfigLoaded);
    connect(this, &RecentConfig::configSaved, this, &RecentConfig::onConfigSaved);
}

RecentConfig::~RecentConfig()
{
#ifndef QT_NO_DEBUG_OUTPUT
    if (saveRecentConfig())
        qDebug() << "Written file " << configFile.absoluteFilePath();
    else
        qDebug() << "Write failed to file " << configFile.absoluteFilePath();
#else
    saveRecentConfig();
#endif
}

void RecentConfig::createMenuActions()
{
    menu->clear();
    for (int i = 0; i < cfgfiles->count(); i++) {
        const QFileInfo *file = &cfgfiles->at(i);
        if (!file->exists())
        {
            cfgfiles->remove(i--);
            continue;
        }

        QString fname(file->fileName());

        QString nr = (i < 9 ? " &" : "&") + QString::number(i + 1);

        // QAction deleted by clear above
        QAction *action = new QAction(nr + " " + (fname.length() > MENU_FILENAME_LEN ?
                                                      fname.left(MENU_FILENAME_LEN) + "..." : fname),
                                      menu);
        action->setStatusTip("Load settings from config file " + file->canonicalFilePath());
        action->setData(i);
        connect(action, &QAction::triggered, this, [=](bool) { onMenuAction(i); });
        menu->addAction(action);
    }
}

QVector<QFileInfo>::const_iterator RecentConfig::getConfigFiles() const
{
    return cfgfiles->cbegin();
}

QMenu *RecentConfig::getMenu()
{
    return menu;
}

bool RecentConfig::loadRecentConfig()
{
    QFile file(configFile.canonicalFilePath());

    if (!file.open(QFile::ReadOnly))
        return false;

    QTextStream s(&file);

    while (!s.atEnd())
    {
        QString buf;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 5, 0))
        if (!s.readLineInto(&buf, 33000))
        {
            file.close();
            cfgfiles->clear();
            return false;
        }
#else
        buf = s.readLine(33000);
        if (buf.isNull())
        {
            file.close();
            cfgfiles->clear();
            return false;
        }
#endif
        if (!buf.isEmpty())
        {
            const auto fi = QFileInfo(buf);
            if (fi.exists())
                cfgfiles->append(fi);
        }
    }

    file.close();
    return true;
}

bool RecentConfig::saveRecentConfig()
{
    if (cfgfiles->count() == 0)
    {
        return false;
    }

    QFile file(configFile.absoluteFilePath());

    if (!file.open(QFile::WriteOnly))
        return false;

    QTextStream s(&file);

    for (auto it = cfgfiles->cbegin(); it != cfgfiles->cend(); it++)
    {
        s << it->canonicalFilePath() << "\n";
    }

    file.close();
    return true;
}

void RecentConfig::onConfigSaved(const QString &filename)
{
    updateFiles(filename);
    createMenuActions();
}

void RecentConfig::onConfigLoaded(const QString &filename)
{
    updateFiles(filename);
    createMenuActions();
}

inline void RecentConfig::updateFiles(const QString &filename)
{
    QFileInfo file(filename);
    if (!file.exists())
        return;

#if (QT_VERSION >= QT_VERSION_CHECK(5, 4, 0))
    cfgfiles->removeOne(file);
#else
    const int i = cfgfiles->indexOf(file);
    if (i >= 0)
    {
        cfgfiles->removeAt(i);
    }
#endif
    cfgfiles->prepend(file);
    if (cfgfiles->count() > MENU_MAX_ENTRIES)
    {
        cfgfiles->removeLast();
    }
}

void RecentConfig::onMenuAction(int index) const
{
    if (cfgfiles->count() > index)
    {
        const auto fname = cfgfiles->at(index).canonicalFilePath();
        emit loadConfig(fname);
#ifndef QT_NO_DEBUG_OUTPUT
        qDebug() << "Emitted loadConfig(" << fname << ")";
#endif
    }
}
