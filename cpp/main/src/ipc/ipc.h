#pragma once

#include "types.h"
#include <QList>
#include <QString>
#include <QStringList>

namespace ipc {
	QString ProductionParseStart(QString code);
	bool ProductionParseQuery(QString id, ProductionResult *result);
	void ProductionParseCancel(QString id);

	QString LLProcessRequest(QString code, bool withTranslate);
	void LLProcessSwitchMode(QString id, int mode);
	void LLProcessRelease(QString id);
	void LLProcessSetBreakpoints(QString id, QList<Breakpoint> breakpoints);
	bool LLProcessGetVariables(QString id, LLBreakpointVariables *variables,
							   Breakpoint *point);
	bool LLProcessExit(QString id, LLExitResult *exitResult);
} // namespace ipc