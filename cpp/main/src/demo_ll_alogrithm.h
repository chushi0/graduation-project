#pragma once

#include "./ui_demo_ll_alogrithm.h"
#include <QMainWindow>
#include <QProgressBar>

class DemoLLAlogrithmWindow : public QMainWindow {
	Q_OBJECT

public:
	DemoLLAlogrithmWindow(QWidget *parent = nullptr);
	~DemoLLAlogrithmWindow();

protected:
	virtual void closeEvent(QCloseEvent *) override;

private:
	Ui_DemoLLAlogrithmWindow *ui;
	QProgressBar *statusProgress;
};