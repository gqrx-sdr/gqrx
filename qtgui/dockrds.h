#ifndef DOCKRDS_H
#define DOCKRDS_H
#include <QDockWidget>
#include <QSettings>
//#include "qtgui/agc_options.h"
//#include "qtgui/demod_options.h"
//#include "qtgui/nb_options.h"

namespace Ui {
    class DockRDS;
}


class DockRDS : public QDockWidget
{
    Q_OBJECT

public:
    explicit DockRDS(QWidget *parent = 0);
    ~DockRDS();


public slots:
      void updateRDS(QString text, int type);
      void showEnabled();
      void showDisabled();

private:

signals:


private slots:


private:
    Ui::DockRDS *ui;        /*! The Qt designer UI file. */


};

#endif // DOCKRDS_H
