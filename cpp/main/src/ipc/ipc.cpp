#include "ipc.h"
#include "base.h"
#include "util.h"
#include <QJsonDocument>
#include <QJsonObject>

QString ipc::ProductionParseStart(QString code) {
	QJsonObject data;
	data["code"] = code;
	QJsonObject wrap;
	wrap["action"] = "production_parse_start";
	wrap["data"] = data;
	auto req = QJsonDocument(wrap).toJson();
	auto resp = RpcRequest(req);
	return resp.Data["id"].toString();
}

bool ipc::ProductionParseQuery(QString id, ProductionResult *result) {
	QJsonObject data;
	data["id"] = id;
	QJsonObject wrap;
	wrap["action"] = "production_parse_start";
	wrap["data"] = data;
	auto req = QJsonDocument(wrap).toJson();
	auto resp = RpcRequest(req);
	if (resp.ResponseCode != 0) {
		return false;
	}
	ipc::parseJsonArrayToStringList(resp.Data["terminal"].toArray(),
									&result->terminals);
	ipc::parseJsonArrayToStringList(resp.Data["nonterminal"].toArray(),
									&result->nonterminals);
	ipc::parseProductions(resp.Data["productions"].toArray(),
						  &result->productions);
	ipc::parseErrors(resp.Data["errors"].toObject()["fatal"].toArray(),
					 &result->fatals);
	ipc::parseErrors(resp.Data["errors"].toObject()["error"].toArray(),
					 &result->errors);
	ipc::parseErrors(resp.Data["errors"].toObject()["warning"].toArray(),
					 &result->warnings);
	return true;
}