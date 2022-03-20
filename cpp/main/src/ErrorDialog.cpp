#include "ErrorDialog.h"

static QString titleFromType(int type) {
	switch (type) {
		case 10001:
			return "文件读取错误";
		case 20001:
			return "不支持的字符";
		case 20002:
			return "缺少非终结符";
		case 20003:
			return "无效语句";
		case 20004:
			return "缺少生成标记";
		case 20005:
			return "过多的生成标记";
		case 20006:
			return "过多的开始符号定义";
		case 30001:
			return "缺少开始符号";
		case 30002:
			return "不推荐的非终结符";
		case 30003:
			return "未声明开始符号";
		case 30004:
			return "重复产生式";
	}
	return "未知错误";
}

ErrorDialog::ErrorDialog(QWidget *parent) : QDialog(parent) {
	ui.setupUi(this);
}

ErrorDialog::~ErrorDialog() {
}

void ErrorDialog::updateInformation(ipc::ProductionResult *result) {
	cacheErrors.clear();
	cacheErrors.append(result->fatals);
	cacheErrors.append(result->errors);
	cacheErrors.append(result->warnings);

	ui.tableWidget->setRowCount(cacheErrors.size());
	for (int i = 0; i < cacheErrors.size(); i++) {
		auto &error = cacheErrors[i];
		switch (error.type / 10000) {
			case 1:
				ui.tableWidget->setItem(i, 0, new QTableWidgetItem("严重错误"));
				break;
			case 2:
				ui.tableWidget->setItem(i, 0, new QTableWidgetItem("错误"));
				break;
			case 3:
				ui.tableWidget->setItem(i, 0, new QTableWidgetItem("警告"));
				break;
		}
		ui.tableWidget->setItem(
			i, 1, new QTableWidgetItem(QString("%1").arg(error.type)));
		ui.tableWidget->setItem(
			i, 2, new QTableWidgetItem(titleFromType(error.type)));
		ui.tableWidget->setItem(
			i, 3,
			new QTableWidgetItem(
				QString("%1:%2").arg(error.line).arg(error.column)));
		ui.tableWidget->setItem(i, 4, new QTableWidgetItem(error.detail));
	}
}