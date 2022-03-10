#pragma once

#include "./ui_demo_lr0_window.h"
#include <QLabel>
#include <QMainWindow>
#include <QProgressBar>
#include <QTimer>

class DemoLR0AlogrithmWindow : public QMainWindow {
	Q_OBJECT

public:
	DemoLR0AlogrithmWindow(QString code, bool slr);
	~DemoLR0AlogrithmWindow();

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
	Ui::DemoLR0Window *ui;
	QLabel *statusLabel;
	QTimer codeAnalyseTimer;
	QString processId;
	ProcessStatus status;
	QString currentFunction;

private:
	void setProcessBreakpoint(bool withSelectLine = false);
	void clearListItemBackground();
	void setAlogContent(QStringList content);
	void highlightListItem(int line);
	void setupPoint(const ipc::Breakpoint &point);
	void setupPointTranslate(int line);
	void setupPointComputeFirstSet(int line);
	void setupPointComputeFollowSet(int line);
	void setupPointComputeItemClosure(int line);
	void setupPointGenerateAutomaton(int line);
	void appendBreakpoint(QList<ipc::Breakpoint> *breakpoints, int line);

	void appendBreakpointComputeFirstSet(QList<ipc::Breakpoint> *breakpoints,
										 int line);
	void appendBreakpointComputeFollowSet(QList<ipc::Breakpoint> *breakpoints,
										  int line);
	void appendBreakpointComputeItemClosure(QList<ipc::Breakpoint> *breakpoints,
											int line);
};