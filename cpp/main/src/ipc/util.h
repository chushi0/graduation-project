#pragma once

#include "base.h"
#include "ipc.h"
#include "types.h"
#include <QJsonArray>
#include <QStringList>

#define hash(k, v) QHash<k, v>
#define list(e) QList<e>
#define def_array_value(name, type)                                            \
	void parseArray##name(QJsonArray array, QList<type> *out)
#define def_object_value(name, type)                                           \
	void parseHashString##name(QJsonObject object, QHash<QString, type> *out)

namespace ipc {
	void parseErrors(QJsonArray array, QList<ErrorType> *list);
	void parseLLVariables(QJsonObject object, LLBreakpointVariables *out);
	void parseLLExitResult(QJsonObject object, LLExitResult *out);
	void parseLR0Variables(QJsonObject object, LR0BreakpointVariables *out);
	void parseLR0ExitResult(QJsonObject object, LR0ExitResult *out);
	void parseLR1Variables(QJsonObject object, LR1BreakpointVariables *out);
	void parseLR1ExitResult(QJsonObject object, LR1ExitResult *out);

	void parseReplaceProduction(QJsonObject object, ReplaceProduction *out);
	def_array_value(ReplaceProduction, ReplaceProduction);

	void parseLRItem(QJsonObject object, LRItem *out);
	void parseLRItemClosureMapEdge(QJsonObject object,
								   LRItemClosureMapEdge *out);
	void parseLRItemClosureMap(QJsonObject object, LRItemClosureMap *out);
	def_array_value(LRItem, LRItem);
	def_array_value(ArrayLRItem, LRItemClosure);
	def_array_value(LRItemClosureMapEdge, LRItemClosureMapEdge);

	def_array_value(String, QString);
	def_array_value(ArrayString, QStringList);
	def_array_value(HashStringInt, hash(QString, int));
	def_array_value(HashStringString, hash(QString, QString));
	def_object_value(ArrayString, QStringList);
	def_object_value(Int, int);
	def_object_value(String, QString);
	def_object_value(HashStringInt, hash(QString, int));

} // namespace ipc

#undef def_array_value
#undef def_object_value
#undef hash
#undef list