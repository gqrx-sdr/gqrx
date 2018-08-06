#include "dxc_options.h"
#include "ui_dxc_options.h"
#include <QTcpSocket>
#include <QString>
#include <QStringList>
#include "dxc_spots.h"

DXC_Options::DXC_Options(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DXC_Options)
{
    ui->setupUi(this);
    TCPSocket = new QTcpSocket(this);
    connect(TCPSocket, SIGNAL(connected()),this, SLOT(connected()));
    connect(TCPSocket, SIGNAL(disconnected()),this, SLOT(disconnected()));
    connect(TCPSocket, SIGNAL(readyRead()),this, SLOT(readyToRead()));
}

DXC_Options::~DXC_Options()
{
    delete ui;
}

/*! \brief Catch window close events.
 *
 * This method is called when the user closes the audio options dialog
 * window using the window close icon. We catch the event and hide the
 * dialog but keep it around for later use.
 */
void DXC_Options::closeEvent(QCloseEvent *event)
{
    hide();
    event->ignore();
}

/*! \brief Catch window show events. */
void DXC_Options::showEvent(QShowEvent * event)
{
    Q_UNUSED(event);
}

void DXC_Options::on_pushButton_DXCConnect_clicked()
{
    DXCSpots::Get().setSpotTimeout(ui->lineEdit_DXCSpottimeout->text().toInt());
    TCPSocket->connectToHost(ui->lineEdit_DXCAddress->text(),ui->lineEdit_DXCPort->text().toInt());
    if(!TCPSocket->waitForConnected(5000))
    {
        ui->plainTextEdit_DXCMonitor->appendPlainText(TCPSocket->errorString());
    }
}

void DXC_Options::on_pushButton_DXCDisconnect_clicked()
{
    TCPSocket->write("Bye\r\n");
}

void DXC_Options::connected()
{
    ui->plainTextEdit_DXCMonitor->appendPlainText("Connected");
    ui->pushButton_DXCConnect->setDisabled(true);
    ui->pushButton_DXCDisconnect->setEnabled(true);
}
void DXC_Options::disconnected()
{
    ui->plainTextEdit_DXCMonitor->appendPlainText("Disconnected");
    ui->pushButton_DXCDisconnect->setDisabled(true);
    ui->pushButton_DXCConnect->setEnabled(true);
}
void DXC_Options::readyToRead()
{
    DXCSpotInfo info;
    QStringList Spot;
    QString incommingMessage;
    incommingMessage = TCPSocket->readAll();
    ui->plainTextEdit_DXCMonitor->appendPlainText(incommingMessage);
    if(incommingMessage.contains("Please enter your callsign:"))
    {
        TCPSocket->write(ui->lineEdit_DXCUSername->text().append("\r\n").toUtf8());
        ui->plainTextEdit_DXCMonitor->appendPlainText(ui->lineEdit_DXCUSername->text().append("\r\n"));
    }
    else if(incommingMessage.contains("DX de"))
    {
        Spot = incommingMessage.split(" ", QString::SkipEmptyParts);
        info.name = Spot[4].trimmed();
        info.frequency = Spot[3].toDouble() * 1000;
        DXCSpots::Get().add(info);
    }
}
void DXC_Options::saveSettings(QSettings *settings)
{
    if (!settings)
        return;

    settings->beginGroup("dxcluster");

    settings->setValue("DXCAddress", ui->lineEdit_DXCAddress->text());
    settings->setValue("DXCPort", ui->lineEdit_DXCPort->text());
    settings->setValue("DXCUsername", ui->lineEdit_DXCUSername->text());
    settings->setValue("DXCSpotTimeout", ui->lineEdit_DXCSpottimeout->text());

    settings->endGroup();
}

void DXC_Options::readSettings(QSettings *settings)
{
    if (!settings)
        return;

    settings->beginGroup("dxcluster");
    ui->lineEdit_DXCAddress->setText(settings->value("DXCAddress", "localhost").toString());
    ui->lineEdit_DXCPort->setText(settings->value("DXCPort", "7300").toString());
    ui->lineEdit_DXCUSername->setText(settings->value("DXCUsername", "nocall").toString());
    ui->lineEdit_DXCSpottimeout->setText(settings->value("DXCSpotTimeout", "10").toString());

    settings->endGroup();
}
