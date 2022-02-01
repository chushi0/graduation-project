#pragma once

#include <QHash>
#include <QJsonObject>
#include <QList>
#include <QString>
#include <QStringList>

namespace ipc {
	struct Response {
		int ResponseCode;
		QJsonObject Data;
	};

	struct ErrorType {
		int type;
		int line, column, length;
		QString file;
		QString detail;
	};

	struct ProductionResult {
		QStringList terminals;
		QStringList nonterminals;
		QList<QStringList> productions;
		QList<ErrorType> fatals, errors, warnings;
	};

	constexpr int LLProcessModeRun = 1;
	constexpr int LLProcessModePause = 2;
	constexpr int LLProcessModeExit = 4;
	struct Breakpoint {
		QString name;
		int line;
	};
	struct LLBreakpointVariables {
		QList<QStringList> productions;
		int loopVariableI, loopVariableJ, loopVariableK;
		bool modifiedFlag;
		QStringList nonterminalOrders;
		QStringList currentProcessProduction;
		QStringList commonPrefix;
		QHash<QString, QStringList> firstSet, followSet;
		QList<QStringList> selectSet;
	};
} // namespace ipc