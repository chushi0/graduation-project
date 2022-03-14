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

	QString LR0ProcessRequest(QString code, bool slr);
	void LR0ProcessSwitchMode(QString id, int mode);
	void LR0ProcessRelease(QString id);
	void LR0ProcessSetBreakpoints(QString id, QList<Breakpoint> breakpoints);
	bool LR0ProcessGetVariables(QString id, LR0BreakpointVariables *variables,
								Breakpoint *point);
	bool LR0ProcessExit(QString id, LR0ExitResult *exitResult);

	QString LR1ProcessRequest(QString code, bool lalr);
	void LR1ProcessSwitchMode(QString id, int mode);
	void LR1ProcessRelease(QString id);
	void LR1ProcessSetBreakpoints(QString id, QList<Breakpoint> breakpoints);
	bool LR1ProcessGetVariables(QString id, LR1BreakpointVariables *variables,
								Breakpoint *point);
	bool LR1ProcessExit(QString id, LR1ExitResult *exitResult);
} // namespace ipc