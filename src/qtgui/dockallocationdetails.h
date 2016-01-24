#ifndef DOCKALLOCATIONDETAILS_H
#define DOCKALLOCATIONDETAILS_H
#include <QDockWidget>
#include <QSettings>
#include <QtWebKitWidgets>



namespace Ui {
    class DockAllocationDetails;
}


class DockAllocationDetails : public QDockWidget
{
    Q_OBJECT

public:
    explicit DockAllocationDetails(QWidget *parent = 0);
    ~DockAllocationDetails();

    void readSettings(QSettings *settings);
    void saveSettings(QSettings *settings);
        
    void setFrequency(qint64 freq_hz);
    
public slots:


private:
    void initRegionsCombo();
    void updateBandView();
    
signals:

private slots:
    void switchcall(const QString& text);

private:
    Ui::DockAllocationDetails *ui;        /*! The Qt designer UI file. */
    
    qint64 lf_freq_hz;  /** Lower frequency being displayed */
    qint64 uf_freq_hz;  /** Upper frequency being displayed */
};

#endif // DOCKALLOCATIONDETAILS_H
