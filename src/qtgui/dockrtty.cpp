/*
 * rtty gui widget implementation
 *
 * Copyright 2022 Marc CAPDEVILLE F4JMZ
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */
#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include "dockrtty.h"
#include "ui_dockrtty.h"

DockRTTY::DockRTTY(QWidget *parent) :
   QDockWidget(parent),
   ui(new Ui::DockRTTY) {
      ui->setupUi(this);
      ui->mode->addItem(QString("5 bits baudot"));
      ui->mode->addItem(QString("7 bits ascii"));
      ui->mode->addItem(QString("8 bits ascii"));

      ui->parity->addItem(QString("none"));
      ui->parity->addItem(QString("odd"));
      ui->parity->addItem(QString("even"));
      ui->parity->addItem(QString("mark"));
      ui->parity->addItem(QString("space"));
      ui->parity->addItem(QString("dont care"));
   }

DockRTTY::~DockRTTY() {
   delete ui;
}

QString DockRTTY::get_text() {
   return ui->text->toPlainText();
}


void DockRTTY::ClearText() {
   ui->text->setText("");
}

float DockRTTY::get_baud_rate() {
   return ui->baud_rate->value();
}

float DockRTTY::get_mark_freq() {
   return ui->mark_freq->value();
}

float DockRTTY::get_space_freq() {
   return ui->space_freq->value();
}

float DockRTTY::get_threshold() {
   return ui->threshold->value();
}

float DockRTTY::get_bandwidth() {
   return ui->bandwidth->value();
}

float DockRTTY::get_transwidth() {
   return ui->transwidth->value();
}

float DockRTTY::get_filterlen() {
   return ui->filterlen->value();
}

int   DockRTTY::get_mode() {
   return ui->mode->currentIndex();
}

int   DockRTTY::get_parity() {
   return ui->parity->currentIndex();
}

QString DockRTTY::get_directory() {
   return ui->directory->text();
}

void DockRTTY::saveSettings(QSettings *parent_settings) {
   if (!parent_settings)
      return;

   settings = parent_settings;

   settings->beginGroup("rtty");
   settings->setValue("num_profile",QVariant(num_profile).toString());
   settings->setValue("current_profile",QVariant(ui->profile->currentData()).toString());
   settings->setValue("mark_freq", QVariant(get_mark_freq()).toString());
   settings->setValue("space_freq", QVariant(get_space_freq()).toString());
   settings->setValue("threshold", QVariant(get_threshold()).toString());
   settings->setValue("bandwidth", QVariant(get_bandwidth()).toString());
   settings->setValue("transwidth", QVariant(get_transwidth()).toString());
   settings->setValue("filterlen", QVariant(get_filterlen()).toString());
   settings->setValue("baud_rate", QVariant(get_baud_rate()).toString());
   settings->setValue("mode", QVariant(get_mode()).toString());
   settings->setValue("parity", QVariant(get_parity()).toString());
   settings->setValue("directory", QVariant(get_directory()).toString());
   settings->setValue("autosave", QVariant(get_autosave()).toString());
   settings->endGroup();
}

void DockRTTY::readSettings(QSettings *parent_settings) {
   int i;
   int current_profile;

   if (!parent_settings)
      return;

   settings = parent_settings;

   num_profile = settings->value("rtty/num_profile",0).toInt();
   current_profile = settings->value("rtty/current_profile",0).toInt();
   ui->profile->setCurrentIndex(-1);

   if (num_profile) {
      for (i=0;i<num_profile;i++) {
         ui->profile->addItem(settings->value(QString("rtty_profile_%1/name").arg(i)).toString(),i);
      }
      ui->profile->setCurrentIndex(ui->profile->findData(current_profile));
   }

   settings->beginGroup("rtty");
   set_mark_freq(settings->value("mark_freq", 85.0f).toFloat());
   set_space_freq(settings->value("space_freq", -85.0f).toFloat());
   set_threshold(settings->value("threshold", 6.0f).toFloat());
   set_bandwidth(settings->value("bandwidth", 90.0f).toFloat());
   set_transwidth(settings->value("transwidth", 20.0f).toFloat());
   set_filterlen(settings->value("filterlen", 0.8f).toFloat());
   set_baud_rate(settings->value("baud_rate", 45.45f).toFloat());
   set_mode(settings->value("mode", 0).toInt());
   set_parity(settings->value("parity", 0).toInt());
   set_directory(settings->value("directory", "").toString());
   set_autosave(settings->value("autosave", false).toBool());
   settings->endGroup();

}

void DockRTTY::update_text(QString text) {
   ui->text->moveCursor(QTextCursor::End);
   ui->text->insertPlainText(text);
}

void DockRTTY::show_Enabled() {
   if (!ui->rttyEnable->isChecked()) {
      ui->rttyEnable->blockSignals(true);
      ui->rttyEnable->setChecked(true);
      ui->rttyEnable->blockSignals(false);
   }
}

void DockRTTY::show_Disabled() {
   if (ui->rttyEnable->isChecked()) {
      ui->rttyEnable->blockSignals(true);
      ui->rttyEnable->setChecked(false);
      ui->rttyEnable->blockSignals(false);
   }
}

void DockRTTY::set_Disabled() {
   ui->rttyEnable->setDisabled(false);
   ui->rttyEnable->blockSignals(true);
   ui->rttyEnable->setChecked(false);
   ui->rttyEnable->blockSignals(false);
   ui->baud_rate->setDisabled(true);
   ui->mark_freq->setDisabled(true);
   ui->space_freq->setDisabled(true);
   ui->threshold->setDisabled(true);
   ui->bandwidth->setDisabled(true);
   ui->transwidth->setDisabled(true);
   ui->filterlen->setDisabled(true);
   ui->text->setDisabled(true);
   ui->mode->setDisabled(true);
}

void DockRTTY::set_Enabled() {
   ui->rttyEnable->setDisabled(false);
   ui->baud_rate->setDisabled(false);
   ui->mark_freq->setDisabled(false);
   ui->space_freq->setDisabled(false);
   ui->threshold->setDisabled(false);
   ui->bandwidth->setDisabled(false);
   ui->transwidth->setDisabled(false);
   ui->filterlen->setDisabled(false);
   ui->text->setDisabled(false);
   ui->mode->setDisabled(false);
}

void DockRTTY::set_baud_rate(float baud_rate) {
   ui->baud_rate->setValue((double)baud_rate);
}

void DockRTTY::set_mark_freq(float mark_freq) {
   ui->mark_freq->setValue((double)mark_freq);
}

void DockRTTY::set_space_freq(float space_freq) {
   ui->space_freq->setValue((double)space_freq);
}

void DockRTTY::set_threshold(float threshold) {
   ui->threshold->setValue(threshold);
}

void DockRTTY::set_bandwidth(float bandwidth) {
   ui->bandwidth->setValue((double)bandwidth);
}

void DockRTTY::set_transwidth(float transwidth) {
   ui->transwidth->setValue((double)transwidth);
}

void DockRTTY::set_filterlen(float filterlen) {
   ui->filterlen->setValue((double)filterlen);
}

void DockRTTY::set_mode(int mode) {
   ui->mode->setCurrentIndex(mode);
}

void DockRTTY::set_parity(int parity) {
   ui->parity->setCurrentIndex(parity);
}

void DockRTTY::set_directory(QString dir) {
   ui->directory->setText(dir);
}

void DockRTTY::set_autosave(bool state) {
   ui->autosave->setChecked(state);
}

/** Enable/disable RTTY decoder */
void DockRTTY::on_rttyEnable_toggled(bool checked) {
   if (checked)
      emit rtty_start_decoder();
   else
      emit rtty_stop_decoder();
}

void DockRTTY::save_profile(int num) {
   if (num<=num_profile) {
      settings->beginGroup(QString("rtty_profile_%1").arg(num));
      settings->setValue("name", ui->profile->currentText());
      settings->setValue("mark_freq", QVariant(get_mark_freq()).toString());
      settings->setValue("space_freq", QVariant(get_space_freq()).toString());
      settings->setValue("threshold", QVariant(get_threshold()).toString());
      settings->setValue("bandwidth", QVariant(get_bandwidth()).toString());
      settings->setValue("transwidth", QVariant(get_transwidth()).toString());
      settings->setValue("filterlen", QVariant(get_filterlen()).toString());
      settings->setValue("baud_rate", QVariant(get_baud_rate()).toString());
      settings->setValue("mode", QVariant(get_mode()).toString());
      settings->setValue("parity", QVariant(get_parity()).toString());
      settings->setValue("directory", QVariant(get_directory()).toString());
      settings->setValue("autosave", QVariant(get_autosave()).toString());
      settings->endGroup();
      if (num == num_profile) {
         num_profile++;
         settings->setValue("rtty/num_profile",num_profile);
      }
   }
}

void DockRTTY::load_profile(int num) {
   if (num<num_profile) {
      settings->beginGroup(QString("rtty_profile_%1").arg(num));
      set_mark_freq(settings->value("mark_freq", -85.0f).toFloat());
      set_space_freq(settings->value("space_freq", 85.0f).toFloat());
      set_threshold(settings->value("threshold", 6.0f).toFloat());
      set_bandwidth(settings->value("bandwidth", 100.0f).toFloat());
      set_transwidth(settings->value("transwidth", 25.0f).toFloat());
      set_filterlen(settings->value("filterlen", 1.0f).toFloat());
      set_baud_rate(settings->value("baud_rate", 45.45f).toFloat());
      set_mode(settings->value("mode", 0).toInt());
      set_parity(settings->value("parity", 0).toInt());
      settings->endGroup();
   }
}

void DockRTTY::on_profile_currentIndexChanged(int idx) {
   if (idx!=-1) {

      load_profile(ui->profile->currentData().toInt());
   }
}

void DockRTTY::on_load_profile_clicked() {
   if (ui->profile->currentIndex()!=-1)
      load_profile(ui->profile->currentData().toInt());
}

void DockRTTY::on_save_profile_clicked() {
   if (ui->profile->currentIndex()!=-1) {
      ui->profile->setItemText(ui->profile->currentIndex(),ui->profile->currentText());
      save_profile(ui->profile->currentData().toInt());
   }
}

void DockRTTY::on_new_profile_clicked() {
   int current_profile = num_profile;

   if (ui->profile->currentText().isEmpty())
      ui->profile->setCurrentText(QString("Profile %1").arg(current_profile));

   ui->profile->addItem(ui->profile->currentText(),current_profile);
   save_profile(current_profile);

   ui->profile->setCurrentIndex(ui->profile->findData(current_profile));

}

void DockRTTY::on_delete_profile_clicked() {
   int i,j;

   if (ui->profile->currentIndex()!=-1) {
      num_profile--;
      for (i=ui->profile->currentData().toInt();i<num_profile;i++) {
         settings->beginGroup(QString("rtty_profile_%1").arg(i+1));
         QStringList keys = settings->allKeys();
         settings->endGroup();

         for (j=0;j<keys.size();j++)
            settings->setValue(QString("rtty_profile_%1/%2").arg(i).arg(keys.at(j)),settings->value(QString("rtty_profile_%1/%2").arg(i+1).arg(keys.at(j))));

         ui->profile->setItemText(ui->profile->findData(i),settings->value(QString("rtty_profile_%1/name").arg(i)).toString());
      }

      ui->profile->removeItem(ui->profile->findData(num_profile));
      settings->remove(QString("rtty_profile_%1").arg(num_profile));
      settings->setValue(QString("rtty/num_profile"),num_profile);
      ui->profile->setCurrentIndex(-1);
   }
}

void DockRTTY::on_baud_rate_editingFinished() {
   if (ui->baud_rate->value()>0)
      emit rtty_baud_rate_Changed(ui->baud_rate->value());
}

void DockRTTY::on_mark_freq_editingFinished() {
   emit rtty_mark_freq_Changed(ui->mark_freq->value());
}

void DockRTTY::on_space_freq_editingFinished() {
   emit rtty_space_freq_Changed(ui->space_freq->value());
}

void DockRTTY::on_threshold_valueChanged(int threshold) {
   emit rtty_threshold_Changed(threshold);
}

void DockRTTY::on_bandwidth_valueChanged(double bandwidth) {
   emit rtty_bandwidth_Changed((float)bandwidth);
}

void DockRTTY::on_transwidth_valueChanged(double transwidth) {
   emit rtty_transwidth_Changed((float)transwidth);
}

void DockRTTY::on_filterlen_valueChanged(double filterlen) {
   emit rtty_filterlen_Changed((float)filterlen);
}

void DockRTTY::on_mode_currentIndexChanged(int mode) {
   emit rtty_mode_Changed(mode);
}

void DockRTTY::on_parity_currentIndexChanged(int parity) {
   emit rtty_parity_Changed(parity);
}

void DockRTTY::on_reset_clicked() {
   ui->text->setText("");
   emit rtty_reset_clicked();
}

void DockRTTY::on_swap_clicked() {
   float mark,space;

   mark=ui->space_freq->value();
   space=ui->mark_freq->value();

   ui->mark_freq->setValue((double)mark);
   ui->space_freq->setValue((double)space);

   emit rtty_mark_freq_Changed(ui->mark_freq->value());
   emit rtty_space_freq_Changed(ui->space_freq->value());
}

void DockRTTY::on_save_clicked() {
   int ret;
   ret = emit rtty_save_clicked();
   if (!ret)
      QMessageBox::information(this,"Rtty","Saved.");
   else
      QMessageBox::critical(this,"Rtty","Not Saved");
}

bool DockRTTY::get_autosave() {
   return ui->autosave->checkState();
}

void DockRTTY::on_select_clicked() {
   QString Directory = QFileDialog::getExistingDirectory(this,"Select directory",ui->directory->text());
   ui->directory->setText(Directory);
}
