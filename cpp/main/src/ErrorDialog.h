#pragma once

#include "ipc/types.h"
#include "ui_error_dialog.h"
#include <QDialog>

class ErrorDialog : public QDialog {

public:
	ErrorDialog(QWidget *parent = nullptr);
	virtual ~ErrorDialog();

	void updateInformation(ipc::ProductionResult *result);
	void initView();

private:
	Ui::ErrorDialog ui;

	QList<ipc::ErrorType> cacheErrors;
};