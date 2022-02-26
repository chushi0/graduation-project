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
	struct ReplaceProduction {
		QStringList original, replace;
	};
	struct LLBreakpointVariables {
		QStringList terminals;
		QList<QStringList> productions;
		int loopVariableI, loopVariableJ, loopVariableK;
		bool modifiedFlag;
		QStringList nonterminalOrders;
		QStringList currentProcessProduction;
		QList<QStringList> removeProductions;
		QList<QStringList> addProductions;
		QList<ReplaceProduction> replaceProduction;
		QStringList commonPrefix;
		QHash<QString, QStringList> firstSet, followSet;
		QList<QStringList> selectSet;
		QHash<QString, QHash<QString, int>> automation;
	};
	struct LLExitResult {
		int code;
		LLBreakpointVariables variable;
	};

	struct LRItem {
		int production;
		int progress;
	};
	struct LRItemClosureMapEdge {
		int from, to;
		QString symbol;
	};
	typedef QList<LRItem> LRItemClosure;
	struct LRItemClosureMap {
		QList<LRItemClosure> closures;
		QList<LRItemClosureMapEdge> edges;
	};

	struct LR0BreakpointVariables {
		QStringList terminals;
		QList<QStringList> productions;
		int loopVariableI, loopVariableJ, loopVariableK;
		bool modifiedFlag;
		QStringList nonterminalOrders;
		QStringList processedSymbol;
		QString currentProcessSymbol;
		QHash<QString, QStringList> firstSet, followSet;
		LRItemClosureMap closureMap;
		LRItemClosure currentClosure;
		QList<QHash<QString, QString>> actionTable;
		QList<QHash<QString, int>> gotoTable;
	};
	struct LR0ExitResult {
		int code;
		LR0BreakpointVariables variable;
	};
} // namespace ipc