#include "demo_ll_alogrithm.h"
#include "ipc/base.h"
#include "ipc/ipc.h"
#include <QCloseEvent>
#include <QScrollBar>

DemoLLAlogrithmWindow::DemoLLAlogrithmWindow(QString code)
	: QMainWindow(), ui(new Ui::DemoLLWindow) {
	ui->setupUi(this);
	processId = ipc::LLProcessRequest(code);
	setProcessBreakpoint();
	ipc::LLProcessSwitchMode(processId, ipc::LLProcessModeRun);
	status = Run;
	processCheck();
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
	}
}