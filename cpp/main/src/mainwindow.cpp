#include "mainwindow.h"
#include "Qsci/qscilexercpp.h"
#include "demo_ll_alogrithm.h"
#include "ipc/base.h"
#include "ipc/ipc.h"
#include <QCloseEvent>
#include <QFontDatabase>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent), ui(new Ui::MainWindow), parseId("") {
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

	connect(ui->codeView, &QsciScintilla::linesChanged, this,
			&MainWindow::codeLineChange);
	connect(ui->codeView, &QsciScintilla::textChanged, this,
			&MainWindow::codeChange);

	connect(ui->actionNewFile, &QAction::triggered, this,
			&MainWindow::actionNewFile);
	connect(ui->actionExit, &QAction::triggered, this, &QWidget::close);
	connect(ui->actionLL, &QAction::triggered, this, &MainWindow::actionAlogLL);
	connect(ui->actionLL_2, &QAction::triggered, this,
			&MainWindow::actionAlogLLWithoutTranslate);
}

MainWindow::~MainWindow() {
	delete ui;
	if (!parseId.isEmpty()) {
		ipc::ProductionParseCancel(parseId);
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
	ui->statusbar->showMessage("正在解析产生式代码...");
	receiveProduction();
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
	ui->statusbar->showMessage(QString("%1 个错误，%2 个警告")
								   .arg(result.errors.size())
								   .arg(result.warnings.size()));

	updateList(ui->nonterminalList, result.nonterminals);
	updateList(ui->terminalList, result.terminals);
}

void MainWindow::updateList(QListWidget *listWidget, QStringList items) {
	listWidget->clear();
	listWidget->insertItems(0, items);
}