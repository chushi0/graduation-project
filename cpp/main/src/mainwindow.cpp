#include "mainwindow.h"
#include "ipc/base.h"
#include "ipc/ipc.h"
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent), ui(new Ui::MainWindow) {
	ui->setupUi(this);
}

MainWindow::~MainWindow() {
	delete ui;
}

void MainWindow::mouseDoubleClickEvent(QMouseEvent *event) {
	ipc::SendLogMessage("user click");
	auto msg = ipc::Hello();
	QMessageBox::information(this, "ipc::Hello", msg);
}