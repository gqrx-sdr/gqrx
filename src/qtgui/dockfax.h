/*
 * fax gui widget header
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

#ifndef DOCKFAX_H
#define DOCKFAX_H
#include <QDockWidget>
#include <QSettings>

namespace Ui {
   class DockFAX;
}


class DockFAX : public QDockWidget
{
   Q_OBJECT

   public:
      explicit DockFAX(QWidget *parent = 0);
      ~DockFAX();

      float get_lpm();
      float get_ioc();
      float get_black_freq();
      float get_white_freq();
      QString get_directory();
      bool get_autosave();

      void saveSettings(QSettings *settings);
      void readSettings(QSettings *settings);

      public slots:
         void update_image(QImage& image);
      void clearView();
      void show_Enabled();
      void show_Disabled();
      void set_Enabled();
      void set_Disabled();
      void set_lpm(float);
      void set_ioc(float);
      void set_black_freq(float);
      void set_white_freq(float);
      void set_directory(QString);
      void set_autosave(bool);

   private:
      float d_lpm;
      float d_ioc;
      float d_black_freq;
      float d_white_freq;
      float d_scale;

signals:
      void fax_start_decoder();
      void fax_stop_decoder();
      void fax_lpm_Changed(float);
      void fax_ioc_Changed(float);
      void fax_black_freq_Changed(float);
      void fax_white_freq_Changed(float);
      void fax_start_Clicked();
      void fax_reset_Clicked();
      void fax_sync_Clicked();
      int fax_save_Clicked();

      private slots:
         void on_faxEnable_toggled(bool);
      void on_lpm_editingFinished();
      void on_ioc_editingFinished();
      void on_black_freq_editingFinished();
      void on_white_freq_editingFinished();
      void on_reset_clicked();
      void on_start_clicked();
      void on_sync_clicked();
      void on_save_clicked();
      void on_select_clicked();

   private:
      Ui::DockFAX *ui;        /*! The Qt designer UI file. */
};

#endif // DOCKFAX_H
