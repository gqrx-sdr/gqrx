#ifndef DXC_OPTIONS_H
#define DXC_OPTIONS_H


#include <QCloseEvent>
#include <QShowEvent>
#include <QTcpSocket>
#include <QSettings>

#include <QDialog>

namespace Ui {
class DXC_Options;
}

class DXC_Options : public QDialog
{
    Q_OBJECT

public:
    explicit DXC_Options(QWidget *parent = 0);
    ~DXC_Options();

    void closeEvent(QCloseEvent *event);
    void showEvent(QShowEvent * event);
    void saveSettings(QSettings *settings);
    void readSettings(QSettings *settings);

private slots:

    void on_pushButton_DXCConnect_clicked();
    void on_pushButton_DXCDisconnect_clicked();
    void connected();
    void disconnected();
    void readyToRead();

private:
    Ui::DXC_Options *ui;
    QTcpSocket *TCPSocket;
};

#endif // DXC_OPTIONS_H
