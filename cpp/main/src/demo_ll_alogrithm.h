#pragma once

#include "./ui_demo_ll_window.h"
#include <QMainWindow>
#include <QProgressBar>

class DemoLLAlogrithmWindow : public QMainWindow {
	Q_OBJECT

public:
	DemoLLAlogrithmWindow(QString code);
	~DemoLLAlogrithmWindow();

protected:
	virtual void closeEvent(QCloseEvent *) override;

private:
	Ui::DemoLLWindow *ui;
};