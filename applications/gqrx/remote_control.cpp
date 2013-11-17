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
#include <stdio.h>
#include <QDebug>
#include <QString>

#include "remote_control.h"

RemoteControl::RemoteControl(QObject *parent) :
    QObject(parent)
{

    rx_freq = 0;
    filter_offset = 0;
    bw_half = 740e3;

    port = 7356;
    allowed_hosts.append("127.0.0.1");

    connect(&server, SIGNAL(newConnection()), this, SLOT(acceptConnection()));

}

RemoteControl::~RemoteControl()
{
    stop_server();
}

/*! \brief Start the server. */
void RemoteControl::start_server()
{
    qDebug() << __func__;
    server.listen(QHostAddress::Any, port);
}

/*! \brief Stop the server. */
void RemoteControl::stop_server()
{
    if (server.isListening())
        server.close();

    socket->close();
}

/*! \brief Read settings. */
void RemoteControl::readSettings(QSettings *settings)
{
    bool conv_ok;

    rx_freq = settings->value("input/frequency", 144500000).toLongLong(&conv_ok);
    filter_offset = settings->value("receiver/offset", 0).toInt(&conv_ok);

    // Get port number; restart server if running
    port = settings->value("remote_control/port", 7356).toInt(&conv_ok);
    if (server.isListening())
    {
        server.close();
        server.listen(QHostAddress::Any, port);
    }

    // get list of allowed hosts
    if (settings->contains("remote_control/port"))
        allowed_hosts = settings->value("remote_control/port").toStringList();
}

void RemoteControl::saveSettings(QSettings *settings) const
{
    if (port != 7356)
        settings->setValue("remote_control/port", port);
    else
        settings->remove("remote_control/port");

    if ((allowed_hosts.count() != 1) || (allowed_hosts.at(0) != "127.0.0.1"))
        settings->setValue("remote_control/allowed_hosts", allowed_hosts);
    else
        settings->remove("remote_control/allowed_hosts");
}


/*! \brief Accept a new client connection.
 *
 * This slot is called when a client opens a new connection.
 */
void RemoteControl::acceptConnection()
{
    socket = server.nextPendingConnection();

    // check if host is allowed
    QString address = socket->peerAddress().toString();
    if (allowed_hosts.indexOf(address) == -1)
    {
        qDebug() << "Connection attempt from" << address << "(not in allowed list)";
        socket->close();
    }
    else
    {
        connect(socket, SIGNAL(readyRead()), this, SLOT(startRead()));
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


    bytes_read = socket->readLine(buffer, 1024);
    if (bytes_read < 2)  // command + '\n'
        return;

    printf("%s", buffer);

    if (buffer[0] == 'F')
    {
        // set frequency
        if (sscanf(buffer,"F %lld\n", &freq) == 1)
        {
            setNewRemoteFreq(freq);
            socket->write("RPRT 0\n");
        }
        else
        {
            socket->write("RPRT 1\n");
        }
    }
    else if (buffer[0] == 'f')
    {
        // get frequency
        socket->write(QString("%1\n").arg(rx_freq).toAscii());
    }
    else if (buffer[0] == 'c')
    {
        // FIXME: for now we assume 'close' command
        socket->close();
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
    rx_freq = freq;
}

/*! \brief Slot called when the filter offset is changed. */
void RemoteControl::setFilterOffset(qint64 freq)
{
    filter_offset = freq;
}

void RemoteControl::setBandwidth(qint64 bw)
{
    // we want to leave some margin
    bw_half = 0.9 * (bw / 2);
}

/*! \brief New remote frequency received. */
void RemoteControl::setNewRemoteFreq(qint64 freq)
{
    qint64 delta = freq - rx_freq;

    if (abs(filter_offset + delta) < bw_half)
    {
        // move filter offset
        filter_offset += delta;
        emit newFilterOffset(filter_offset);
    }
    else
    {
        // move rx freqeucy and let MainWindow deal with it
        // (will usually change hardware PLL)
        emit newFrequency(freq);
    }

    rx_freq = freq;
}
