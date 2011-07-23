#include "dockfcdctl.h"
#include "ui_dockfcdctl.h"

DockFcdCtl::DockFcdCtl(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::DockFcdCtl)
{
    ui->setupUi(this);
}

DockFcdCtl::~DockFcdCtl()
{
    delete ui;
}
