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
	auto req = QJsonDocument(wrap).toJson(QJsonDocument::Compact);
	auto resp = RpcRequest(req);
	return resp.Data["id"].toString();
}

bool ipc::ProductionParseQuery(QString id, ProductionResult *result) {
	QJsonObject data;
	data["id"] = id;
	QJsonObject wrap;
	wrap["action"] = "production_parse_query";
	wrap["data"] = data;
	auto req = QJsonDocument(wrap).toJson(QJsonDocument::Compact);
	auto resp = RpcRequest(req);
	if (resp.ResponseCode != 0) {
		return false;
	}
	ipc::parseJsonArrayToStringList(resp.Data["terminal"].toArray(),
									&result->terminals);
	ipc::parseJsonArrayToStringList(resp.Data["nonterminal"].toArray(),
									&result->nonterminals);
	ipc::parseStringListList(resp.Data["productions"].toArray(),
							 &result->productions);
	ipc::parseErrors(resp.Data["errors"].toObject()["fatal"].toArray(),
					 &result->fatals);
	ipc::parseErrors(resp.Data["errors"].toObject()["error"].toArray(),
					 &result->errors);
	ipc::parseErrors(resp.Data["errors"].toObject()["warning"].toArray(),
					 &result->warnings);
	return true;
}

void ipc::ProductionParseCancel(QString id) {
	QJsonObject data;
	data["id"] = id;
	QJsonObject wrap;
	wrap["action"] = "production_parse_cancel";
	wrap["data"] = data;
	auto req = QJsonDocument(wrap).toJson(QJsonDocument::Compact);
	auto resp = RpcRequest(req);
}

QString ipc::LLProcessRequest(QString code) {
	QJsonObject data;
	data["code"] = code;
	QJsonObject wrap;
	wrap["action"] = "ll_process_request";
	wrap["data"] = data;
	auto req = QJsonDocument(wrap).toJson(QJsonDocument::Compact);
	auto resp = RpcRequest(req);
	return resp.Data["id"].toString();
}

void ipc::LLProcessSwitchMode(QString id, int mode) {
	QJsonObject data;
	data["id"] = id;
	data["mode"] = mode;
	QJsonObject wrap;
	wrap["action"] = "ll_process_switchmode";
	wrap["data"] = data;
	auto req = QJsonDocument(wrap).toJson(QJsonDocument::Compact);
	auto resp = RpcRequest(req);
}

void ipc::LLProcessRelease(QString id) {
	QJsonObject data;
	data["id"] = id;
	QJsonObject wrap;
	wrap["action"] = "ll_process_release";
	wrap["data"] = data;
	auto req = QJsonDocument(wrap).toJson(QJsonDocument::Compact);
	auto resp = RpcRequest(req);
}

void ipc::LLProcessSetBreakpoints(QString id, QList<Breakpoint> breakpoints) {
	QJsonArray array;
	for (auto &breakpoint : breakpoints) {
		QJsonObject o;
		o["name"] = breakpoint.name;
		o["line"] = breakpoint.line;
		array.append(o);
	}
	QJsonObject data;
	data["id"] = id;
	data["breakpoints"] = array;
	QJsonObject wrap;
	wrap["action"] = "ll_process_release";
	wrap["data"] = data;
	auto req = QJsonDocument(wrap).toJson(QJsonDocument::Compact);
	auto resp = RpcRequest(req);
}