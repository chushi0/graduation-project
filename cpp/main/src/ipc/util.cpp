#include "util.h"

void ipc::parseJsonArrayToStringList(QJsonArray array, QStringList *list) {
	QStringList result;
	for (auto i : array) {
		result.append(i.toString());
	}
	*list = result;
}

void ipc::parseStringListList(QJsonArray array, QList<QStringList> *list) {
	QList<QStringList> result;
	for (auto i : array) {
		QStringList item;
		ipc::parseJsonArrayToStringList(i.toArray(), &item);
		result.append(item);
	}
	*list = result;
}

void ipc::parseErrors(QJsonArray array, QList<ipc::ErrorType> *list) {
	QList<ipc::ErrorType> result;
	for (auto i : array) {
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

void ipc::parseLLVariables(QJsonObject object, LLBreakpointVariables *out) {
	parseStringListList(object["productions"].toArray(), &out->productions);
	out->loopVariableI = object["loop_variable_i"].toInt();
	out->loopVariableJ = object["loop_variable_j"].toInt();
	out->loopVariableK = object["loop_variable_k"].toInt();
	out->modifiedFlag = object["modified_flag"].toBool();
	parseJsonArrayToStringList(object["nonterminal_orders"].toArray(),
							   &out->nonterminalOrders);
	parseJsonArrayToStringList(object["current_process_production"].toArray(),
							   &out->currentProcessProduction);
	parseStringListList(object["remove_production"].toArray(),
						&out->removeProductions);
	parseStringListList(object["add_production"].toArray(),
						&out->addProductions);
	parseJsonArrayToStringList(object["common_prefix"].toArray(),
							   &out->commonPrefix);
	parseHashStringStringList(object["first"].toObject(), &out->firstSet);
	parseHashStringStringList(object["follow"].toObject(), &out->followSet);
	parseStringListList(object["select"].toArray(), &out->selectSet);
}

void ipc::parseHashStringStringList(QJsonObject object,
									QHash<QString, QStringList> *out) {
	for (auto &k : object.keys()) {
		ipc::parseJsonArrayToStringList(object[k].toArray(), &(*out)[k]);
	}
}