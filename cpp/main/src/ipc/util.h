#pragma once

#include "base.h"
#include "ipc.h"
#include "types.h"
#include <QJsonArray>
#include <QStringList>

namespace ipc {
	void parseJsonArrayToStringList(QJsonArray array, QStringList *list);
	void parseStringListList(QJsonArray array, QList<QStringList> *list);
	void parseErrors(QJsonArray array, QList<ErrorType> *list);
	void parseLLVariables(QJsonObject object, LLBreakpointVariables *out);
	void parseHashStringStringList(QJsonObject object,
								   QHash<QString, QStringList> *out);
} // namespace ipc