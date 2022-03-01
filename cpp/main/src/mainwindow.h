#pragma once

#include "./ui_mainwindow.h"
#include "widget/ClickableLabel.h"
#include <QMainWindow>
#include <QTimer>

class MainWindow : public QMainWindow {
	Q_OBJECT

public:
	MainWindow(QWidget *parent = nullptr);
	~MainWindow();

protected:
	virtual void closeEvent(QCloseEvent *) override;

private slots:
	void codeLineChange();
	void codeChange();
	void actionNewFile();
	void actionAlogLL();
	void actionAlogLLWithoutTranslate();
	void actionAlogLR0();
	void actionAlogSLR();
	void receiveProduction();

private:
	void updateList(QListWidget *listWidget, QStringList items);

private:
	Ui::MainWindow *ui;
	ClickableLabel *statusLabel;
	QTimer codeAnalyseTimer;

	QString parseId;
};
