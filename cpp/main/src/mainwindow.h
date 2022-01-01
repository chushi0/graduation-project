#pragma once

#include "./ui_mainwindow.h"
#include <QMainWindow>

class MainWindow : public QMainWindow {
	Q_OBJECT

public:
	MainWindow(QWidget *parent = nullptr);
	~MainWindow();

protected:
	virtual void mouseDoubleClickEvent(QMouseEvent *event);

private:
	Ui::MainWindow *ui;
};
