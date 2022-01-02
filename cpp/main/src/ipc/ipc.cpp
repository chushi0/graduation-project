#include "ipc.h"
#include "base.h"
#include <QJsonDocument>
#include <QJsonObject>

QString ipc::Hello() {
	auto req = "{\"action\":\"hello\",\"data\":{}}";
	auto resp = RpcRequest(req);
	return resp.Data["text"].toString();
}