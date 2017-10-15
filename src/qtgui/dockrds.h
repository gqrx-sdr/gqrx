#ifndef DOCKRDS_H
#define DOCKRDS_H
#include <QDockWidget>
#include <QSharedPointer>
#include <QSettings>
#include <QTextCodec>

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
    void updateRDS(std::string text, int type);
    void showEnabled();
    void showDisabled();
    void setEnabled();
    void setDisabled();
    void initCodecs();

private:
    void ClearTextFields();

signals:
    void rdsDecoderToggled(bool);

private slots:
    void on_rdsCheckbox_toggled(bool checked);

    void on_textCodecComboBox_currentIndexChanged(const QString &arg1);

private:
    Ui::DockRDS *ui;        /*! The Qt designer UI file. */
    QSharedPointer<QTextDecoder> decoder;
    QHash<QString,QString> allCodecs;
};

#endif // DOCKRDS_H
