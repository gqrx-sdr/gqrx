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
#ifndef REMOTE_CONTROL_H
#define REMOTE_CONTROL_H

#include <QObject>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QTcpServer>
#include <QTcpSocket>
#include <QtNetwork>

/*! \brief Simple TCP server for remote control.
 *
 * The TCP interface is compatible with the hamlib rigtctld so that applications
 * gpredict can be used with gqrx without any modifications.
 *
 * The hamlib rigctld protocol is described in the man page
 * http://hamlib.sourceforge.net/pdf/rigctld.8.pdf
 * but here is a summary.
 *
 *   client:  F 144500000\n       # set frequency in Hz
 *     gqrx:  RPRT 0\n            # 0 means no error
 *
 *   client:  f\n                 # get frequency
 *     gqrx:  144500000\n         # gqrx replies with frequency in Hz
 *
 * We also have some gqrx specific commands:
 *
 *  close: Close connection (useful for interactive telnet sessions).
 *
 *
 * FIXME: The server code is very minimalistic and probably not very robust.
 */
class RemoteControl : public QObject
{
    Q_OBJECT
public:
    explicit RemoteControl(QObject *parent = 0);
    ~RemoteControl();

    void start_server(void);
    void stop_server(void);

    void readSettings(QSettings *settings);
    void saveSettings(QSettings *settings) const;

    void setPort(int port);
    int  getPort(void) const
    {
        return rc_port;
    }

    void setHosts(QStringList hosts);
    QStringList getHosts(void) const
    {
        return rc_allowed_hosts;
    }
    void setReceiverStatus(bool enabled);

public slots:
    void setNewFrequency(qint64 freq);
    void setFilterOffset(qint64 freq);
    void setLnbLo(double freq_mhz);
    void setBandwidth(qint64 bw);
    void setSignalLevel(float level);
    void setMode(int mode);
    void setPassband(int passband_lo, int passband_hi);
    void setSquelchLevel(double level);
    void startAudioRecorder(QString unused);
    void stopAudioRecorder();

signals:
    void newFrequency(qint64 freq);
    void newFilterOffset(qint64 offset);
    void newLnbLo(double freq_mhz);
    void newMode(int mode);
    void newPassband(int passband);
    void newSquelchLevel(double level);
    void startAudioRecorderEvent();
    void stopAudioRecorderEvent();

private slots:
    void acceptConnection();
    void startRead();

private:
    QTcpServer  rc_server;         /*!< The active server object. */
    QTcpSocket* rc_socket;         /*!< The active socket object. */

    QStringList rc_allowed_hosts;  /*!< Hosts where we accept connection from. */
    int         rc_port;           /*!< The port we are listening on. */
    
    qint64      rc_freq;
    qint64      rc_filter_offset;
    qint64      bw_half;
    double      rc_lnb_lo_mhz;     /*!< Current LNB LO freq in MHz */

    int         rc_mode;           /*!< Current mode. */
    int         rc_passband_lo;    /*!< Current low cutoff. */
    int         rc_passband_hi;    /*!< Current high cutoff. */
    float       signal_level;      /*!< Signal level in dBFS */
    double      squelch_level;     /*!< Squelch level in dBFS */
    bool        audio_recorder_status; /*!< Recording enabled */
    bool        receiver_running;  /*!< Wether the receiver is running or not */
    bool        hamlib_compatible;

    void        setNewRemoteFreq(qint64 freq);
    int         modeStrToInt(QString mode_str);
    QString     intToModeStr(int mode);

    /* RC commands */
    QString     cmd_get_freq() const;
    QString     cmd_set_freq(QStringList cmdlist);
    QString     cmd_get_mode();
    QString     cmd_set_mode(QStringList cmdlist);
    QString     cmd_get_level(QStringList cmdlist);
    QString     cmd_set_level(QStringList cmdlist);
    QString     cmd_get_func(QStringList cmdlist);
    QString     cmd_set_func(QStringList cmdlist);
    QString     cmd_get_vfo() const;
    QString     cmd_set_vfo(QStringList cmdlist);
    QString     cmd_get_split_vfo() const;
    QString     cmd_set_split_vfo();
    QString     cmd_get_info() const;
    QString     cmd_AOS();
    QString     cmd_LOS();
    QString     cmd_lnb_lo(QStringList cmdlist);
    QString     cmd_dump_state() const;
};

#endif // REMOTE_CONTROL_H
