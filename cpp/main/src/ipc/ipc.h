#pragma once

#include "types.h"
#include <QList>
#include <QString>
#include <QStringList>

namespace ipc {
	QString ProductionParseStart(QString code);
	bool ProductionParseQuery(QString id, ProductionResult *result);
	void ProductionParseCancel(QString id);
} // namespace ipc