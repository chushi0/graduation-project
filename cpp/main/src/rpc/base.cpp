#include "base.h"
#include <QMutex>
#include <QTextStream>

namespace rpc {
	QTextStream *stdinStream, *stdoutStream, *stderrStream;
	QMutex *mutex;
} // namespace rpc

void rpc::Init() {
	stdinStream = new QTextStream(stdin);
	stdoutStream = new QTextStream(stdout);
	stderrStream = new QTextStream(stderr);

	stdinStream->setEncoding(QStringConverter::Encoding::Utf8);
	stdoutStream->setEncoding(QStringConverter::Encoding::Utf8);
	stderrStream->setEncoding(QStringConverter::Encoding::Utf8);

	mutex = new QMutex();
}

QString rpc::ReceiveRpcMessage() {
	QString msg;
	do {
		msg = stdinStream->readLine();
	} while (msg.isEmpty());
	return msg;
}

void rpc::SendRpcMessage(QString msg) {
	*stdoutStream << msg << "\n";
	stdoutStream->flush();
}

void rpc::SendLogMessage(QString msg) {
	*stderrStream << msg << "\n";
	stderrStream->flush();
}

QString rpc::RpcRequest(QString req) {
	QMutexLocker locker(mutex);
	SendRpcMessage(req);
	return ReceiveRpcMessage();
}