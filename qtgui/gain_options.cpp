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
#include <QDebug>

#include "gain_options.h"
#include "ui_gain_options.h"

CGainOptions::CGainOptions(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CGainOptions)
{
    ui->setupUi(this);

    layout = new QGridLayout(this);
    setLayout(layout);
}

CGainOptions::~CGainOptions()
{
    clearWidgets();
    delete layout;
    delete ui;
}

/*! \brief Catch window close events.
 *
 * This method is called when the user closes the demod options dialog
 * window using the window close icon. We catch the event and hide the
 * dialog but keep it around for later use.
 */
void CGainOptions::closeEvent(QCloseEvent *event)
{
    hide();
    event->ignore();
}

/*! \brief Set gain parameter. */
void CGainOptions::setGain(QString &name, double gain)
{
    int val = -1;

    for (int idx = 0; idx < gain_labels.length(); idx++)
    {
        if (gain_labels.at(idx)->text() == name)
        {
            val = (int)(10 * gain);
            gain_sliders.at(idx)->setValue(val);
            break;
        }
    }
}

/*! \brief Set gain stages for new device. */
void CGainOptions::setGainStages(gain_list_t &gain_list)
{
    QLabel  *label;
    QSlider *slider;
    QLabel  *value;
    int start, stop, step, gain;

    // ensure that gain lists are empty
    clearWidgets();

    for (unsigned int i = 0; i < gain_list.size(); i++)
    {
        start = (int)(10.0 * gain_list[i].start);
        stop  = (int)(10.0 * gain_list[i].stop);
        step  = (int)(10.0 * gain_list[i].step);
        gain  = (int)(10.0 * gain_list[i].value);

        label = new QLabel(gain_list[i].name.c_str(), this);
        label->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum));

        value = new QLabel(QString("%1 dB").arg(gain_list[i].value, 0, 'f', 1), this);
        value->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum));

        slider = new QSlider(Qt::Horizontal, this);
        slider->setProperty("idx", i);
        slider->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum));
        slider->setRange(start, stop);
        slider->setSingleStep(step);
        slider->setValue(gain);
        if (abs(stop - start) > 10 * step)
            slider->setPageStep(10 * step);

        layout->addWidget(label, i, 0, Qt::AlignLeft);
        layout->addWidget(slider, i, 1, Qt::AlignCenter);
        layout->addWidget(value, i, 2, Qt::AlignLeft);

        gain_labels.push_back(label);
        gain_sliders.push_back(slider);
        value_labels.push_back(value);

        connect(slider, SIGNAL(valueChanged(int)), this, SLOT(sliderValueChanged(int)));
    }

    qDebug() << "********************";
    for (gain_list_t::iterator it = gain_list.begin(); it != gain_list.end(); ++it)
    {
        qDebug() << "Gain name:" << QString(it->name.c_str());
        qDebug() << "      min:" << it->start;
        qDebug() << "      max:" << it->stop;
        qDebug() << "     step:" << it->step;
    }
    qDebug() << "********************";
}

/*! \brief Remove all widgets from the lists. */
void CGainOptions::clearWidgets()
{
    QWidget *widget;

    // sliders
    while (!gain_sliders.isEmpty())
    {
        widget = gain_sliders.takeFirst();
        layout->removeWidget(widget);
        delete widget;
    }

    // labels
    while (!gain_labels.isEmpty())
    {
        widget = gain_labels.takeFirst();
        layout->removeWidget(widget);
        delete widget;
    }

    // value labels
    while (!value_labels.isEmpty())
    {
        widget = value_labels.takeFirst();
        layout->removeWidget(widget);
        delete widget;
    }
}

/*! \brief Slot for managing slider value changed signals.
 *  \param value The value of the slider.
 *
 * Note. We use the sender() function to find out which slider has emitted the signal.
 */
void CGainOptions::sliderValueChanged(int value)
{
    QSlider *slider = (QSlider *) sender();
    int idx = slider->property("idx").toInt();

    // convert to discrete value according to step
    value = slider->singleStep() * (value / slider->singleStep());

    // convert to double and send signal
    double gain = (double)value / 10.0;
    updateLabel(idx, gain);
    emit gainChanged(gain_labels.at(idx)->text(), gain);
}

/*! \brief Update value label
 *  \param idx The index of the gain
 *  \param value The new value
 */
void CGainOptions::updateLabel(int idx, double value)
{
    QLabel *label = value_labels.at(idx);

    label->setText(QString("%1 dB").arg(value, 0, 'f', 1));
}
