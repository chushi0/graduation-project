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
		// breakpoints << ipc::Breakpoint{name, -1};
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
		setupPointRemoveLeftRecusion(point.line);
	} else if (name == "ExtractCommonPrefix") {
		setupPointExtractCommonPrefix(point.line);
	} else if (name == "ComputeFirstSet") {
		setupPointComputeFirstSet(point.line);
	} else if (name == "ComputeFollowSet") {
		setupPointComputeFollowSet(point.line);
	} else if (name == "ComputeSelectSet") {
		setupPointComputeSelectSet(point.line);
	}
}

void DemoLLAlogrithmWindow::setupPointRemoveLeftRecusion(int line) {
	clearListItemBackground();
	switch (line) {
		case 0:
			{
				QStringList list;
				list << "按照某顺序排列非终结符";
				list << "循环 i 从 1 到 n（非终结符数量）：";
				list << "    循环 j 从 1 到 i-1：";
				list << "        将 A[i] -> A[j]r 替换为 A[i] -> Br（A[j] -> "
						"B）";
				list << "    消除 A[i] 中的立即左递归";
				list << "";
				setAlogContent(list);
				highlightListItem(0);
				ui->keyWidget->translateDefault();
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
		case 9:
			highlightListItem(4);
			break;
		case -1:
			highlightListItem(5);
			break;
	}
}

void DemoLLAlogrithmWindow::setupPointExtractCommonPrefix(int line) {
	clearListItemBackground();
	switch (line) {
		case 0:
			{
				QStringList list;
				list << "按照一定顺序排列非终结符";
				list << "循环 i 从 1 到 n（非终结符数量）：";
				list << "    循环提取最短的公共前缀";
				list << "";
				setAlogContent(list);
				highlightListItem(0);
				ui->keyWidget->translateDefault();
				break;
			}
		case 1:
		case 2:
			highlightListItem(1);
			break;
		case 3:
		case 4:
			highlightListItem(2);
			break;
		case -1:
			highlightListItem(3);
			break;
	}
}

void DemoLLAlogrithmWindow::setupPointComputeFirstSet(int line) {
	clearListItemBackground();
	switch (line) {
		case 0:
			{
				QStringList list;
				list << "按照一定顺序排列产生式";
				list << "循环 i 从 1 到 n（产生式数量）：";
				list << "    循环 j 从 1 到 m（当前产生式右部符号数量）：";
				list << "        如果产生式第 j 个符号是终结符：";
				list << "            将该终结符加入 First 集，跳出内部循环";
				list << "        如果产生式第 j 个符号是非终结符：";
				list << "            将该非终结符的 First 集中空以外的内容加入 "
						"First 集；如果该非终结符的 First "
						"集不包含空，跳出内部循环";
				list << "如果有任何非终结符的First集有更新，则重新执行以上内容";
				list << "";
				setAlogContent(list);
				highlightListItem(0);
				ui->keyWidget->translateDefault();
				break;
			}
		case 2:
			highlightListItem(1);
			break;
		case 3:
			highlightListItem(2);
			break;
		case 4:
			highlightListItem(3);
			break;
		case 5:
			highlightListItem(4);
			break;
		case 6:
			highlightListItem(5);
			break;
		case 7:
			highlightListItem(6);
			break;
		case 10:
			highlightListItem(7);
			break;
		case -1:
			highlightListItem(8);
			break;
	}
}

void DemoLLAlogrithmWindow::setupPointComputeFollowSet(int line) {
	clearListItemBackground();
	switch (line) {
		case 0:
			{
				QStringList list;
				list << "按照一定顺序排列产生式";
				list << "将结束符加入开始符号的Follow集";
				list << "循环 i 从 1 到 n（产生式数量）：";
				list << "    循环 j 从 1 到 m（当前产生式右部符号数量）：";
				list << "        如果产生式第 j "
						"个符号是终结符，进入下一次循环（continue）";
				list << "        循环 k 从 j+1 到 m+1：";
				list << "           如果 k == m+1，则将产生式左部符号的 Follow "
						"集加入第j个符号的 Follow 集，跳出循环";
				list << "           将产生式第 k "
						"个符号的 First 集中空以外的符号加入第 j "
						"个符号的 Follow 集";
				list << "           如果第 k 个符号的 First "
						"集中不包含空，则退出内部循环";
				list
					<< "如果有任何非终结符的Follow集有更新，则重新执行以上内容";
				list << "";
				setAlogContent(list);
				highlightListItem(0);
				ui->keyWidget->translateDefault();
				break;
			}
		case 1:
			highlightListItem(1);
			break;
		case 2:
		case 4:
			highlightListItem(2);
			break;
		case 5:
			highlightListItem(3);
			break;
		case 8:
			highlightListItem(4);
			break;
		case 6:
			highlightListItem(5);
			break;
		case 7:
			highlightListItem(6);
			break;
		case 9:
			highlightListItem(7);
			break;
		case 10:
			highlightListItem(8);
			break;
		case 11:
		case 12:
			highlightListItem(9);
			break;
		case -1:
			highlightListItem(10);
			break;
		default:
			break;
	}
}

void DemoLLAlogrithmWindow::setupPointComputeSelectSet(int line) {
	clearListItemBackground();
	switch (line) {
		case 0:
			{
				QStringList list;
				list << "按照一定顺序排列产生式";
				list << "循环 i 从 1 到 n（产生式数量）：";
				list << "    循环 j 从 1 到 m+1（当前产生式右部符号数量+1）：";
				list << "        若 j == m+1，则将当前产生式左部符号的 Follow "
						"集加入当前产生式的Select集，退出内部循环";
				list << "        将当前符号的 First "
						"集除空以外的终结符加入当前产生式的 Select 集";
				list << "        若当前符号的 First 集不包含空，退出内部循环";
				list << "";
				setAlogContent(list);
				highlightListItem(0);
				ui->keyWidget->translateDefault();
				break;
			}
		case 1:
			highlightListItem(1);
			break;
		case 2:
			highlightListItem(2);
			break;
		case 3:
			highlightListItem(3);
			break;
		case 4:
			highlightListItem(4);
			break;
		case 5:
			highlightListItem(5);
			break;
		case 6:
		case -1:
			highlightListItem(6);
			break;
		default:
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