/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2023 Jeff Long.
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
#ifndef DATA_CONTROLS_H
#define DATA_CONTROLS_H

#include <QCloseEvent>
#include <QDialog>
#include <QSettings>
#include <QShowEvent>
#include <QString>

#define DEFAULT_URI "tcp://127.0.0.1:5000"

namespace Ui {
    class CDataControls;
}

/*! \brief User interface for data output controls. */
class CDataControls : public QDialog
{
    Q_OBJECT

public:
    explicit CDataControls(QWidget *parent = 0);
    ~CDataControls();

    void closeEvent(QCloseEvent *event);
    void showEvent(QShowEvent * event);

    void saveSettings(QSettings *settings);
    void readSettings(QSettings *settings);

    QString uri();
    float interval();
    bool outputEnabled();
    bool samplesEnabled();
    bool linearFFTEnabled();

    void setOutputEnabled(bool enable);
    void setStatus(const std::string& status);

signals:
    void settingsChanged();

// public slots:

private slots:
    void on_uriResetButton_clicked();
    void on_uriEdit_textChanged(const QString& text);
    void on_dataIntervalBox_currentIndexChanged(int index);
    void on_enableCheckBox_stateChanged(int state);
    void on_samplesCheckBox_stateChanged(int state);
    void on_linearFFTCheckBox_stateChanged(int state);

private:
    Ui::CDataControls *ui;
};

#endif // DATA_CONTROLS_H
