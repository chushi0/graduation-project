#include "demo_ll_alogrithm.h"
#include <QCloseEvent>
#include <QScrollBar>

DemoLLAlogrithmWindow::DemoLLAlogrithmWindow(QString code)
	: QMainWindow(), ui(new Ui::DemoLLWindow) {
	ui->setupUi(this);
}

DemoLLAlogrithmWindow::~DemoLLAlogrithmWindow() {
	delete ui;
}

void DemoLLAlogrithmWindow::closeEvent(QCloseEvent *event) {
	event->accept();
	deleteLater();
}
