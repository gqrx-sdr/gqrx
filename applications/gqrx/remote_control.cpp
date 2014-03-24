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
#include <QString>

#include "remote_control.h"

RemoteControl::RemoteControl(QObject *parent) :
    QObject(parent)
{

    rc_freq = 0;
    rc_filter_offset = 0;
    bw_half = 740e3;

    signal_level = -200.0;

    rc_port = 7356;
    rc_allowed_hosts.append("127.0.0.1");

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
    bool conv_ok;

    rc_freq = settings->value("input/frequency", 144500000).toLongLong(&conv_ok);
    rc_filter_offset = settings->value("receiver/offset", 0).toInt(&conv_ok);

    // Get port number; restart server if running
    rc_port = settings->value("remote_control/port", 7356).toInt(&conv_ok);
    if (rc_server.isListening())
    {
        rc_server.close();
        rc_server.listen(QHostAddress::Any, rc_port);
    }

    // get list of allowed hosts
    if (settings->contains("remote_control/allowed_hosts"))
        rc_allowed_hosts = settings->value("remote_control/allowed_hosts").toStringList();
}

void RemoteControl::saveSettings(QSettings *settings) const
{
    if (rc_port != 7356)
        settings->setValue("remote_control/port", rc_port);
    else
        settings->remove("remote_control/port");

    if ((rc_allowed_hosts.count() != 1) || (rc_allowed_hosts.at(0) != "127.0.0.1"))
        settings->setValue("remote_control/allowed_hosts", rc_allowed_hosts);
    else
        settings->remove("remote_control/allowed_hosts");
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
    rc_allowed_hosts.clear();

    for (int i = 0; i < hosts.count(); i++)
        rc_allowed_hosts << hosts.at(i);
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
        qDebug() << "Connection attempt from" << address << "(not in allowed list)";
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
    qint64  freq;


    bytes_read = rc_socket->readLine(buffer, 1024);
    if (bytes_read < 2)  // command + '\n'
        return;

    if (buffer[0] == 'F')
    {
        // set frequency
        if (sscanf(buffer,"F %lld\n", &freq) == 1)
        {
            setNewRemoteFreq(freq);
            rc_socket->write("RPRT 0\n");
        }
        else
        {
            rc_socket->write("RPRT 1\n");
        }
    }
    else if (buffer[0] == 'f')
    {
        // get frequency
        rc_socket->write(QString("%1\n").arg(rc_freq).toLatin1());
    }
    else if (buffer[0] == 'c')
    {
        // FIXME: for now we assume 'close' command
        rc_socket->close();
    }

    // Get level
    // FIXME: For now only signal strength is returned
    else if (buffer[0] == 'l')
    {
        rc_socket->write(QString("%1\n").arg(signal_level, 0, 'f', 1).toLatin1());
    }

    // Mode and filter
    else if (buffer[0] == 'M')
    {
        int mode = modeStrToInt(buffer);
        if (mode == -1)
        {
            // invalid string
            rc_socket->write("RPRT 1\n");
        }
        else
        {
            rc_socket->write("RPRT 0\n");
            rc_mode = mode;
            emit newMode(rc_mode);
        }
    }
    else if (buffer[0] == 'm')
    {
        rc_socket->write(QString("%1\n").arg(intToModeStr(rc_mode)).toLatin1());
    }


    // Gpredict / Gqrx specific commands:
    //   AOS  - satellite AOS event
    //   LOS  - satellite LOS event
    else if (bytes_read >= 4 && buffer[1] == 'O' && buffer[2] == 'S')
    {
        if (buffer[0] == 'A')
        {
            emit satAosEvent();
            rc_socket->write("RPRT 0\n");
        }
        else if (buffer[0] == 'L')
        {
            emit satLosEvent();
            rc_socket->write("RPRT 0\n");
        }
        else
        {
            rc_socket->write("RPRT 1\n");
        }
    }

    else
    {
        // respond with an error
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
}

/*! \brief New remote frequency received. */
void RemoteControl::setNewRemoteFreq(qint64 freq)
{
    qint64 delta = freq - rc_freq;

    if (abs(rc_filter_offset + delta) < bw_half)
    {
        // move filter offset
        rc_filter_offset += delta;
        emit newFilterOffset(rc_filter_offset);
    }
    else
    {
        // move rx freqeucy and let MainWindow deal with it
        // (will usually change hardware PLL)
        emit newFrequency(freq);
    }

    rc_freq = freq;
}


/*! \brief Convert mode string to enum (DockRxOpt::rxopt_mode_idx)
 *  \param mode The Hamlib rigctld compatible mode string
 *  \return An integer corresponding to the mode.
 *
 * Following mode strings are recognized: OFF, RAW, AM, FM, WFM,
 * WFM_ST, LSB, USB, CW, CWL, CWU.
 */
int RemoteControl::modeStrToInt(const char *buffer)
{
    int mode_int = 0;

    QString mode_str = QString(buffer).split(' ', QString::SkipEmptyParts).at(1).trimmed();

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
    }
    else if (mode_str.compare("CWL", Qt::CaseInsensitive) == 0)
    {
        mode_int = 8;
    }
    else if (mode_str.compare("CWU", Qt::CaseInsensitive) == 0)
    {
        mode_int = 9;
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
        mode_str = "CWL";
        break;

    case 9:
        mode_str = "CWU";
        break;

    default:
        mode_str = "ERR";
        break;
    }

    return mode_str;
}
