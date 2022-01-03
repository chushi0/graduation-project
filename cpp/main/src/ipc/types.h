#pragma once

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
} // namespace ipc