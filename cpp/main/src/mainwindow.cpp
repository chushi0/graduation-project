#include "mainwindow.h"
#include "ErrorDialog.h"
#include "Qsci/qscilexercpp.h"
#include "demo_ll_alogrithm.h"
#include "demo_lr0_alogrithm.h"
#include "demo_lr1_alogrithm.h"
#include "ipc/base.h"
#include "ipc/ipc.h"
#include <QCloseEvent>
#include <QFontDatabase>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent), ui(new Ui::MainWindow), parseId(""), errorDialog() {
	codeAnalyseTimer.start(100);

	connect(&codeAnalyseTimer, &QTimer::timeout, this,
			&MainWindow::receiveProduction);

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
	connect(ui->actionExit, &QAction::triggered, this, &QWidget::close);
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
}

MainWindow::~MainWindow() {
	delete ui;
	if (!parseId.isEmpty()) {
		ipc::ProductionParseCancel(parseId);
	}
	exit(0);
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
	statusLabel->setText("正在解析产生式代码...");
	receiveProduction();
}

void MainWindow::codePositionChanged(int line, int index) {
	int select = ui->codeView->selectedText().length();
	if (select == 0) {
		columnLabel->setText(QString("行 %1, 列 %2").arg(line + 1).arg(index));
	} else {
		columnLabel->setText(QString("行 %1, 列 %2 (已选择 %3)")
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
	statusLabel->setText(QString("%1 个错误，%2 个警告")
							 .arg(result.errors.size())
							 .arg(result.warnings.size()));

	updateList(ui->nonterminalList, result.nonterminals);
	updateList(ui->terminalList, result.terminals);
	errorDialog.updateInformation(&result);
}

void MainWindow::statusLabelClicked() {
	errorDialog.initView();
	errorDialog.show();
}

void MainWindow::updateList(QListWidget *listWidget, QStringList items) {
	listWidget->clear();
	listWidget->insertItems(0, items);
}