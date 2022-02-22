#pragma once

#include "./ui_demo_ll_window.h"
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

private:
	enum ProcessStatus { Run, Pause, Exit };

private:
	Ui::DemoLLWindow *ui;
	QTimer codeAnalyseTimer;
	QString processId;
	ProcessStatus status;

private:
	void setProcessBreakpoint();
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
};