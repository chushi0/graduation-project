#pragma once

#include "./ui_demo_ll_window.h"
#include <QMainWindow>
#include <QProgressBar>
#include <QTimer>

class DemoLLAlogrithmWindow : public QMainWindow {
	Q_OBJECT

public:
	DemoLLAlogrithmWindow(QString code);
	~DemoLLAlogrithmWindow();

protected:
	virtual void closeEvent(QCloseEvent *) override;

private slots:
	void processCheck();

private:
	enum ProcessStatus { Run, Pause, Exit };

private:
	Ui::DemoLLWindow *ui;
	QTimer codeAnalyseTimer;
	QString processId;
	ProcessStatus status;

private:
	void setProcessBreakpoint();
};