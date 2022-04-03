#pragma once

#include "./ui_demo_lr1_window.h"
#include "widget/DemoWidget.h"
#include <QLabel>
#include <QMainWindow>
#include <QProgressBar>
#include <QTimer>

class DemoLR1AlogrithmWindow : public QMainWindow {
	Q_OBJECT

public:
	DemoLR1AlogrithmWindow(QString code, bool lalr);
	~DemoLR1AlogrithmWindow();

protected:
	virtual void closeEvent(QCloseEvent *) override;

private slots:
	void processCheck();

	void runButtonTrigger();
	void stepButtonTrigger();
	void runToCursorTrigger();
	void contextMenu();

private:
	enum ProcessStatus { Run, Pause, Exit };

private:
	Ui::DemoLR1Window *ui;
	QLabel *statusLabel;
	QTimer codeAnalyseTimer;
	QString processId;
	ProcessStatus status;
	QString currentFunction;
	QList<DemoWidget *> demoWidgets;

private:
	void setProcessBreakpoint(bool withSelectLine = false);
	void clearListItemBackground();
	void setAlogContent(QStringList content);
	void highlightListItem(int line);
	void setupPoint(const ipc::Breakpoint &point);
	void setupPointTranslate(int line);
	void setupPointComputeFirstSet(int line);
	void setupPointComputeItemClosure(int line);
	void setupPointGenerateAutomaton(int line);
	void appendBreakpoint(QList<ipc::Breakpoint> *breakpoints, int line);

	void appendBreakpointComputeFirstSet(QList<ipc::Breakpoint> *breakpoints,
										 int line);
	void appendBreakpointComputeItemClosure(QList<ipc::Breakpoint> *breakpoints,
											int line);
};