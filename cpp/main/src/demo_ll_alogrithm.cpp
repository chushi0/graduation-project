#include "demo_ll_alogrithm.h"
#include "ipc/base.h"
#include "ipc/ipc.h"
#include <QCloseEvent>
#include <QListWidgetItem>
#include <QScrollBar>

DemoLLAlogrithmWindow::DemoLLAlogrithmWindow(QString code)
	: QMainWindow(), ui(new Ui::DemoLLWindow) {
	ui->setupUi(this);
	processId = ipc::LLProcessRequest(code);
	setProcessBreakpoint();
	ipc::LLProcessSwitchMode(processId, ipc::LLProcessModeRun);
	status = Run;
	processCheck();

	connect(ui->runButton, &QToolButton::clicked, this,
			&DemoLLAlogrithmWindow::runButtonTrigger);
	connect(ui->stepButton, &QToolButton::clicked, this,
			&DemoLLAlogrithmWindow::stepButtonTrigger);

	codeAnalyseTimer.start(100);
	connect(&codeAnalyseTimer, &QTimer::timeout, this,
			&DemoLLAlogrithmWindow::processCheck);
}

DemoLLAlogrithmWindow::~DemoLLAlogrithmWindow() {
	delete ui;
	if (!processId.isEmpty()) {
		ipc::LLProcessRelease(processId);
	}
}

void DemoLLAlogrithmWindow::closeEvent(QCloseEvent *event) {
	event->accept();
	deleteLater();
}

void DemoLLAlogrithmWindow::setProcessBreakpoint() {
	QList<ipc::Breakpoint> breakpoints;
	const char *names[] = {"RemoveLeftRecusion", "ExtractCommonPrefix",
						   "ComputeFirstSet", "ComputeFollowSet",
						   "ComputeSelectSet"};
	for (auto name : names) {
		breakpoints << ipc::Breakpoint{name, 0};
		breakpoints << ipc::Breakpoint{name, -1};
	}
	ipc::LLProcessSetBreakpoints(processId, breakpoints);
}

void DemoLLAlogrithmWindow::processCheck() {
	if (status != Run || processId.isEmpty()) {
		return;
	}
	ipc::LLBreakpointVariables vars;
	ipc::Breakpoint point;
	auto paused = ipc::LLProcessGetVariables(processId, &vars, &point);
	if (paused) {
		status = Pause;
		ui->keyWidget->setVariableAndPoint(vars, point);
		setupPoint(point);
		return;
	}
	auto exit = ipc::LLProcessExit(processId);
	if (exit) {
		status = Exit;
		ipc::LLProcessRelease(processId);
		processId = "";
	}
}

void DemoLLAlogrithmWindow::runButtonTrigger() {
	if (status != Pause || processId.isEmpty()) {
		return;
	}
	ipc::LLProcessSwitchMode(processId, ipc::LLProcessModeRun);
	status = Run;
}

void DemoLLAlogrithmWindow::stepButtonTrigger() {
	if (status != Pause || processId.isEmpty()) {
		return;
	}
	ipc::LLProcessSwitchMode(processId, ipc::LLProcessModePause);
	status = Run;
	processCheck();
}

void DemoLLAlogrithmWindow::setupPoint(const ipc::Breakpoint &point) {
	auto name = point.name;
	if (name == "RemoveLeftRecusion") {
		setupPointRemoveLeftRecusicn(point.line);
	}
}

void DemoLLAlogrithmWindow::setupPointRemoveLeftRecusicn(int line) {
	clearListItemBackground();
	switch (line) {
		case 0:
			{
				QStringList list;
				list << "按照某顺序排列非终结符";
				list << "循环 i 从 1 到 n";
				list << "    循环 j 从 1 到 i-1";
				list << "        将 A[i] -> A[j]r 替换为 A[i] -> Br（A[j] -> "
						"B）";
				list << "    消除 A[i] 中的立即左递归";
				list << "";
				setAlogContent(list);
				highlightListItem(0);
				break;
			}
		case 1:
		case 2:
			highlightListItem(1);
			break;
		case 3:
			highlightListItem(2);
			break;
		case 4:
		case 5:
			highlightListItem(3);
			break;
		case 6:
		case 7:
		case 8:
			highlightListItem(4);
			break;
		case 9:
		case -1:
			highlightListItem(5);
			break;
	}
}

void DemoLLAlogrithmWindow::clearListItemBackground() {
	int c = ui->listWidget->count();
	for (int i = 0; i < c; i++) {
		ui->listWidget->item(i)->setBackground(QColor(0, 0, 0, 0));
	}
}

void DemoLLAlogrithmWindow::highlightListItem(int line) {
	ui->listWidget->item(line)->setBackground(QColor(0x99, 0, 0xff, 0x80));
}

void DemoLLAlogrithmWindow::setAlogContent(QStringList content) {
	ui->listWidget->clear();
	for (auto item : content) {
		ui->listWidget->addItem(item);
	}
}