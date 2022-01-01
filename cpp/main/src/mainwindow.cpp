#include "mainwindow.h"
#include "rpc/base.h"
#include "rpc/rpc.h"
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent), ui(new Ui::MainWindow) {
	ui->setupUi(this);
}

MainWindow::~MainWindow() {
	delete ui;
}

void MainWindow::mouseDoubleClickEvent(QMouseEvent *event) {
	rpc::SendLogMessage("user click");
	auto msg = rpc::Hello();
	QMessageBox::information(this, "rpc::Hello", msg);
}