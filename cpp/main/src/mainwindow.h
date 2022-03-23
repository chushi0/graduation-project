#pragma once

#include "ErrorDialog.h"
#include "ui_mainwindow.h"
#include "widget/ClickableLabel.h"
#include <QMainWindow>
#include <QTimer>

class MainWindow : public QMainWindow {
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = nullptr,
						QString filename = QString());
	~MainWindow();

protected:
	virtual void closeEvent(QCloseEvent *) override;

private slots:
	void codeLineChange();
	void codeChange();
	void codePositionChanged(int line, int index);
	void actionNewFile();
	void actionOpenFile();
	void actionSaveFile();
	void actionAlogLL();
	void actionAlogLLWithoutTranslate();
	void actionAlogLR0();
	void actionAlogSLR();
	void actionAlogLR1();
	void actionAlogLALR();
	void receiveProduction();
	void statusLabelClicked();

private:
	void updateList(QListWidget *listWidget, QStringList items);

private:
	Ui::MainWindow *ui;
	ClickableLabel *statusLabel;
	QLabel *columnLabel;
	ErrorDialog errorDialog;
	QTimer codeAnalyseTimer;

	QString parseId;
};
