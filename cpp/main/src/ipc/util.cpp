#include "util.h"

void ipc::parseJsonArrayToStringList(QJsonArray array, QStringList *list) {
	QStringList result;
	for (auto &i : array) {
		result.append(i.toString());
	}
	*list = result;
}

void ipc::parseProductions(QJsonArray array, QList<QStringList> *list) {
	QList<QStringList> result;
	for (auto &i : array) {
		QStringList item;
		ipc::parseJsonArrayToStringList(i.toArray(), &item);
		result.append(item);
	}
	*list = result;
}

void ipc::parseErrors(QJsonArray array, QList<ipc::ErrorType> *list) {
	QList<ipc::ErrorType> result;
	for (auto &i : array) {
		auto o = i.toObject();
		ipc::ErrorType error;
		error.type = o["type"].toInt();
		error.file = o["file"].toString();
		error.line = o["line"].toInt();
		error.column = o["column"].toInt();
		error.detail = o["detail"].toString();
		error.length = o["length"].toInt();
		result.append(error);
	}
	*list = result;
}