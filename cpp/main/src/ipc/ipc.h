#include <QList>
#include <QString>
#include <QStringList>

namespace ipc {
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

	QString ProductionParseStart(QString code);
	bool ProductionParseQuery(QString id, ProductionResult *result);
	void ProductionParseCancel(QString id);
} // namespace ipc