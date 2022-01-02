#include <QString>

namespace ipc {
	void Init();
	QString ReceiveRpcMessage();
	void SendRpcMessage(QString);
	void SendLogMessage(QString);

	QString RpcRequest(QString);
} // namespace ipc