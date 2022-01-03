#include "base.h"
#include "ipc.h"
#include <QJsonArray>
#include <QStringList>

namespace ipc {
	void parseJsonArrayToStringList(QJsonArray array, QStringList *list);
	void parseProductions(QJsonArray array, QList<QStringList> *list);
	void parseErrors(QJsonArray array, QList<ErrorType> *list);
} // namespace ipc