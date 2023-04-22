/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
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
#include <QCommandLineParser>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QString>
#include <QStringList>
#include <QtGlobal>

#ifdef WITH_PORTAUDIO
#include <portaudio.h>
#endif
#ifdef WITH_PULSEAUDIO
#include <pulse/error.h>
#include <pulse/simple.h>
#endif

#include "mainwindow.h"
#include "gqrx.h"
#include "../../../lib/discord_game_sdk.h"
#include <iostream>

static void reset_conf(const QString &file_name);
static void list_conf();

//DS Strings
char freqSTRS[127]; //Freq string
char typeSTRS[127]; //Type String (NFM/WFM/AM/Another...)
char rdsSTRS[127]; //RDS Prototype
char SDRIcon[127]; //SDR Large Icon

int recivOn = 0;

//DS Callbacks/Funcs:
struct ApplicationDS {
    struct IDiscordCore* core;
    struct IDiscordUserManager* users;
    struct IDiscordAchievementManager* achievements;
    struct IDiscordActivityManager* activities;
    struct IDiscordRelationshipManager* relationships;
    struct IDiscordApplicationManager* application;
    struct IDiscordLobbyManager* lobbies;
    DiscordUserId user_id;
};

void dscalback(){

}

bool whileavalible = false; //While Available?

void myCallback(void *data, enum EDiscordResult result) {
    if(result == 0)
        whileavalible = true;
    else
        whileavalible = false;
}

void dscall(ApplicationDS a, int suc){
    //Check on succes
    if(suc != 0) return;

    printf("[DS Rich Presence] Callback Thread Init...\n");
    void (*func_ptr)() = dscalback;
    void *ptr = (void *) func_ptr;

    //Set Activity
    struct DiscordActivity ac;
    memset(&ac, 0, sizeof(ac));
    sprintf(ac.details, "Прослушивает радио-частоту:");
    sprintf(ac.assets.small_image, "gqrx");
    sprintf(ac.assets.large_image, "%s", SDRIcon);

    printf("[DS Rich Presence] Callback Thread Init Success!\n");

    //Calback While
    while(true){

        //check with while avalible
        if(!whileavalible) {
            printf("[DS Rich Presence] Callback Thread Available fail!\n");
            break;
        }

        //Set RPC details
        if (recivOn == 0) sprintf(ac.details, "Ресивер выключен(Не слушает радио)");
        else if (strcmp(typeSTRS, "OFF") == 0) sprintf(ac.details, "DEMOD Отключен(Не слушает радио)");
        else if (strcmp(typeSTRS, "RAW") == 0) sprintf(ac.details, "Прослушивает Радио в RAW - Режиме.");
        else if (strcmp(typeSTRS, "WFM_S") == 0) sprintf(ac.details, "Прослушивает Радио в FM(Stereo) - Режиме.");
        else if (strcmp(typeSTRS, "NFM") == 0) sprintf(ac.details, "Прослушивает Радио в NarrowFM - Режиме.");
        else if (strcmp(typeSTRS, "WFM") == 0) sprintf(ac.details, "Прослушивает Радио в FM - Режиме.");
        else if (strcmp(typeSTRS, "AMSYNC") == 0) sprintf(ac.details, "Прослушивает Радио в AMSync - Режиме.");
        else if (strcmp(typeSTRS, "AM") == 0) sprintf(ac.details, "Прослушивает Радио в AM - Режиме.");
        else sprintf(ac.details, "Прослушивает радио-частоту:");

        //Set RPC state (freq MHz)
        sprintf(ac.state, "%s", freqSTRS);

        //Update Activity
        a.activities->update_activity(a.core->get_activity_manager(a.core), &ac, ptr, myCallback);

        //Run callback
        EDiscordResult r = a.core->run_callbacks(a.core);

        //Check Sucess?
        if(r != 0) {
            printf("[DS Rich Presence] Callback Thread Available fail!\n");
            whileavalible = false;
            break;
        }

        //Sleep thread
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}
//END.DS

int main(int argc, char *argv[]) {
    //Init DS Rich Presence
    printf("[DS Rich Presence] Init...\n");

    //Set SDR Icon - blue-sdr or white-sdr or gold-sdr
    sprintf(SDRIcon, "%s", "blue-sdr");

    //Init globals DS-RP vars
    sprintf(freqSTRS, "100.000MHz");
    sprintf(typeSTRS, "NONE");

    struct ApplicationDS sapp;
    memset(&sapp, 0, sizeof(sapp));

    IDiscordCoreEvents events;
    memset(&events, 0, sizeof(events));
    struct DiscordCreateParams params;
    params.client_id = 1099004654978879570;
    params.flags = DiscordCreateFlags_NoRequireDiscord;
    params.events = &events;
    params.event_data = &sapp;

    //Create Discord
    EDiscordResult a = DiscordCreate(DISCORD_VERSION, &params, &sapp.core);


    //Check success?
    if(a == 0) {
        whileavalible = true;
        sapp.activities = sapp.core->get_activity_manager(sapp.core);
        sapp.application = sapp.core->get_application_manager(sapp.core);
        sapp.users = sapp.core->get_user_manager(sapp.core);
        printf("[DS Rich Presence] Init success!\n");
    }else{
        whileavalible = false;
        printf("[DS Rich Presence] Init fail!\n");
    }

    //Start Callback thread
    std::thread t(dscall, sapp, a);


    //END DS INIT

    QString         cfg_file;
    bool            edit_conf = false;
    int             return_code = 0;

    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName(GQRX_ORG_NAME);
    QCoreApplication::setOrganizationDomain(GQRX_ORG_DOMAIN);
    QCoreApplication::setApplicationName(GQRX_APP_NAME);
    QCoreApplication::setApplicationVersion(VERSION);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps, true);
#endif
    QLoggingCategory::setFilterRules("*.debug=false");

    QString plugin_path = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../soapy-modules");
    QFileInfo plugin_path_info(plugin_path);
    if (plugin_path_info.isDir())
    {
        qputenv("SOAPY_SDR_PLUGIN_PATH", plugin_path.toUtf8());
        qputenv("SOAPY_SDR_ROOT", "/invalid");
    }

    // setup controlport via environment variables
    // see http://lists.gnu.org/archive/html/discuss-gnuradio/2013-05/msg00270.html
    // Note: tried using gr::prefs().save() but that doesn't have effect until the next time
    if (qputenv("GR_CONF_CONTROLPORT_ON", "False"))
        qDebug() << "Controlport disabled";
    else
        qDebug() << "Failed to disable controlport";

    QCommandLineParser parser;
    parser.setApplicationDescription("Gqrx software defined radio receiver " VERSION);
    parser.addHelpOption();
    parser.addOptions({
        {{"s", "style"}, "Use the given style (fusion, windows)", "style"},
        {{"l", "list"}, "List existing configurations"},
        {{"c", "conf"}, "Start with this config file", "file"},
        {{"e", "edit"}, "Edit the config file before using it"},
        {{"r", "reset"}, "Reset configuration file"},
    });
    parser.process(app);

    if (parser.isSet("style"))
        QApplication::setStyle(parser.value("style"));

    if (parser.isSet("list"))
    {
        list_conf();
        return 0;
    }

    // check whether audio backend is functional
#ifdef WITH_PORTAUDIO
    PaError     err = Pa_Initialize();
    if (err != paNoError)
    {
        QString message = QString("Portaudio error: %1").arg(Pa_GetErrorText(err));
        qCritical() << message;
        QMessageBox::critical(nullptr, "Audio Error", message,
                              QMessageBox::Abort, QMessageBox::NoButton);
        return 1;
    }
#endif

#ifdef WITH_PULSEAUDIO
    int         error = 0;
    pa_simple  *test_sink;
    pa_sample_spec ss;

    ss.format = PA_SAMPLE_FLOAT32LE;
    ss.rate = 48000;
    ss.channels = 2;
    test_sink =  pa_simple_new(NULL, "Gqrx Test", PA_STREAM_PLAYBACK, NULL,
                               "Test stream", &ss, NULL, NULL, &error);
    if (!test_sink)
    {
        QString message = QString("Pulseaudio error: %1").arg(pa_strerror(error));
        qCritical() << message;
        QMessageBox::critical(0, "Audio Error", message,
                              QMessageBox::Abort, QMessageBox::NoButton);
        return 1;
    }
    pa_simple_free(test_sink);
#endif


    if (parser.isSet("conf"))
    {
        cfg_file = parser.value("conf");
        qDebug() << "User specified config file:" << cfg_file;
    }
    else
    {
        cfg_file = "default.conf";
        qDebug() << "No user supplied config file. Using" << cfg_file;
    }

    if (parser.isSet("reset"))
        reset_conf(cfg_file);
    else if (parser.isSet("edit"))
        edit_conf = true;

    try {
        // Mainwindow will check whether we have a configuration
        // and open the config dialog if there is none or the specified
        // file does not exist.
        MainWindow w(cfg_file, edit_conf);

        if (w.configOk)
        {
            w.show();
            return_code = QApplication::exec();
        }
        else
        {
            return_code = 1;
        }
    }
    catch (std::exception &x)
    {
        QMessageBox::warning(nullptr,
                         QObject::tr("Crash detected"),
                         QObject::tr("<p>gqrx exited with an exception:</p>"
                                     "<p><b>%1</b></p>"
                                     "<p>More information can be found by launching it from the console.</p>")
                                 .arg(x.what()),
                         QMessageBox::Close);
    }

#ifdef WITH_PORTAUDIO
    Pa_Terminate();
#endif

    return return_code;
}

/** Reset configuration file specified by file_name. */
static void reset_conf(const QString &file_name) {
    QString cfg_file;
    QByteArray xdg_dir = qgetenv("XDG_CONFIG_HOME");

    if (xdg_dir.isEmpty())
        cfg_file = QString("%1/.config/gqrx/%2").arg(QDir::homePath()).arg(file_name);
    else
        cfg_file = QString("%1/gqrx/%2").arg(xdg_dir.data()).arg(file_name);

    if (QFile::exists(cfg_file)) {
        if (QFile::remove(cfg_file))
            qDebug() << cfg_file << "deleted";
        else
            qDebug() << "Failed to remove" << cfg_file;
    } else {
        qDebug() << "Can not delete" << cfg_file << "- file does not exist!";
    }
}

/** List available configurations. */
static void list_conf()
{
    QString     conf_path;
    QByteArray  xdg_dir = qgetenv("XDG_CONFIG_HOME");

    if (xdg_dir.isEmpty())
        conf_path = QString("%1/.config/gqrx/").arg(QDir::homePath());
    else
        conf_path = QString("%1/gqrx/").arg(xdg_dir.data());

    QDir conf_dir = QDir(conf_path, "*.conf", QDir::Name, QDir::Files);
    QStringList conf_files = conf_dir.entryList(QStringList("*.conf"));

    std::cout << std::endl
              << " Existing configuration files in "
              << conf_path.toStdString()
              << std::endl;

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
