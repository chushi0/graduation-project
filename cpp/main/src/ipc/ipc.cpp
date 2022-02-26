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
	ipc::parseArrayString(resp.Data["terminal"].toArray(), &result->terminals);
	ipc::parseArrayString(resp.Data["nonterminal"].toArray(),
						  &result->nonterminals);
	ipc::parseArrayArrayString(resp.Data["productions"].toArray(),
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

QString ipc::LLProcessRequest(QString code, bool withTranslate) {
	QJsonObject data;
	data["code"] = code;
	data["with_translate"] = withTranslate;
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
	wrap["action"] = "ll_process_setbreakpoints";
	wrap["data"] = data;
	auto req = QJsonDocument(wrap).toJson(QJsonDocument::Compact);
	auto resp = RpcRequest(req);
}

bool ipc::LLProcessGetVariables(QString id, LLBreakpointVariables *variables,
								Breakpoint *point) {
	QJsonObject data;
	data["id"] = id;
	QJsonObject wrap;
	wrap["action"] = "ll_process_variables";
	wrap["data"] = data;
	auto req = QJsonDocument(wrap).toJson(QJsonDocument::Compact);
	auto resp = RpcRequest(req);
	if (resp.ResponseCode == 1003) {
		return false;
	}
	ipc::parseLLVariables(resp.Data["var"].toObject(), variables);
	point->name = resp.Data["point"].toObject()["name"].toString();
	point->line = resp.Data["point"].toObject()["line"].toInt();
	return true;
}

bool ipc::LLProcessExit(QString id, LLExitResult *exitResult) {
	QJsonObject data;
	data["id"] = id;
	QJsonObject wrap;
	wrap["action"] = "ll_process_exit";
	wrap["data"] = data;
	auto req = QJsonDocument(wrap).toJson(QJsonDocument::Compact);
	auto resp = RpcRequest(req);
	if (resp.ResponseCode == 1004) {
		return false;
	}
	if (exitResult != nullptr) {
		parseLLExitResult(resp.Data, exitResult);
	}
	return true;
}

QString ipc::LR0ProcessRequest(QString code, bool slr) {
	QJsonObject data;
	data["code"] = code;
	data["slr"] = slr;
	QJsonObject wrap;
	wrap["action"] = "lr0_process_request";
	wrap["data"] = data;
	auto req = QJsonDocument(wrap).toJson(QJsonDocument::Compact);
	auto resp = RpcRequest(req);
	return resp.Data["id"].toString();
}

void ipc::LR0ProcessSwitchMode(QString id, int mode) {
	QJsonObject data;
	data["id"] = id;
	data["mode"] = mode;
	QJsonObject wrap;
	wrap["action"] = "lr0_process_switchmode";
	wrap["data"] = data;
	auto req = QJsonDocument(wrap).toJson(QJsonDocument::Compact);
	auto resp = RpcRequest(req);
}

void ipc::LR0ProcessRelease(QString id) {
	QJsonObject data;
	data["id"] = id;
	QJsonObject wrap;
	wrap["action"] = "lr0_process_release";
	wrap["data"] = data;
	auto req = QJsonDocument(wrap).toJson(QJsonDocument::Compact);
	auto resp = RpcRequest(req);
}

void ipc::LR0ProcessSetBreakpoints(QString id, QList<Breakpoint> breakpoints) {
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
	wrap["action"] = "lr0_process_setbreakpoints";
	wrap["data"] = data;
	auto req = QJsonDocument(wrap).toJson(QJsonDocument::Compact);
	auto resp = RpcRequest(req);
}

bool ipc::LR0ProcessGetVariables(QString id, LR0BreakpointVariables *variables,
								 Breakpoint *point) {
	QJsonObject data;
	data["id"] = id;
	QJsonObject wrap;
	wrap["action"] = "lr0_process_variables";
	wrap["data"] = data;
	auto req = QJsonDocument(wrap).toJson(QJsonDocument::Compact);
	auto resp = RpcRequest(req);
	if (resp.ResponseCode == 1003) {
		return false;
	}
	ipc::parseLR0Variables(resp.Data["var"].toObject(), variables);
	point->name = resp.Data["point"].toObject()["name"].toString();
	point->line = resp.Data["point"].toObject()["line"].toInt();
	return true;
}

bool ipc::LR0ProcessExit(QString id, LR0ExitResult *exitResult) {
	QJsonObject data;
	data["id"] = id;
	QJsonObject wrap;
	wrap["action"] = "lr0_process_exit";
	wrap["data"] = data;
	auto req = QJsonDocument(wrap).toJson(QJsonDocument::Compact);
	auto resp = RpcRequest(req);
	if (resp.ResponseCode == 1004) {
		return false;
	}
	if (exitResult != nullptr) {
		parseLR0ExitResult(resp.Data, exitResult);
	}
	return true;
}