#include "demo_lr1_alogrithm.h"
#include "ipc/base.h"
#include "ipc/ipc.h"
#include <QCloseEvent>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QScrollBar>

DemoLR1AlogrithmWindow::DemoLR1AlogrithmWindow(QString code, bool lalr)
	: QMainWindow(), ui(new Ui::DemoLR1Window) {
	ui->setupUi(this);
	statusLabel = new QLabel(ui->statusbar);
	ui->statusbar->addWidget(statusLabel);

	connect(ui->runButton, &QToolButton::clicked, this,
			&DemoLR1AlogrithmWindow::runButtonTrigger);
	connect(ui->stepButton, &QToolButton::clicked, this,
			&DemoLR1AlogrithmWindow::stepButtonTrigger);
	connect(ui->runToCursorButton, &QToolButton::clicked, this,
			&DemoLR1AlogrithmWindow::runToCursorTrigger);

	processId = ipc::LR1ProcessRequest(code, lalr);
	setProcessBreakpoint();
	ipc::LR1ProcessSwitchMode(processId, ipc::ProcessModeRun);
	status = Run;
	processCheck();

	codeAnalyseTimer.start(100);
	connect(&codeAnalyseTimer, &QTimer::timeout, this,
			&DemoLR1AlogrithmWindow::processCheck);
}

DemoLR1AlogrithmWindow::~DemoLR1AlogrithmWindow() {
	delete ui;
	if (!processId.isEmpty()) {
		ipc::LR1ProcessRelease(processId);
	}
}

void DemoLR1AlogrithmWindow::closeEvent(QCloseEvent *event) {
	event->accept();
	deleteLater();
}

void DemoLR1AlogrithmWindow::processCheck() {
	if (status != Run || processId.isEmpty()) {
		return;
	}
	ipc::LR1BreakpointVariables vars;
	ipc::Breakpoint point;
	auto paused = ipc::LR1ProcessGetVariables(processId, &vars, &point);
	if (paused) {
		status = Pause;
		ui->keyWidget->setVariableAndPoint(vars, point);
		setupPoint(point);
		return;
	}
	ipc::LR1ExitResult result;
	auto exit = ipc::LR1ProcessExit(processId, &result);
	if (exit) {
		status = Exit;
		ipc::LR1ProcessRelease(processId);
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
				statusLabel->setText("没有开始符号");
				QMessageBox::information(this, "演示错误", "没有开始符号");
				break;
			case 3:
				statusLabel->setText("项目集闭包冲突");
				QMessageBox::information(this, "演示错误",
										 "项目集闭包冲突，无法生成自动机");
				break;
		}
	}
}

void DemoLR1AlogrithmWindow::setProcessBreakpoint(bool withSelectLine) {
	QList<ipc::Breakpoint> breakpoints;
	const char *names[] = {"Translate", "ComputeFirstSet", "ComputeFollowSet",
						   "ComputeItemClosure", "GenerateAutomaton"};
	for (auto name : names) {
		breakpoints << ipc::Breakpoint{name, 0};
		breakpoints << ipc::Breakpoint{name, -1};
	}
	if (withSelectLine) {
		auto indexes = ui->listWidget->selectionModel()->selectedIndexes();
		for (QModelIndex index : indexes) {
			int i = index.row();
			appendBreakpoint(&breakpoints, i);
		}
	}
	ipc::LR1ProcessSetBreakpoints(processId, breakpoints);
}

void DemoLR1AlogrithmWindow::setupPoint(const ipc::Breakpoint &point) {
	auto name = point.name;
	currentFunction = name;
	if (name == "Translate") {
		setupPointTranslate(point.line);
	} else if (name == "ComputeFirstSet") {
		setupPointComputeFirstSet(point.line);
	} else if (name == "ComputeItemClosure") {
		setupPointComputeItemClosure(point.line);
	} else if (name == "GenerateAutomaton") {
		setupPointGenerateAutomaton(point.line);
	}
}

void DemoLR1AlogrithmWindow::clearListItemBackground() {
	int c = ui->listWidget->count();
	for (int i = 0; i < c; i++) {
		ui->listWidget->item(i)->setBackground(QColor(0, 0, 0, 0));
	}
}

void DemoLR1AlogrithmWindow::highlightListItem(int line) {
	ui->listWidget->item(line)->setBackground(QColor(0x99, 0, 0xff, 0x80));
}

void DemoLR1AlogrithmWindow::setAlogContent(QStringList content) {
	ui->listWidget->clear();
	for (auto item : content) {
		ui->listWidget->addItem(item);
	}
}

void DemoLR1AlogrithmWindow::setupPointTranslate(int line) {
	clearListItemBackground();
	switch (line) {
		case 0:
			{
				ui->step2->setEnabled(true);
				statusLabel->setText("当前算法：转换增广文法");
				QStringList list;
				list << "如果多个产生式左部有开始符号，则引入新的开始符号";
				list << "";
				setAlogContent(list);
				highlightListItem(0);
				ui->keyWidget->translateDefault();
				break;
			}
		case -1:
			highlightListItem(1);
			break;
	}
}

void DemoLR1AlogrithmWindow::setupPointComputeFirstSet(int line) {
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

void DemoLR1AlogrithmWindow::setupPointComputeItemClosure(int line) {
	clearListItemBackground();
	switch (line) {
		case 0:
			{
				ui->step5->setEnabled(true);
				statusLabel->setText("当前算法：计算项目集闭包");
				QStringList list;
				list << "计算开始符号的项目集闭包";
				list << "循环所有项目集闭包";
				list << "    循环当前项目集闭包的项目";
				list << "        计算该项目的下一个项目的项目集闭包";
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
		case -1:
			highlightListItem(4);
			break;
	}
}

void DemoLR1AlogrithmWindow::setupPointGenerateAutomaton(int line) {
	clearListItemBackground();
	switch (line) {
		case 0:
			{
				ui->step6->setEnabled(true);
				statusLabel->setText("当前算法：生成自动机");
				QStringList list;
				list << "根据项目集闭包绘制表格";
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

void DemoLR1AlogrithmWindow::appendBreakpoint(
	QList<ipc::Breakpoint> *breakpoints, int line) {
	if (currentFunction == "ComputeFirstSet") {
		appendBreakpointComputeFirstSet(breakpoints, line);
	} else if (currentFunction == "ComputeItemClosure") {
		appendBreakpointComputeItemClosure(breakpoints, line);
	}
}

void DemoLR1AlogrithmWindow::appendBreakpointComputeFirstSet(
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

void DemoLR1AlogrithmWindow::appendBreakpointComputeItemClosure(
	QList<ipc::Breakpoint> *breakpoints, int line) {
	switch (line) {
		case 1:
			*breakpoints << ipc::Breakpoint{"ComputeItemClosure", 1};
			break;
		case 2:
			*breakpoints << ipc::Breakpoint{"ComputeItemClosure", 2};
			break;
		case 3:
			*breakpoints << ipc::Breakpoint{"ComputeItemClosure", 3};
			break;
		default:
			break;
	}
}

void DemoLR1AlogrithmWindow::runButtonTrigger() {
	if (status != Pause || processId.isEmpty()) {
		return;
	}
	setProcessBreakpoint();
	ipc::LR1ProcessSwitchMode(processId, ipc::ProcessModeRun);
	status = Run;
	processCheck();
}

void DemoLR1AlogrithmWindow::stepButtonTrigger() {
	if (status != Pause || processId.isEmpty()) {
		return;
	}
	ipc::LR1ProcessSwitchMode(processId, ipc::ProcessModePause);
	status = Run;
	processCheck();
}

void DemoLR1AlogrithmWindow::runToCursorTrigger() {
	if (status != Pause || processId.isEmpty()) {
		return;
	}
	setProcessBreakpoint(true);
	ipc::LR1ProcessSwitchMode(processId, ipc::ProcessModeRun);
	status = Run;
	processCheck();
}