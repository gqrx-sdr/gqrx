#ifndef DOCKRDS_H
#define DOCKRDS_H
#include <QDockWidget>
#include <QSettings>

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
      void showNotSupported();

private:

signals:


private slots:


private:
    Ui::DockRDS *ui;        /*! The Qt designer UI file. */


};

#endif // DOCKRDS_H
