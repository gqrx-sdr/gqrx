#ifndef FREQUENCYLISTTABLEMODEL_H
#define FREQUENCYLISTTABLEMODEL_H

#include <QAbstractTableModel>

class FrequencyListTableModel : public QAbstractTableModel
{
    Q_OBJECT

    struct Row
    {
        static const int iNumColumns = 4;

        qint64  frequency;
        QString name;
        QString modulation;
        qint64  bandwidth;

        Row()
        {
            this->frequency = 0;
            this->bandwidth = 0;
        }
        Row( qint64 frequency, QString name, qint64 bandwidth, QString modulation )
        {
            this->frequency = frequency;
            this->name = name;
            this->modulation = modulation;
            this->bandwidth = bandwidth;
        }
    };
    QList<Row> table;

public:
    explicit FrequencyListTableModel(QObject *parent = 0);
    
    bool load(QString configDir, QString filename);

    int rowCount ( const QModelIndex & parent = QModelIndex() ) const;

    int columnCount ( const QModelIndex & parent = QModelIndex() ) const;

    QVariant headerData ( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;

    QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;

signals:
    void newFrequency(qint64);

public slots:
    void activated(const QModelIndex & index );
};

#endif // FREQUENCYLISTTABLEMODEL_H
