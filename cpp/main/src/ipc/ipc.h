#pragma once

#include "types.h"
#include <QList>
#include <QString>
#include <QStringList>

namespace ipc {
	QString ProductionParseStart(QString code);
	bool ProductionParseQuery(QString id, ProductionResult *result);
	void ProductionParseCancel(QString id);

	QString LLProcessRequest(QString code);
	void LLProcessSwitchMode(QString id, int mode);
	void LLProcessRelease(QString id);
	void LLProcessSetBreakpoints(QString id, QList<Breakpoint> breakpoints);
	bool LLProcessGetVariables(QString id, LLBreakpointVariables *variables);
} // namespace ipc