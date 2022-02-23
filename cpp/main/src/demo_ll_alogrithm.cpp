#include "demo_ll_alogrithm.h"
#include "ipc/base.h"
#include "ipc/ipc.h"
#include <QCloseEvent>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QScrollBar>

DemoLLAlogrithmWindow::DemoLLAlogrithmWindow(QString code, bool withTranslate)
	: QMainWindow(), ui(new Ui::DemoLLWindow) {
	ui->setupUi(this);
	statusLabel = new QLabel(ui->statusbar);
	ui->statusbar->addWidget(statusLabel);

	connect(ui->runButton, &QToolButton::clicked, this,
			&DemoLLAlogrithmWindow::runButtonTrigger);
	connect(ui->stepButton, &QToolButton::clicked, this,
			&DemoLLAlogrithmWindow::stepButtonTrigger);
	connect(ui->runToCursorButton, &QToolButton::clicked, this,
			&DemoLLAlogrithmWindow::runToCursorTrigger);

	processId = ipc::LLProcessRequest(code, withTranslate);
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

void DemoLLAlogrithmWindow::setProcessBreakpoint(bool withSelectLine) {
	QList<ipc::Breakpoint> breakpoints;
	const char *names[] = {"RemoveLeftRecusion", "ExtractCommonPrefix",
						   "ComputeFirstSet",	 "ComputeFollowSet",
						   "ComputeSelectSet",	 "GenerateAutomaton"};
	for (auto name : names) {
		breakpoints << ipc::Breakpoint{name, 0};
		breakpoints << ipc::Breakpoint{name, -1};
	}
	auto indexes = ui->listWidget->selectionModel()->selectedIndexes();
	for (QModelIndex index : indexes) {
		int i = index.row();
		appendBreakpoint(&breakpoints, i);
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
	ipc::LLExitResult result;
	auto exit = ipc::LLProcessExit(processId, &result);
	if (exit) {
		status = Exit;
		ipc::LLProcessRelease(processId);
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
			case 2:
				statusLabel->setText("Select 集合冲突，无法生成自动机");
				point.line = -1;
				point.name = "GenerateAutomaton";
				ui->keyWidget->setVariableAndPoint(result.variable, point);
				setupPoint(point);
				QMessageBox::information(this, "演示错误",
										 "Select 集合冲突，无法生成自动机");
				break;
		}
	}
}

void DemoLLAlogrithmWindow::runButtonTrigger() {
	if (status != Pause || processId.isEmpty()) {
		return;
	}
	setProcessBreakpoint();
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

void DemoLLAlogrithmWindow::runToCursorTrigger() {
	if (status != Pause || processId.isEmpty()) {
		return;
	}
	setProcessBreakpoint(true);
	ipc::LLProcessSwitchMode(processId, ipc::LLProcessModeRun);
	status = Run;
	processCheck();
}

void DemoLLAlogrithmWindow::setupPoint(const ipc::Breakpoint &point) {
	auto name = point.name;
	currentFunction = name;
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
	} else if (name == "GenerateAutomaton") {
		setupPointGenerateAutomaton(point.line);
	}
}

void DemoLLAlogrithmWindow::setupPointRemoveLeftRecusion(int line) {
	clearListItemBackground();
	switch (line) {
		case 0:
			{
				ui->step1->setEnabled(true);
				statusLabel->setText("当前算法：清除左递归");
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
				ui->step2->setEnabled(true);
				statusLabel->setText("当前算法：提取公共前缀");
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
				ui->step3->setEnabled(true);
				statusLabel->setText("当前算法：计算 First 集");
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
				ui->step4->setEnabled(true);
				statusLabel->setText("当前算法：计算 Follow 集");
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
				ui->step5->setEnabled(true);
				statusLabel->setText("当前算法：计算 Select 集");
				QStringList list;
				list << "按照一定顺序排列产生式";
				list << "循环 i 从 1 到 n（产生式数量）：";
				list << "    循环 j 从 1 到 m+1（当前产生式右部符号数量+1）：";
				list << "        若 j == m+1，则将当前产生式左部符号的 Follow "
						"集加入当前产生式的 Select 集，退出内部循环";
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

void DemoLLAlogrithmWindow::setupPointGenerateAutomaton(int line) {
	clearListItemBackground();
	switch (line) {
		case 0:
			{
				ui->step6->setEnabled(true);
				statusLabel->setText("当前算法：生成自动机");
				QStringList list;
				list << "根据 Select 集绘制表格";
				list << "";
				setAlogContent(list);
				highlightListItem(0);
				ui->keyWidget->translateDefault();
				break;
			}
		case -1:
			highlightListItem(1);
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

void DemoLLAlogrithmWindow::appendBreakpoint(
	QList<ipc::Breakpoint> *breakpoints, int line) {
	if (currentFunction == "RemoveLeftRecusion") {
		appendBreakpointRemoveLeftRecusion(breakpoints, line);
	} else if (currentFunction == "ExtractCommonPrefix") {
		appendBreakpointExtractCommonPrefix(breakpoints, line);
	} else if (currentFunction == "ComputeFirstSet") {
		appendBreakpointComputeFirstSet(breakpoints, line);
	} else if (currentFunction == "ComputeFollowSet") {
		appendBreakpointComputeFollowSet(breakpoints, line);
	} else if (currentFunction == "ComputeSelectSet") {
		appendBreakpointComputeSelectSet(breakpoints, line);
	} else if (currentFunction == "GenerateAutomaton") {
		appendBreakpointGenerateAutomaton(breakpoints, line);
	}
}

void DemoLLAlogrithmWindow::appendBreakpointRemoveLeftRecusion(
	QList<ipc::Breakpoint> *breakpoints, int line) {
	switch (line) {
		case 1:
			*breakpoints << ipc::Breakpoint{"RemoveLeftRecusion", 1};
			*breakpoints << ipc::Breakpoint{"RemoveLeftRecusion", 2};
			break;
		case 2:
			*breakpoints << ipc::Breakpoint{"RemoveLeftRecusion", 3};
			break;
		case 3:
			*breakpoints << ipc::Breakpoint{"RemoveLeftRecusion", 4};
			*breakpoints << ipc::Breakpoint{"RemoveLeftRecusion", 5};
			break;
		case 4:
			*breakpoints << ipc::Breakpoint{"RemoveLeftRecusion", 6};
			*breakpoints << ipc::Breakpoint{"RemoveLeftRecusion", 7};
			*breakpoints << ipc::Breakpoint{"RemoveLeftRecusion", 8};
			*breakpoints << ipc::Breakpoint{"RemoveLeftRecusion", 9};
			break;
		default:
			break;
	}
}

void DemoLLAlogrithmWindow::appendBreakpointExtractCommonPrefix(
	QList<ipc::Breakpoint> *breakpoints, int line) {
	switch (line) {
		case 1:
			*breakpoints << ipc::Breakpoint{"ExtractCommonPrefix", 1};
			*breakpoints << ipc::Breakpoint{"ExtractCommonPrefix", 2};
			break;
		case 2:
			*breakpoints << ipc::Breakpoint{"ExtractCommonPrefix", 3};
			*breakpoints << ipc::Breakpoint{"ExtractCommonPrefix", 4};
			break;
		default:
			break;
	}
}

void DemoLLAlogrithmWindow::appendBreakpointComputeFirstSet(
	QList<ipc::Breakpoint> *breakpoints, int line) {
	switch (line) {
		case 1:
			*breakpoints << ipc::Breakpoint{"ComputeFirstSet", 2};
			break;
		case 2:
			*breakpoints << ipc::Breakpoint{"ComputeFirstSet", 3};
			break;
		case 3:
			*breakpoints << ipc::Breakpoint{"ComputeFirstSet", 4};
			break;
		case 4:
			*breakpoints << ipc::Breakpoint{"ComputeFirstSet", 5};
			break;
		case 5:
			*breakpoints << ipc::Breakpoint{"ComputeFirstSet", 6};
			break;
		case 6:
			*breakpoints << ipc::Breakpoint{"ComputeFirstSet", 7};
			break;
		case 7:
			*breakpoints << ipc::Breakpoint{"ComputeFirstSet", 10};
			break;
		default:
			break;
	}
}

void DemoLLAlogrithmWindow::appendBreakpointComputeFollowSet(
	QList<ipc::Breakpoint> *breakpoints, int line) {
	switch (line) {
		case 1:
			*breakpoints << ipc::Breakpoint{"ComputeFollowSet", 1};
			break;
		case 2:
			*breakpoints << ipc::Breakpoint{"ComputeFollowSet", 2};
			*breakpoints << ipc::Breakpoint{"ComputeFollowSet", 4};
			break;
		case 3:
			*breakpoints << ipc::Breakpoint{"ComputeFollowSet", 5};
			break;
		case 4:
			*breakpoints << ipc::Breakpoint{"ComputeFollowSet", 8};
			break;
		case 5:
			*breakpoints << ipc::Breakpoint{"ComputeFollowSet", 6};
			break;
		case 6:
			*breakpoints << ipc::Breakpoint{"ComputeFollowSet", 7};
			break;
		case 7:
			*breakpoints << ipc::Breakpoint{"ComputeFollowSet", 9};
			break;
		case 8:
			*breakpoints << ipc::Breakpoint{"ComputeFollowSet", 10};
			break;
		case 9:
			*breakpoints << ipc::Breakpoint{"ComputeFollowSet", 11};
			*breakpoints << ipc::Breakpoint{"ComputeFollowSet", 12};
			break;
		default:
			break;
	}
}

void DemoLLAlogrithmWindow::appendBreakpointComputeSelectSet(
	QList<ipc::Breakpoint> *breakpoints, int line) {

	switch (line) {
		case 1:
			*breakpoints << ipc::Breakpoint{"ComputeSelectSet", 1};
			break;
		case 2:
			*breakpoints << ipc::Breakpoint{"ComputeSelectSet", 2};
			break;
		case 3:
			*breakpoints << ipc::Breakpoint{"ComputeSelectSet", 3};
			break;
		case 4:
			*breakpoints << ipc::Breakpoint{"ComputeSelectSet", 4};
			break;
		case 5:
			*breakpoints << ipc::Breakpoint{"ComputeSelectSet", 5};
			break;
		case 6:
			*breakpoints << ipc::Breakpoint{"ComputeSelectSet", 6};
			break;
		default:
			break;
	}
}

void DemoLLAlogrithmWindow::appendBreakpointGenerateAutomaton(
	QList<ipc::Breakpoint> *breakpoints, int line) {
}