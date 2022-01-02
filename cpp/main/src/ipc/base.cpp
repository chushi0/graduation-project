#include "base.h"
#include <QJsonDocument>
#include <QMutex>
#include <QTextStream>

namespace ipc {
	QTextStream *stdinStream, *stdoutStream, *stderrStream;
	QMutex *mutex;
} // namespace ipc

void ipc::Init() {
	stdinStream = new QTextStream(stdin);
	stdoutStream = new QTextStream(stdout);
	stderrStream = new QTextStream(stderr);

	stdinStream->setEncoding(QStringConverter::Encoding::Utf8);
	stdoutStream->setEncoding(QStringConverter::Encoding::Utf8);
	stderrStream->setEncoding(QStringConverter::Encoding::Utf8);

	mutex = new QMutex();
}

QString ipc::ReceiveRpcMessage() {
	QString msg;
	do {
		msg = stdinStream->readLine();
	} while (msg.isEmpty());
	return msg;
}

void ipc::SendRpcMessage(QString msg) {
	*stdoutStream << msg << "\n";
	stdoutStream->flush();
}

void ipc::SendLogMessage(QString msg) {
	*stderrStream << msg << "\n";
	stderrStream->flush();
}

ipc::Response ipc::RpcRequest(QString req) {
	QMutexLocker locker(mutex);
	SendRpcMessage(req);
	QString resp = ReceiveRpcMessage();
	locker.unlock();
	QJsonParseError err;
	auto doc = QJsonDocument::fromJson(resp.toUtf8(), &err);
	if (err.error != QJsonParseError::NoError) {
		throw err.errorString();
	}
	Response response;
	response.ResponseCode = doc.object()["code"].toInt();
	response.Data = doc.object()["data"].toObject();
	return response;
}