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
#ifndef RECENTCONFIG_H
#define RECENTCONFIG_H

#include <QFileInfo>
#include <QMenu>
#include <QSharedPointer>

#define RECENT_CONFIG_FILENAME "recentconfig.cfg"

class RecentConfig : public QObject
{
    Q_OBJECT

public:
    RecentConfig(const QString &configDir, QMenu *menu);
    ~RecentConfig();

    /**
     * @brief Create actions in menu for recent config files
     */
    void createMenuActions();

signals:
    /**
     * @brief Connect to loadConfig and load the file
     * @param filename
     */
    void loadConfig(const QString &filename) const;

    /**
     * @brief configSaved emit starts menu update for filename on save
     * @param filename
     */
    void configSaved(const QString &filename);

    /**
     * @brief configLoaded emit starts menu update for filename on load
     * @param filename
     */
    void configLoaded(const QString &filename);

protected:
    QVector<QFileInfo>::const_iterator getConfigFiles() const;
    QMenu *getMenu();
    bool loadRecentConfig();
    bool saveRecentConfig();

protected slots:
    void onConfigSaved(const QString &filename);
    void onConfigLoaded(const QString &filename);

private:
    QSharedPointer<QVector<QFileInfo>> cfgfiles;
    QMenu *menu;
    QFileInfo configFile;
    inline void updateFiles(const QString &filename);

private slots:
    void onMenuAction(int index) const;

};

#endif // RECENTCONFIG_H
