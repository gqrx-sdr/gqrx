/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           http://gqrx.dk/
 *
 * Copyright (c) 2016 Josh Blum <josh@joshknows.com>
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

#include "applications/gqrx/receiver.h"
#include <QFileInfo>
#include <QTemporaryFile>
#include <QDataStream>
#include <iostream>

std::string receiver::get_random_file(void)
{
    static std::string path;
    if (path.empty())
    {
        path = "/dev/random";
        QFileInfo checkFile(QString::fromStdString(path));
        if (!checkFile.exists())
        {
            //static temp file persists until process end
            static QTemporaryFile temp_file;
            temp_file.open();
            path = temp_file.fileName().toStdString();
            {
                QDataStream stream(&temp_file);
                for (size_t i = 0; i < 1024*8; i++) stream << qint8(rand());
            }
            std::cout << "Created random file " << path << std::endl;
        }
    }
    return path;
}

std::string receiver::get_null_file(void)
{
    static std::string path;
    if (path.empty())
    {
        path = "/dev/null";
        QFileInfo checkFile(QString::fromStdString(path));
        if (!checkFile.exists())
        {
            //static temp file persists until process end
            static QTemporaryFile temp_file;
            temp_file.open();
            path = temp_file.fileName().toStdString();
            {
                QDataStream stream(&temp_file);
                for (size_t i = 0; i < 1024*8; i++) stream << qint8(0);
            }
            std::cout << "Created null file " << path << std::endl;
        }
    }
    return path;
}
