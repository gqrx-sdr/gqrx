#ifndef DOCKFCDCTL_H
#define DOCKFCDCTL_H

#include <QDockWidget>

namespace Ui {
    class DockFcdCtl;
}

class DockFcdCtl : public QDockWidget
{
    Q_OBJECT

public:
    explicit DockFcdCtl(QWidget *parent = 0);
    ~DockFcdCtl();

private:
    Ui::DockFcdCtl *ui;
};

#endif // FCDCTL_H
