#pragma once

#include "ui_demo_ll_window.h"
#include "widget/DemoWidget.h"
#include <QLabel>
#include <QMainWindow>
#include <QProgressBar>
#include <QTimer>

class DemoLLAlogrithmWindow : public QMainWindow {
	Q_OBJECT

public:
	DemoLLAlogrithmWindow(QString code, bool withTranslate);
	~DemoLLAlogrithmWindow();

protected:
	virtual void closeEvent(QCloseEvent *) override;

private slots:
	void processCheck();

	void runButtonTrigger();
	void stepButtonTrigger();
	void runToCursorTrigger();

private:
	enum ProcessStatus { Run, Pause, Exit };

private:
	Ui::DemoLLWindow *ui;
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
	void setupPointRemoveLeftRecusion(int line);
	void setupPointExtractCommonPrefix(int line);
	void setupPointComputeFirstSet(int line);
	void setupPointComputeFollowSet(int line);
	void setupPointComputeSelectSet(int line);
	void setupPointGenerateAutomaton(int line);
	void appendBreakpoint(QList<ipc::Breakpoint> *breakpoints, int line);
	void appendBreakpointRemoveLeftRecusion(QList<ipc::Breakpoint> *breakpoints,
											int line);
	void
		appendBreakpointExtractCommonPrefix(QList<ipc::Breakpoint> *breakpoints,
											int line);
	void appendBreakpointComputeFirstSet(QList<ipc::Breakpoint> *breakpoints,
										 int line);
	void appendBreakpointComputeFollowSet(QList<ipc::Breakpoint> *breakpoints,
										  int line);
	void appendBreakpointComputeSelectSet(QList<ipc::Breakpoint> *breakpoints,
										  int line);
	void appendBreakpointGenerateAutomaton(QList<ipc::Breakpoint> *breakpoints,
										   int line);
};