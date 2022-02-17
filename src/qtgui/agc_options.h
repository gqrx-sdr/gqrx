/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2012-2013 Alexandru Csete OZ9AEC.
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
#ifndef AGC_OPTIONS_H
#define AGC_OPTIONS_H

#include <QDialog>
#include <QCloseEvent>

namespace Ui {
class CAgcOptions;
}

/*! \brief Dialog windows with advanced AGC controls.
 *  \ingroup UI
 *
 * By default the user is presented with a combo box and a small tool button.
 * The combo box contains the most common AGC presets (fast, medium, slow, user)
 * plus an option to switch AGC OFF. The controls for the individual AGC parameters
 * are inside this dialog and can be shown using the small tool button next to
 * the combo box containing the presets.
 *
 * \todo A graph that shows the current AGC profile updated in real time.
 */

class CAgcOptions : public QDialog
{
    Q_OBJECT
    
public:
    explicit CAgcOptions(QWidget *parent = 0);
    ~CAgcOptions();

    void closeEvent(QCloseEvent *event);

    int gain();
    void setGain(int value);
    void enableGain(bool enabled);

    int maxGain();
    void setMaxGain(int value);

    int targetLevel();
    void setTargetLevel(int value);

    int attack();
    void setAttack(int value);
    void enableAttack(bool enabled);

    int decay();
    void setDecay(int value);
    void enableDecay(bool enabled);

    int hang();
    void setHang(int value);
    void enableHang(bool enabled);


    enum agc_preset_e
    {
        AGC_FAST = 0,    /*! decay =  500 ms, slope = 2 */
        AGC_MEDIUM = 1,  /*! decay = 1500 ms, slope = 2 */
        AGC_SLOW = 2,    /*! decay = 3000 ms, slope = 2 */
        AGC_USER = 3,
        AGC_OFF = 4
    };

    void setPreset(agc_preset_e preset);

signals:
    void gainChanged(int gain);
    void maxGainChanged(int gain);
    void targetLevelChanged(int level);
    void attackChanged(int decay);
    void decayChanged(int decay);
    void hangChanged(int hang);

private slots:
    void on_gainSlider_valueChanged(int value);
    void on_maxGainSlider_valueChanged(int value);
    void on_targetLevelSlider_valueChanged(int value);
    void on_attackSlider_valueChanged(int value);
    void on_decaySlider_valueChanged(int value);
    void on_hangSlider_valueChanged(int value);

private:
    Ui::CAgcOptions *ui;
};

#endif // AGC_OPTIONS_H
