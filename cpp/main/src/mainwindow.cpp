#include "mainwindow.h"
#include "ErrorDialog.h"
#include "Qsci/qscilexercpp.h"
#include "demo_ll_alogrithm.h"
#include "demo_lr0_alogrithm.h"
#include "demo_lr1_alogrithm.h"
#include "ipc/base.h"
#include "ipc/ipc.h"
#include <QCloseEvent>
#include <QFileDialog>
#include <QFontDatabase>
#include <QMessageBox>
#include <QProcess>

static QString readFileContent(QString filename) {
	QFile file(filename);
	file.open(QIODevice::ReadOnly | QIODevice::Text);
	auto content = file.readAll();
	file.close();
	return QString::fromUtf8(content);
}

static void writeFileContent(QString filename, QString content) {
	QFile file(filename);
	file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text);
	file.write(content.toUtf8());
	file.close();
}

MainWindow::MainWindow(QWidget *parent, QString filename)
	: QMainWindow(parent), ui(new Ui::MainWindow), parseId(""), errorDialog() {
	codeAnalyseTimer.start(100);

	connect(&codeAnalyseTimer, &QTimer::timeout, this,
			&MainWindow::timerUpdate);

	ui->setupUi(this);

	ui->codeView->SendScintilla(QsciScintilla::SCI_SETCODEPAGE,
								QsciScintilla::SC_CP_UTF8);
	QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
	font.setPointSize(12);
	ui->codeView->setFont(font);
	ui->codeView->setMarginsFont(font);
	ui->codeView->setMarginWidth(0, "000");
	ui->codeView->setMarginLineNumbers(0, true);
	ui->codeView->setWrapMode(QsciScintilla::WrapNone);
	ui->codeView->setEolMode(QsciScintilla::EolUnix);
	ui->codeView->setWhitespaceVisibility(QsciScintilla::WsInvisible);
	ui->codeView->setIndentationsUseTabs(false);
	ui->codeView->setTabWidth(4);
	ui->codeView->setCaretLineVisible(true);
	statusLabel = new ClickableLabel(ui->statusbar);
	ui->statusbar->addWidget(statusLabel);
	columnLabel = new QLabel(ui->statusbar);
	ui->statusbar->addPermanentWidget(columnLabel);

	connect(ui->codeView, &QsciScintilla::linesChanged, this,
			&MainWindow::codeLineChange);
	connect(ui->codeView, &QsciScintilla::textChanged, this,
			&MainWindow::codeChange);
	connect(ui->codeView, &QsciScintilla::cursorPositionChanged, this,
			&MainWindow::codePositionChanged);
	connect(statusLabel, &ClickableLabel::clicked, this,
			&MainWindow::statusLabelClicked);

	connect(ui->actionNewFile, &QAction::triggered, this,
			&MainWindow::actionNewFile);
	connect(ui->actionOpenFile, &QAction::triggered, this,
			&MainWindow::actionOpenFile);
	connect(ui->actionSaveFile, &QAction::triggered, this,
			&MainWindow::actionSaveFile);
	connect(ui->actionExit, &QAction::triggered, this, &QWidget::close);
	connect(ui->actionUndo, &QAction::triggered, ui->codeView,
			&QsciScintilla::undo);
	connect(ui->actionRedo, &QAction::triggered, ui->codeView,
			&QsciScintilla::redo);
	connect(ui->actionErrorDialog, &QAction::triggered, this,
			&MainWindow::statusLabelClicked);
	connect(ui->actionCodeLL, &QAction::triggered, this,
			&MainWindow::actionCodeLL);
	connect(ui->actionCodeLLWithoutTranslate, &QAction::triggered, this,
			&MainWindow::actionCodeLLWithoutTranslate);
	connect(ui->actionCodeLR0, &QAction::triggered, this,
			&MainWindow::actionCodeLR0);
	connect(ui->actionCodeSLR, &QAction::triggered, this,
			&MainWindow::actionCodeSLR);
	connect(ui->actionCodeLR1, &QAction::triggered, this,
			&MainWindow::actionCodeLR1);
	connect(ui->actionCodeLALR, &QAction::triggered, this,
			&MainWindow::actionCodeLALR);
	connect(ui->actionLL, &QAction::triggered, this, &MainWindow::actionAlogLL);
	connect(ui->actionLL_2, &QAction::triggered, this,
			&MainWindow::actionAlogLLWithoutTranslate);
	connect(ui->actionLR_0, &QAction::triggered, this,
			&MainWindow::actionAlogLR0);
	connect(ui->actionSLR, &QAction::triggered, this,
			&MainWindow::actionAlogSLR);
	connect(ui->actionLR_1, &QAction::triggered, this,
			&MainWindow::actionAlogLR1);
	connect(ui->actionLALR, &QAction::triggered, this,
			&MainWindow::actionAlogLALR);

	codeChange();
	codePositionChanged(0, 0);
	errorDialog.initView();

	if (!filename.isEmpty()) {
		ui->codeView->setText(readFileContent(filename));
	}
}

MainWindow::~MainWindow() {
	delete ui;
	errorDialog.close();
	if (!parseId.isEmpty()) {
		ipc::ProductionParseCancel(parseId);
	}
	if (!llProcessId.isEmpty()) {
		ipc::LLProcessRelease(llProcessId);
	}
	if (!lr0ProcessId.isEmpty()) {
		ipc::LR0ProcessRelease(lr0ProcessId);
	}
	if (!lr1ProcessId.isEmpty()) {
		ipc::LR1ProcessRelease(lr1ProcessId);
	}
}

void MainWindow::closeEvent(QCloseEvent *event) {
	event->accept();
	deleteLater();
}

void MainWindow::codeLineChange() {
	auto lineCount = ui->codeView->lines();
	if (lineCount < 10) {
		lineCount = 10;
	}
	ui->codeView->setMarginWidth(0, QString("0%1").arg(lineCount));
}

void MainWindow::codeChange() {
	if (!parseId.isEmpty()) {
		ipc::ProductionParseCancel(parseId);
	}
	parseId = ipc::ProductionParseStart(ui->codeView->text());
	statusLabel->setText("???????????????????????????...");
	receiveProduction();
}

void MainWindow::codePositionChanged(int line, int index) {
	int select = ui->codeView->selectedText().length();
	if (select == 0) {
		columnLabel->setText(QString("??? %1, ??? %2").arg(line + 1).arg(index));
	} else {
		columnLabel->setText(QString("??? %1, ??? %2 (????????? %3)")
								 .arg(line + 1)
								 .arg(index)
								 .arg(select));
	}
}

void MainWindow::actionNewFile() {
	auto w = new MainWindow();
	w->show();
	w->activateWindow();
}

void MainWindow::actionOpenFile() {
	QString fileName = QFileDialog::getOpenFileName(this);
	if (fileName.isEmpty()) {
		return;
	}
	if (ui->codeView->text().isEmpty()) {
		ui->codeView->setText(readFileContent(fileName));
		return;
	}
	auto w = new MainWindow(nullptr, fileName);
	w->show();
}

void MainWindow::actionSaveFile() {
	QString fileName = QFileDialog::getSaveFileName(this);
	if (fileName.isEmpty()) {
		return;
	}
	writeFileContent(fileName, ui->codeView->text());
}

void MainWindow::actionCodeLL() {
	if (checkCodeGenerateState()) {
		return;
	}
	QString fileName = QFileDialog::getSaveFileName(this, "??????????????????");
	if (fileName.isEmpty()) {
		return;
	}
	startCodeProcess();
	llProcessId = ipc::LLProcessRequest(ui->codeView->text(), false, fileName);
	ipc::LLProcessSwitchMode(llProcessId, ipc::ProcessModeRun);
	checkLLProcess();
}

void MainWindow::actionCodeLLWithoutTranslate() {
	if (checkCodeGenerateState()) {
		return;
	}
	QString fileName = QFileDialog::getSaveFileName(this, "??????????????????");
	if (fileName.isEmpty()) {
		return;
	}
	startCodeProcess();
	llProcessId = ipc::LLProcessRequest(ui->codeView->text(), true, fileName);
	ipc::LLProcessSwitchMode(llProcessId, ipc::ProcessModeRun);
	checkLLProcess();
}

void MainWindow::actionCodeLR0() {
	if (checkCodeGenerateState()) {
		return;
	}
	QString fileName = QFileDialog::getSaveFileName(this, "??????????????????");
	if (fileName.isEmpty()) {
		return;
	}
	startCodeProcess();
	lr0ProcessId =
		ipc::LR0ProcessRequest(ui->codeView->text(), false, fileName);
	ipc::LR0ProcessSwitchMode(lr0ProcessId, ipc::ProcessModeRun);
	checkLR0Process();
}

void MainWindow::actionCodeSLR() {
	if (checkCodeGenerateState()) {
		return;
	}
	QString fileName = QFileDialog::getSaveFileName(this, "??????????????????");
	if (fileName.isEmpty()) {
		return;
	}
	startCodeProcess();
	lr0ProcessId = ipc::LR0ProcessRequest(ui->codeView->text(), true, fileName);
	ipc::LR0ProcessSwitchMode(lr0ProcessId, ipc::ProcessModeRun);
	checkLR0Process();
}

void MainWindow::actionCodeLR1() {
	if (checkCodeGenerateState()) {
		return;
	}
	QString fileName = QFileDialog::getSaveFileName(this, "??????????????????");
	if (fileName.isEmpty()) {
		return;
	}
	startCodeProcess();
	lr1ProcessId =
		ipc::LR1ProcessRequest(ui->codeView->text(), false, fileName);
	ipc::LR1ProcessSwitchMode(lr1ProcessId, ipc::ProcessModeRun);
	checkLR1Process();
}

void MainWindow::actionCodeLALR() {
	if (checkCodeGenerateState()) {
		return;
	}
	QString fileName = QFileDialog::getSaveFileName(this, "??????????????????");
	if (fileName.isEmpty()) {
		return;
	}
	startCodeProcess();
	lr1ProcessId = ipc::LR1ProcessRequest(ui->codeView->text(), true, fileName);
	ipc::LR1ProcessSwitchMode(lr1ProcessId, ipc::ProcessModeRun);
	checkLR1Process();
}

void MainWindow::actionAlogLL() {
	auto w = new DemoLLAlogrithmWindow(ui->codeView->text(), true);
	w->show();
}

void MainWindow::actionAlogLLWithoutTranslate() {
	auto w = new DemoLLAlogrithmWindow(ui->codeView->text(), false);
	w->show();
}

void MainWindow::actionAlogLR0() {
	auto w = new DemoLR0AlogrithmWindow(ui->codeView->text(), false);
	w->show();
}

void MainWindow::actionAlogSLR() {
	auto w = new DemoLR0AlogrithmWindow(ui->codeView->text(), true);
	w->show();
}

void MainWindow::actionAlogLR1() {
	auto w = new DemoLR1AlogrithmWindow(ui->codeView->text(), false);
	w->show();
}

void MainWindow::actionAlogLALR() {
	auto w = new DemoLR1AlogrithmWindow(ui->codeView->text(), true);
	w->show();
}

void MainWindow::receiveProduction() {
	if (parseId.isEmpty()) {
		return;
	}
	ipc::ProductionResult result;
	bool ok = ipc::ProductionParseQuery(parseId, &result);
	if (!ok) {
		return;
	}
	parseId = "";
	statusLabel->setText(QString("%1 ????????????%2 ?????????")
							 .arg(result.errors.size())
							 .arg(result.warnings.size()));

	updateList(ui->nonterminalList, result.nonterminals);
	updateList(ui->terminalList, result.terminals);
	errorDialog.updateInformation(&result);
}

void MainWindow::timerUpdate() {
	receiveProduction();
	checkLLProcess();
	checkLR0Process();
	checkLR1Process();
}

void MainWindow::startCodeProcess() {
	setEnabled(false);
	statusLabel->setText("??????????????????...");
	if (!parseId.isEmpty()) {
		ipc::ProductionParseCancel(parseId);
		parseId = "";
	}
}

void MainWindow::endCodeProcess() {
	setEnabled(true);
	codeChange();
}

void MainWindow::checkLLProcess() {
	if (llProcessId.isEmpty()) {
		return;
	}
	ipc::LLExitResult result;
	auto exit = ipc::LLProcessExit(llProcessId, &result);
	if (exit) {
		endCodeProcess();
		ipc::LLProcessRelease(llProcessId);
		llProcessId = "";
		switch (result.code) {
			case 0:
				QProcess::startDetached(
					"explorer",
					{"/select,", result.variable.codePath.replace('/', '\\')});
				QMessageBox::information(this, "??????", "??????????????????");
				break;
			case 1:
				QMessageBox::information(this, "????????????",
										 "???????????????????????????");
				break;
			case 2:
				QMessageBox::information(this, "????????????",
										 "Select ????????????????????????????????????");
				break;
		}
	}
}

void MainWindow::checkLR0Process() {
	if (lr0ProcessId.isEmpty()) {
		return;
	}
	ipc::LR0ExitResult result;
	auto exit = ipc::LR0ProcessExit(lr0ProcessId, &result);
	if (exit) {
		endCodeProcess();
		ipc::LR0ProcessRelease(lr0ProcessId);
		lr0ProcessId = "";
		switch (result.code) {
			case 0:
				QProcess::startDetached(
					"explorer",
					{"/select,", result.variable.codePath.replace('/', '\\')});
				QMessageBox::information(this, "??????", "??????????????????");
				break;
			case 1:
				QMessageBox::information(this, "????????????",
										 "???????????????????????????");
				break;
			case 2:
				QMessageBox::information(this, "????????????", "??????????????????");
				break;
			case 3:
				QMessageBox::information(this, "????????????",
										 "?????????????????????????????????????????????");
				break;
		}
	}
}

void MainWindow::checkLR1Process() {
	if (lr1ProcessId.isEmpty()) {
		return;
	}
	ipc::LR1ExitResult result;
	auto exit = ipc::LR1ProcessExit(lr1ProcessId, &result);
	if (exit) {
		endCodeProcess();
		ipc::LR1ProcessRelease(lr1ProcessId);
		lr1ProcessId = "";
		switch (result.code) {
			case 0:
				QProcess::startDetached(
					"explorer",
					{"/select,", result.variable.codePath.replace('/', '\\')});
				QMessageBox::information(this, "??????", "??????????????????");
				break;
			case 1:
				QMessageBox::information(this, "????????????",
										 "???????????????????????????");
				break;
			case 2:
				QMessageBox::information(this, "????????????", "??????????????????");
				break;
			case 3:
				QMessageBox::information(this, "????????????",
										 "?????????????????????????????????????????????");
				break;
		}
	}
}

bool MainWindow::checkCodeGenerateState() {
	return !(llProcessId.isEmpty() && lr0ProcessId.isEmpty() &&
			 lr1ProcessId.isEmpty());
}

void MainWindow::statusLabelClicked() {
	errorDialog.initView();
	errorDialog.show();
	errorDialog.activateWindow();
}

void MainWindow::updateList(QListWidget *listWidget, QStringList items) {
	listWidget->clear();
	listWidget->insertItems(0, items);
}