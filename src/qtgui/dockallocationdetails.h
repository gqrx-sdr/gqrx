#ifndef DOCKALLOCATIONDETAILS_H
#define DOCKALLOCATIONDETAILS_H
#include <QDockWidget>
#include <QSettings>
#include <QtWebKitWidgets/QWebView>



namespace Ui {
    class DockAllocationDetails;
}


class DockAllocationDetails : public QDockWidget
{
    Q_OBJECT

public:
    explicit DockAllocationDetails(QWidget *parent = 0);
    ~DockAllocationDetails();

public slots:
      void updateRDS(QString text, int type);
      void showEnabled();
      void showDisabled();
      void setEnabled();
      void setDisabled();

private:

signals:
    void rdsDecoderToggled(bool);

private slots:
      void on_rdsCheckbox_toggled(bool checked);

private:
    Ui::DockAllocationDetails *ui;        /*! The Qt designer UI file. */
};

#endif // DOCKALLOCATIONDETAILS_H
