#include "mainwindow.h"
#include "Qsci/qscilexercpp.h"
#include "ipc/base.h"
#include "ipc/ipc.h"
#include <QCloseEvent>
#include <QFontDatabase>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent), ui(new Ui::MainWindow) {
	ui->setupUi(this);

	ui->codeView->SendScintilla(QsciScintilla::SCI_SETCODEPAGE,
								QsciScintilla::SC_CP_UTF8);
	QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
	font.setPointSize(12);
	ui->codeView->setFont(font);
	ui->codeView->setMarginsFont(font);
	ui->codeView->setMarginWidth(0, "00");
	ui->codeView->setMarginLineNumbers(0, true);
	ui->codeView->setWrapMode(QsciScintilla::WrapNone);
	ui->codeView->setEolMode(QsciScintilla::EolUnix);
	ui->codeView->setWhitespaceVisibility(QsciScintilla::WsInvisible);
	ui->codeView->setIndentationsUseTabs(true);
	ui->codeView->setTabWidth(4);
	ui->codeView->setBraceMatching(QsciScintilla::SloppyBraceMatch);
	ui->codeView->setCaretLineVisible(true);

	connect(ui->codeView, &QsciScintilla::linesChanged, this,
			&MainWindow::codeLineChange);

	connect(ui->actionNewFile, &QAction::triggered, this,
			&MainWindow::actionNewFile);
	connect(ui->actionExit, &QAction::triggered, this, &QWidget::close);
}

MainWindow::~MainWindow() {
	delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event) {
	event->accept();
	deleteLater();
}

void MainWindow::codeLineChange() {
	auto lineCount = ui->codeView->lines();
	ui->codeView->setMarginWidth(0, QString("0%1").arg(lineCount));
}

void MainWindow::actionNewFile() {
	auto w = new MainWindow();
	w->show();
	w->activateWindow();
}