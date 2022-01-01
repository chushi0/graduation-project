#include <QString>

namespace rpc {
	void Init();
	QString ReceiveRpcMessage();
	void SendRpcMessage(QString);
	void SendLogMessage(QString);

	QString RpcRequest(QString);
} // namespace rpc