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
        rc_socket->write(QString("%1\n").arg(rc_freq).toAscii());
    }
    else if (buffer[0] == 'c')
    {
        // FIXME: for now we assume 'close' command
        rc_socket->close();
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
