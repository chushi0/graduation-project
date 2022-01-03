#include <QJsonObject>
#include <QString>

namespace ipc {

	struct Response {
		int ResponseCode;
		QJsonObject Data;
	};

	void Init();
	QString ReceiveRpcMessage();
	void SendRpcMessage(QString);
	void SendLogMessage(QString);

	ipc::Response RpcRequest(QString);
} // namespace ipc