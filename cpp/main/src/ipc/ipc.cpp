#include "ipc.h"
#include "base.h"
#include <QJsonDocument>
#include <QJsonObject>

QString ipc::Hello() {
	auto req = "{\"action\":\"hello\",\"data\":{}}";
	auto resp = RpcRequest(req);
	QJsonParseError err;
	auto doc = QJsonDocument::fromJson(resp.toUtf8(), &err);
	if (err.error != QJsonParseError::NoError) {
		throw err.errorString();
	}
	return doc.object()["data"].toObject()["text"].toString();
}