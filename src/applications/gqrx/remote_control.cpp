/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           http://gqrx.dk/
 *
 * Copyright 2013 Alexandru Csete OZ9AEC.
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
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <QString>
#include <QStringList>
#include "remote_control.h"

#define DEFAULT_RC_PORT            7356
#define DEFAULT_RC_ALLOWED_HOSTS   "::ffff:127.0.0.1"

RemoteControl::RemoteControl(QObject *parent) :
    QObject(parent)
{

    rc_freq = 0;
    rc_filter_offset = 0;
    bw_half = 740e3;
    rc_mode = 0;
    signal_level = -200.0;
    squelch_level = -150.0;
    audio_recorder_status = false;
    receiver_running = false;
    hamlib_compatible = false;

    rc_port = DEFAULT_RC_PORT;
    rc_allowed_hosts.append(DEFAULT_RC_ALLOWED_HOSTS);

    rc_socket = 0;

    connect(&rc_server, SIGNAL(newConnection()), this, SLOT(acceptConnection()));

}

RemoteControl::~RemoteControl()
{
    stop_server();
}

/*! \brief Start the server. */
void RemoteControl::start_server()
{
    if (!rc_server.isListening())
        rc_server.listen(QHostAddress::Any, rc_port);
}

/*! \brief Stop the server. */
void RemoteControl::stop_server()
{
    if (rc_socket != 0)
        rc_socket->close();

    if (rc_server.isListening())
        rc_server.close();

}

/*! \brief Read settings. */
void RemoteControl::readSettings(QSettings *settings)
{
    if (!settings)
        return;

    settings->beginGroup("remote_control");

    // Get port number; restart server if running
    if (settings->contains("port"))
        setPort(settings->value("port").toInt());

    // Get list of allowed hosts
    if (settings->contains("allowed_hosts"))
        setHosts(settings->value("allowed_hosts").toStringList());

    settings->endGroup();
}

void RemoteControl::saveSettings(QSettings *settings) const
{
    if (!settings)
        return;

    settings->beginGroup("remote_control");

    if (rc_server.isListening())
        settings->setValue("enabled", true);
    else
        settings->remove("enabled");

    if (rc_port != DEFAULT_RC_PORT)
        settings->setValue("port", rc_port);
    else
        settings->remove("port");

    if ((rc_allowed_hosts.count() != 1) || (rc_allowed_hosts.at(0) != DEFAULT_RC_ALLOWED_HOSTS))
        settings->setValue("allowed_hosts", rc_allowed_hosts);
    else
        settings->remove("allowed_hosts");

    settings->endGroup();
}

/*! \brief Set new network port.
 *  \param port The new network port.
 *
 * If the server is running it will be restarted.
 *
 */
void RemoteControl::setPort(int port)
{
    if (port == rc_port)
        return;

    rc_port = port;
    if (rc_server.isListening())
    {
        rc_server.close();
        rc_server.listen(QHostAddress::Any, rc_port);
    }
}

void RemoteControl::setHosts(QStringList hosts)
{
    rc_allowed_hosts = hosts;
}


/*! \brief Accept a new client connection.
 *
 * This slot is called when a client opens a new connection.
 */
void RemoteControl::acceptConnection()
{
    rc_socket = rc_server.nextPendingConnection();

    // check if host is allowed
    QString address = rc_socket->peerAddress().toString();
    if (rc_allowed_hosts.indexOf(address) == -1)
    {
        std::cout << "*** Remote connection attempt from " << address.toStdString()
                  << " (not in allowed list)" << std::endl;
        rc_socket->close();
    }
    else
    {
        connect(rc_socket, SIGNAL(readyRead()), this, SLOT(startRead()));
    }
}

/*! \brief Start reading from the socket.
 *
 * This slot is called when the client TCP socket emits a readyRead() signal,
 * i.e. when there is data to read.
 */
void RemoteControl::startRead()
{
    char    buffer[1024] = {0};
    int     bytes_read;

    bool ok = true;

    bytes_read = rc_socket->readLine(buffer, 1024);
    if (bytes_read < 2)  // command + '\n'
        return;

    QStringList cmdlist = QString(buffer).trimmed().split(" ", QString::SkipEmptyParts);

    if (cmdlist.size() == 0)
        return;

    // Set new frequency
    if (cmdlist[0] == "F")
    {
        double freq = cmdlist.value(1, "ERR").toDouble(&ok);
        if (ok)
        {
            setNewRemoteFreq((qint64)freq);
            rc_socket->write("RPRT 0\n");
        }
        else
        {
            rc_socket->write("RPRT 1\n");
        }
    }
    // Get frequency
    else if (cmdlist[0] == "f")
    {
        rc_socket->write(QString("%1\n").arg(rc_freq).toLatin1());
    }
    else if (cmdlist[0] == "q" || cmdlist[0] == "Q")
    {
        // FIXME: for now we assume 'close' command
        rc_socket->close();
    }
    // Set level
    else if (cmdlist[0] == "L")
    {
        QString lvl = cmdlist.value(1, "");
        if (lvl == "?")
        {
            rc_socket->write("SQL\n");
        }
        else if (lvl.compare("SQL", Qt::CaseInsensitive) == 0)
        {
            double squelch = cmdlist.value(2, "ERR").toDouble(&ok);
            if (ok)
            {
                rc_socket->write("RPRT 0\n");
                squelch_level = std::max<double>(-150, std::min<double>(0, squelch));
                emit newSquelchLevel(squelch_level);
            }
            else
            {
                rc_socket->write("RPRT 1\n");
            }
        }
        else
        {
            rc_socket->write("RPRT 1\n");
        }
    }
    // Get level
    else if (cmdlist[0] == "l")
    {
        QString lvl = cmdlist.value(1, "");
        if (lvl == "?")
            rc_socket->write("SQL STRENGTH\n");
        else if (lvl.compare("STRENGTH", Qt::CaseInsensitive) == 0 || lvl.isEmpty())
            rc_socket->write(QString("%1\n").arg(signal_level, 0, 'f', 1).toLatin1());
        else if (lvl.compare("SQL", Qt::CaseInsensitive) == 0)
            rc_socket->write(QString("%1\n").arg(squelch_level, 0, 'f', 1).toLatin1());
        else
            rc_socket->write("RPRT 1\n");
    }
    // Mode and filter
    else if (cmdlist[0] == "M")
    {
        QString cmd_arg = cmdlist.value(1, "");
        if (cmd_arg == "?")
            rc_socket->write("OFF RAW AM FM WFM WFM_ST WFM_ST_OIRT LSB USB CW CWL CWR CWU\n");
        else
        {
            int mode = modeStrToInt(cmd_arg);
            if (mode == -1)
            {
                // invalid string
                rc_socket->write("RPRT 1\n");
            }
            else
            {
                rc_mode = mode;
                emit newMode(rc_mode);

                int passband = cmdlist.value(2, "0").toInt();
                if ( passband != 0 )
                    emit newPassband(passband);

                if (rc_mode == 0)
                    audio_recorder_status = false;

                rc_socket->write("RPRT 0\n");
            }
        }
    }
    else if (cmdlist[0] == "m")
    {
        QString msg = QString("%1 %2\n")
                              .arg(intToModeStr(rc_mode))
                              .arg(rc_passband_hi - rc_passband_lo);
        rc_socket->write(msg.toLatin1());
    }
    else if (cmdlist[0] == "U")
    {
        QString func = cmdlist.value(1, "");
        bool ok;
        int status = cmdlist.value(2, "").toInt(&ok);

        if (func == "?")
        {
            rc_socket->write("RECORD\n");
        }
        else if (func == "" || !ok)
        {
            rc_socket->write("RPRT 1\n");
        }
        else if (func.compare("RECORD", Qt::CaseInsensitive) == 0)
        {
            if (rc_mode == 0 || !receiver_running)
            {
                rc_socket->write("RPRT 1\n");
            }
            else
            {
                rc_socket->write("RPRT 0\n");
                audio_recorder_status = status;
                if (status)
                    emit startAudioRecorderEvent();
                else
                    emit stopAudioRecorderEvent();
            }
        }
        else
        {
            rc_socket->write("RPRT 1\n");
        }
    }
    else if (cmdlist[0] == "u")
    {
        QString func = cmdlist.value(1, "");

        if (func == "?")
            rc_socket->write("RECORD\n");
        else if (func.compare("RECORD", Qt::CaseInsensitive) == 0)
            rc_socket->write(QString("%1\n").arg(audio_recorder_status).toLatin1());
        else
            rc_socket->write("RPRT 1\n");
    }


    // Gpredict / Gqrx specific commands:
    //   AOS  - satellite AOS event
    //   LOS  - satellite LOS event
    else if (cmdlist[0] == "AOS")
    {
        if (rc_mode > 0 && receiver_running)
        {
            emit startAudioRecorderEvent();
            audio_recorder_status = true;
        }
        rc_socket->write("RPRT 0\n");

    }
    else if (cmdlist[0] == "LOS")
    {
        emit stopAudioRecorderEvent();
        audio_recorder_status = false;
        rc_socket->write("RPRT 0\n");

    }

    /* dump_state used by some clients, e.g. xdx
     * For now just some quick hack that works taken from
     * https://github.com/hexameron/rtl-sdrangelove/blob/master/plugins/channel/tcpsrc/rigctl.cpp
     *
     * More info in tests/rigctl_parse.c
     */
    else if (cmdlist[0] == "\\dump_state")
    {
        rc_socket->write("0\n"
                         "2\n"
                         "1\n"
                         "150000.000000 30000000.000000  0x900af -1 -1 0x10000003 0x3\n"
                         "0 0 0 0 0 0 0\n"
                         "150000.000000 30000000.000000  0x900af -1 -1 0x10000003 0x3\n"
                         "0 0 0 0 0 0 0\n"
                         "0 0\n"
                         "0 0\n"
                         "0\n"
                         "0\n"
                         "0\n"
                         "0\n"
                         "\n"
                         "\n"
                         "0x0\n"
                         "0x0\n"
                         "0x0\n"
                         "0x0\n"
                         "0x0\n"
                         "0\n");
    }

    else
    {
        // print unknown command and respond with an error
        qWarning() << "Unknown remote command:"
                << cmdlist;
        rc_socket->write("RPRT 1\n");
    }
}

/*! \brief Slot called when the receiver is tuned to a new frequency.
 *  \param freq The new frequency in Hz.
 *
 * Note that this is the frequency gqrx is receiveing on, i.e. the
 * hardware frequency + the filter offset.
 */
void RemoteControl::setNewFrequency(qint64 freq)
{
    rc_freq = freq;
}

/*! \brief Slot called when the filter offset is changed. */
void RemoteControl::setFilterOffset(qint64 freq)
{
    rc_filter_offset = freq;
}

void RemoteControl::setBandwidth(qint64 bw)
{
    // we want to leave some margin
    bw_half = 0.9 * (bw / 2);
}

/*! \brief Set signal level in dBFS. */
void RemoteControl::setSignalLevel(float level)
{
    signal_level = level;
}

/*! \brief Set demodulator (from mainwindow). */
void RemoteControl::setMode(int mode)
{
    rc_mode = mode;

    if (rc_mode == 0)
        audio_recorder_status = false;
}

/*! \brief Set passband (from mainwindow). */
void RemoteControl::setPassband(int passband_lo, int passband_hi)
{
    rc_passband_lo = passband_lo;
    rc_passband_hi = passband_hi;
}

/*! \brief New remote frequency received. */
void RemoteControl::setNewRemoteFreq(qint64 freq)
{
    qint64 delta = freq - rc_freq;

    rc_filter_offset += delta;
    if (((rc_filter_offset > 0) && ((rc_filter_offset + rc_passband_hi) < bw_half))
        || ((rc_filter_offset < 0) && ((rc_filter_offset + rc_passband_lo) > -bw_half)))
    {
        // move filter offset
        emit newFilterOffset(rc_filter_offset);
    }
    else
    {
        // move rx freqeucy and let MainWindow deal with it
        // (will usually change hardware PLL)
        // reset the filter_offset, otherwise the MainWindow will preserve it
        if (rc_filter_offset < 0)
            rc_filter_offset = bw_half - rc_passband_hi;
        else
            rc_filter_offset = -bw_half - rc_passband_lo;
        emit newFilterOffset(rc_filter_offset);
        emit newFrequency(freq);
    }

    rc_freq = freq;
}

/*! \brief Set squelch level (from mainwindow). */
void RemoteControl::setSquelchLevel(double level)
{
    squelch_level = level;
}

/*! \brief Start audio recorder (from mainwindow). */
void RemoteControl::startAudioRecorder(QString unused)
{
    if (rc_mode > 0)
        audio_recorder_status = true;
}

/*! \brief Stop audio recorder (from mainwindow). */
void RemoteControl::stopAudioRecorder()
{
    audio_recorder_status = false;
}

/*! \brief Set receiver status (from mainwindow). */
void RemoteControl::setReceiverStatus(bool enabled)
{
    receiver_running = enabled;
}


/*! \brief Convert mode string to enum (DockRxOpt::rxopt_mode_idx)
 *  \param mode The Hamlib rigctld compatible mode string
 *  \return An integer corresponding to the mode.
 *
 * Following mode strings are recognized: OFF, RAW, AM, FM, WFM,
 * WFM_ST, WFM_ST_OIRT, LSB, USB, CW, CWL, CWU.
 */
int RemoteControl::modeStrToInt(QString mode_str)
{
    int mode_int = -1;

    if (mode_str.compare("OFF", Qt::CaseInsensitive) == 0)
    {
        mode_int = 0;
    }
    else if (mode_str.compare("RAW", Qt::CaseInsensitive) == 0)
    {
        mode_int = 1;
    }
    else if (mode_str.compare("AM", Qt::CaseInsensitive) == 0)
    {
        mode_int = 2;
    }
    else if (mode_str.compare("FM", Qt::CaseInsensitive) == 0)
    {
        mode_int = 3;
    }
    else if (mode_str.compare("WFM", Qt::CaseInsensitive) == 0)
    {
        mode_int = 4;
    }
    else if (mode_str.compare("WFM_ST", Qt::CaseInsensitive) == 0)
    {
        mode_int = 5;
    }
    else if (mode_str.compare("LSB", Qt::CaseInsensitive) == 0)
    {
        mode_int = 6;
    }
    else if (mode_str.compare("USB", Qt::CaseInsensitive) == 0)
    {
        mode_int = 7;
    }
    else if (mode_str.compare("CW", Qt::CaseInsensitive) == 0)
    {
        mode_int = 8;
        hamlib_compatible = true;
    }
    else if (mode_str.compare("CWL", Qt::CaseInsensitive) == 0)
    {
        mode_int = 8;
        hamlib_compatible = false;
    }
    else if (mode_str.compare("CWR", Qt::CaseInsensitive) == 0)
    {
        mode_int = 9;
        hamlib_compatible = true;
    }
    else if (mode_str.compare("CWU", Qt::CaseInsensitive) == 0)
    {
        mode_int = 9;
        hamlib_compatible = false;
    }
    else if (mode_str.compare("WFM_ST_OIRT", Qt::CaseInsensitive) == 0)
    {
        mode_int = 10;
    }

    return mode_int;
}

/*! \brief Convert mode enum to string.
 *  \param mode The mode ID c.f. DockRxOpt::rxopt_mode_idx
 *  \returns The mode string.
 */
QString RemoteControl::intToModeStr(int mode)
{
    QString mode_str;

    switch (mode)
    {
    case 0:
        mode_str = "OFF";
        break;

    case 1:
        mode_str = "RAW";
        break;

    case 2:
        mode_str = "AM";
        break;

    case 3:
        mode_str = "FM";
        break;

    case 4:
        mode_str = "WFM";
        break;

    case 5:
        mode_str = "WFM_ST";
        break;

    case 6:
        mode_str = "LSB";
        break;

    case 7:
        mode_str = "USB";
        break;

    case 8:
        mode_str = (hamlib_compatible) ? "CW" : "CWL";
        break;

    case 9:
        mode_str = (hamlib_compatible) ? "CWR" : "CWU";
        break;

    case 10:
        mode_str = "WFM_ST_OIRT";
        break;

    default:
        mode_str = "ERR";
        break;
    }

    return mode_str;
}
