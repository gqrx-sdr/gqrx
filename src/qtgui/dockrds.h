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
    void setEnabled();
    void setDisabled();

private:
    void ClearTextFields();

signals:
    void rdsDecoderToggled(bool);

private slots:
    void on_rdsCheckbox_toggled(bool checked);

private:
    Ui::DockRDS *ui;        /*! The Qt designer UI file. */
};

#endif // DOCKRDS_H
