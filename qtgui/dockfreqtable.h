#ifndef DOCKFREQTABLE_H
#define DOCKFREQTABLE_H

#include <QDockWidget>
#include "qtgui/frequencylisttablemodel.h"

namespace Ui {
    class DockFreqTable;
}

class DockFreqTable : public QDockWidget
{
    Q_OBJECT

private:
    Ui::DockFreqTable *ui; // ui->tableViewFrequencyList
    QString            m_cfg_dir;   /*!< Default config dir, e.g. XDG_CONFIG_HOME. */

public:
    explicit DockFreqTable(const QString& cfg_dir, QWidget *parent = 0);
    ~DockFreqTable();

    FrequencyListTableModel *frequencyListTableModel;

signals:
    void newFrequency(qint64);

public slots:
    void activated(const QModelIndex & index );
    void setNewFrequency(qint64 rx_freq);
};

#endif // DOCKFREQTABLE_H
