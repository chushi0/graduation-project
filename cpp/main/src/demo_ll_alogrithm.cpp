#include "demo_ll_alogrithm.h"
#include <QCloseEvent>

DemoLLAlogrithmWindow::DemoLLAlogrithmWindow(QWidget *parent)
	: QMainWindow(parent), ui(new Ui_DemoLLAlogrithmWindow) {
	ui->setupUi(this);

	statusProgress = new QProgressBar(this);
	statusProgress->setMaximum(0);
	ui->statusBar->addPermanentWidget(statusProgress);
	ui->statusBar->showMessage("正在解析代码...");
}

DemoLLAlogrithmWindow::~DemoLLAlogrithmWindow() {
	delete ui;
}

void DemoLLAlogrithmWindow::closeEvent(QCloseEvent *event) {
	event->accept();
	deleteLater();
}
