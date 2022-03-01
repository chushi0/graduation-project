#include "demo_lr0_alogrithm.h"
#include "ipc/base.h"
#include "ipc/ipc.h"
#include <QCloseEvent>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QScrollBar>

DemoLR0AlogrithmWindow::DemoLR0AlogrithmWindow(QString code, bool slr)
	: QMainWindow(), ui(new Ui::DemoLR0Window) {
	ui->setupUi(this);
	statusLabel = new QLabel(ui->statusbar);
	ui->statusbar->addWidget(statusLabel);

	connect(ui->runButton, &QToolButton::clicked, this,
			&DemoLR0AlogrithmWindow::runButtonTrigger);
	connect(ui->stepButton, &QToolButton::clicked, this,
			&DemoLR0AlogrithmWindow::stepButtonTrigger);
	connect(ui->runToCursorButton, &QToolButton::clicked, this,
			&DemoLR0AlogrithmWindow::runToCursorTrigger);

	processId = ipc::LR0ProcessRequest(code, slr);
	setProcessBreakpoint();
	ipc::LR0ProcessSwitchMode(processId, ipc::LLProcessModeRun);
	status = Run;
	processCheck();

	codeAnalyseTimer.start(100);
	connect(&codeAnalyseTimer, &QTimer::timeout, this,
			&DemoLR0AlogrithmWindow::processCheck);
}

DemoLR0AlogrithmWindow::~DemoLR0AlogrithmWindow() {
	delete ui;
	if (!processId.isEmpty()) {
		ipc::LR0ProcessRelease(processId);
	}
}

void DemoLR0AlogrithmWindow::closeEvent(QCloseEvent *event) {
	event->accept();
	deleteLater();
}

void DemoLR0AlogrithmWindow::processCheck() {
	if (status != Run || processId.isEmpty()) {
		return;
	}
	ipc::LR0BreakpointVariables vars;
	ipc::Breakpoint point;
	auto paused = ipc::LR0ProcessGetVariables(processId, &vars, &point);
	if (paused) {
		status = Pause;
		ui->keyWidget->setVariableAndPoint(vars, point);
		setupPoint(point);
		return;
	}
	ipc::LR0ExitResult result;
	auto exit = ipc::LR0ProcessExit(processId, &result);
	if (exit) {
		status = Exit;
		ipc::LR0ProcessRelease(processId);
		processId = "";
		switch (result.code) {
			case 0:
				statusLabel->setText("算法演示完成");
				QMessageBox::information(this, "演示完成", "算法已演示完成");
				break;
			case 1:
				statusLabel->setText("产生式代码解析错误");
				QMessageBox::information(this, "演示错误",
										 "产生式代码解析错误");
				close();
				break;
		}
	}
}

void DemoLR0AlogrithmWindow::setProcessBreakpoint(bool withSelectLine) {
	QList<ipc::Breakpoint> breakpoints;
	const char *names[] = {"ComputeFirstSet", "ComputeFollowSet",
						   "ComputeItemClosure", "GenerateAutomaton"};
	for (auto name : names) {
		breakpoints << ipc::Breakpoint{name, 0};
		breakpoints << ipc::Breakpoint{name, -1};
	}
	auto indexes = ui->listWidget->selectionModel()->selectedIndexes();
	for (QModelIndex index : indexes) {
		int i = index.row();
		appendBreakpoint(&breakpoints, i);
	}
	ipc::LR0ProcessSetBreakpoints(processId, breakpoints);
}

void DemoLR0AlogrithmWindow::setupPoint(const ipc::Breakpoint &point) {
}

void DemoLR0AlogrithmWindow::appendBreakpoint(
	QList<ipc::Breakpoint> *breakpoints, int line) {
}

void DemoLR0AlogrithmWindow::runButtonTrigger() {
	if (status != Pause || processId.isEmpty()) {
		return;
	}
	setProcessBreakpoint();
	ipc::LR0ProcessSwitchMode(processId, ipc::LLProcessModeRun);
	status = Run;
	processCheck();
}

void DemoLR0AlogrithmWindow::stepButtonTrigger() {
	if (status != Pause || processId.isEmpty()) {
		return;
	}
	ipc::LR0ProcessSwitchMode(processId, ipc::LLProcessModePause);
	status = Run;
	processCheck();
}

void DemoLR0AlogrithmWindow::runToCursorTrigger() {
	if (status != Pause || processId.isEmpty()) {
		return;
	}
	setProcessBreakpoint(true);
	ipc::LR0ProcessSwitchMode(processId, ipc::LLProcessModeRun);
	status = Run;
	processCheck();
}