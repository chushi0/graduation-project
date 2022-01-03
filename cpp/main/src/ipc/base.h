#pragma once

#include "types.h"
#include <QString>

namespace ipc {

	void Init();
	QString ReceiveRpcMessage();
	void SendRpcMessage(QString);
	void SendLogMessage(QString);

	ipc::Response RpcRequest(QString);
} // namespace ipc