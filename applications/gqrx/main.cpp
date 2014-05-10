/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           http://gqrx.dk/
 *
 * Copyright 2011-2013 Alexandru Csete OZ9AEC.
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
#include <QApplication>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QString>
#include <QStringList>
#include <QtGlobal>

#include "mainwindow.h"
#include "gqrx.h"

#include <iostream>
#include <boost/program_options.hpp>
namespace po = boost::program_options;

static void reset_conf(const QString &file_name);
static void list_conf(void);

int main(int argc, char *argv[])
{
    QString cfg_file;
    std::string conf;
    bool clierr=false;
    bool edit_conf = false;

    QApplication a(argc, argv);
    QCoreApplication::setOrganizationName(GQRX_ORG_NAME);
    QCoreApplication::setOrganizationDomain(GQRX_ORG_DOMAIN);
    QCoreApplication::setApplicationName(GQRX_APP_NAME);
    QCoreApplication::setApplicationVersion(VERSION);

    // setup controlport via environment variables
    // see http://lists.gnu.org/archive/html/discuss-gnuradio/2013-05/msg00270.html
    // Note: tried using gr::prefs().save() but that doesn't have effect until the next time
    if (qputenv("GR_CONF_CONTROLPORT_ON", "False"))
        qDebug() << "Controlport disabled";
    else
        qDebug() << "Failed to disable controlport";

#if 0
//#ifdef WITH_PORTAUDIO
    // FIXME: This should be user configurable although for now
    // the audio-osx-source is useless and the only way to use the
    // Funcube Dongle Pro and Pro+ is via portaudio.
    if (qputenv("GR_CONF_AUDIO_AUDIO_MODULE", "portaudio"))
        qDebug() << "GR_CONF_AUDIO_AUDIO_MODULE set to portaudio";
    else
        qDebug() << "Failed to set GR_CONF_AUDIO_AUDIO_MODULE=portaudio";
#endif

    // setup the program options
    po::options_description desc("Command line options");
    desc.add_options()
        ("help,h", "This help message")
        ("list,l", "List existing configurations")
        ("conf,c", po::value<std::string>(&conf), "Start with this config file")
        ("edit,e", "Edit the config file before using it")
        ("reset,r", "Reset configuration file")
    ;

    po::variables_map vm;
    try
    {
        po::store(po::parse_command_line(argc, argv, desc), vm);
    }
    catch(const boost::program_options::invalid_command_line_syntax& ex)
    {
        /* happens if e.g. -c without file name */
        clierr = true;
    }
    catch(const boost::program_options::unknown_option& ex)
    {
        /* happens if e.g. -c without file name */
        clierr = true;
    }

    po::notify(vm);

    // print the help message
    if (vm.count("help") || clierr)
    {
        std::cout << "Gqrx software defined radio receiver " << VERSION << std::endl;
        std::cout << desc << std::endl;
        return 1;
    }

    if (vm.count("list"))
    {
        list_conf();
        return 0;
    }

    if (!conf.empty())
    {
        cfg_file = QString::fromStdString(conf);
        qDebug() << "User specified config file:" << cfg_file;
    }
    else
    {
        cfg_file = "default.conf";
        qDebug() << "No user supplied config file. Using" << cfg_file;
    }

    if (vm.count("reset"))
        reset_conf(cfg_file);
    else if (vm.count("edit"))
        edit_conf = true;

    // Mainwindow will check whether we have a configuration
    // and open the config dialog if there is none or the specified
    // file does not exist.
    MainWindow w(cfg_file, edit_conf);

    if (w.configOk)
    {
        w.show();
        return a.exec();
    }
    else
    {
        return 1;
    }
}

/*! \brief Reset configuration file specified by file_name. */
static void reset_conf(const QString &file_name)
{
    QString cfg_file;
    QByteArray xdg_dir = qgetenv("XDG_CONFIG_HOME");

    if (xdg_dir.isEmpty())
        cfg_file = QString("%1/.config/gqrx/%2").arg(QDir::homePath()).arg(file_name);
    else
        cfg_file = QString("%1/gqrx/%2").arg(xdg_dir.data()).arg(file_name);

    if (QFile::exists(cfg_file))
    {
        if (QFile::remove(cfg_file))
            qDebug() << cfg_file << "deleted";
        else
            qDebug() << "Failed to remove" << cfg_file;
    }
    else
    {
        qDebug() << "Can not delete" << cfg_file << "- file does not exist!";
    }
}

/*! \brief List avaialble configurations. */
static void list_conf(void)
{
    QString conf_path;
    QByteArray xdg_dir = qgetenv("XDG_CONFIG_HOME");

    if (xdg_dir.isEmpty())
        conf_path = QString("%1/.config/gqrx/").arg(QDir::homePath());
    else
        conf_path = QString("%1/gqrx/").arg(xdg_dir.data());

    QDir conf_dir = QDir(conf_path, "*.conf", QDir::Name, QDir::Files);
    QStringList conf_files = conf_dir.entryList(QStringList("*.conf"));

    std::cout << " Existing configuration files:" << std::endl;

    if (conf_files.isEmpty())
    {
        std::cout << "     *** NONE ***" << std::endl;
    }
    else
    {
        for (int i = 0; i < conf_files.count(); i++)
        {
            std::cout << "     ";
            std::cout << conf_files.at(i).toLocal8Bit().constData() << std::endl;
        }
    }
}
