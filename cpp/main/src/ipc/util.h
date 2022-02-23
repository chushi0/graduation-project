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
	void parseLLExitResult(QJsonObject object, LLExitResult *out);
	void parseHashStringStringList(QJsonObject object,
								   QHash<QString, QStringList> *out);
	void parseReplaceProduction(QJsonObject object, ReplaceProduction *out);
	void parseReplaceProductionArray(QJsonArray array,
									 QList<ReplaceProduction> *out);
	void parseHashStringInt(QJsonObject, QHash<QString, int> *out);
	void parseHashStringHashStringInt(QJsonObject object,
									  QHash<QString, QHash<QString, int>> *out);

} // namespace ipc