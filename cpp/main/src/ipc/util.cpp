#include "util.h"

#define cast_object toObject
#define cast_array toArray
#define cast_string toString
#define cast_int toInt
#define cast_bool toBool

#define hash(k, v) QHash<k, v>
#define list(e) QList<e>

#define def_array_value(name, type, cast)                                      \
	void ipc::parseArray##name(QJsonArray array, QList<type> *out) {           \
		for (auto i : array) {                                                 \
			auto o = i.cast();                                                 \
			type res;                                                          \
			parse##name(o, &res);                                              \
			out->append(res);                                                  \
		}                                                                      \
	}

#define def_object_value(name, type, cast)                                     \
	void ipc::parseHashString##name(QJsonObject object,                        \
									QHash<QString, type> *out) {               \
		for (auto &k : object.keys()) {                                        \
			type res;                                                          \
			parse##name(object[k].cast(), &res);                               \
			(*out)[k] = res;                                                   \
		}                                                                      \
	}
#define def_basic(name, type)                                                  \
	inline void parse##name(type in, type *out) {                              \
		*out = in;                                                             \
	}

def_basic(String, QString);
def_basic(Int, int);
def_basic(Bool, bool);

def_array_value(String, QString, cast_string);
def_array_value(ArrayString, QStringList, cast_array);
def_array_value(HashStringInt, hash(QString, int), cast_object);
def_array_value(HashStringString, hash(QString, QString), cast_object);
def_object_value(ArrayString, QStringList, cast_array);
def_object_value(Int, int, cast_int);
def_object_value(String, QString, cast_string);
def_object_value(HashStringInt, hash(QString, int), cast_object);

def_array_value(ReplaceProduction, ReplaceProduction, cast_object);
def_array_value(LRItem, LRItem, cast_object);
def_array_value(ArrayLRItem, LRItemClosure, cast_array);
def_array_value(LRItemClosureMapEdge, LRItemClosureMapEdge, cast_object);

void ipc::parseErrors(QJsonArray array, QList<ipc::ErrorType> *list) {
	QList<ipc::ErrorType> result;
	for (auto i : array) {
		auto o = i.toObject();
		ipc::ErrorType error;
		error.type = o["type"].toInt();
		error.file = o["file"].toString();
		error.line = o["line"].toInt();
		error.column = o["column"].toInt();
		error.detail = o["detail"].toString();
		error.length = o["length"].toInt();
		result.append(error);
	}
	*list = result;
}

void ipc::parseLLVariables(QJsonObject object, LLBreakpointVariables *out) {
	parseArrayString(object["terminals"].toArray(), &out->terminals);
	parseArrayArrayString(object["productions"].toArray(), &out->productions);
	out->loopVariableI = object["loop_variable_i"].toInt();
	out->loopVariableJ = object["loop_variable_j"].toInt();
	out->loopVariableK = object["loop_variable_k"].toInt();
	out->modifiedFlag = object["modified_flag"].toBool();
	parseArrayString(object["nonterminal_orders"].toArray(),
					 &out->nonterminalOrders);
	parseArrayString(object["current_process_production"].toArray(),
					 &out->currentProcessProduction);
	parseArrayArrayString(object["remove_production"].toArray(),
						  &out->removeProductions);
	parseArrayArrayString(object["add_production"].toArray(),
						  &out->addProductions);
	parseArrayReplaceProduction(object["replace_production"].toArray(),
								&out->replaceProduction);
	parseArrayString(object["common_prefix"].toArray(), &out->commonPrefix);
	parseHashStringArrayString(object["first"].toObject(), &out->firstSet);
	parseHashStringArrayString(object["follow"].toObject(), &out->followSet);
	parseArrayArrayString(object["select"].toArray(), &out->selectSet);
	parseHashStringHashStringInt(object["automaton"].toObject(),
								 &out->automation);
}

void ipc::parseLLExitResult(QJsonObject object, LLExitResult *out) {
	out->code = object["code"].toInt();
	parseLLVariables(object["variables"].toObject(), &out->variable);
}

void ipc::parseLR0Variables(QJsonObject object, LR0BreakpointVariables *out) {
	parseArrayString(object["terminals"].toArray(), &out->terminals);
	parseArrayArrayString(object["productions"].toArray(), &out->productions);
	out->loopVariableI = object["loop_variable_i"].toInt();
	out->loopVariableJ = object["loop_variable_j"].toInt();
	out->loopVariableK = object["loop_variable_k"].toInt();
	out->modifiedFlag = object["modified_flag"].toBool();
	parseArrayString(object["nonterminal_orders"].toArray(),
					 &out->nonterminalOrders);
	parseArrayString(object["process_symbol"].toArray(), &out->processedSymbol);
	out->currentProcessSymbol = object["current_symbol"].toString();
	parseHashStringArrayString(object["first"].toObject(), &out->firstSet);
	parseHashStringArrayString(object["follow"].toObject(), &out->followSet);
	parseLRItemClosureMap(object["closure_map"].toObject(), &out->closureMap);
	parseArrayLRItem(object["current_closure"].toArray(), &out->currentClosure);
	parseArrayHashStringString(object["action_table"].toArray(),
							   &out->actionTable);
	parseArrayHashStringInt(object["goto_table"].toArray(), &out->gotoTable);
}

void ipc::parseLR0ExitResult(QJsonObject object, LR0ExitResult *out) {
	out->code = object["code"].toInt();
	parseLR0Variables(object["variables"].toObject(), &out->variable);
}

void ipc::parseLR1Variables(QJsonObject object, LR1BreakpointVariables *out) {
	parseArrayString(object["terminals"].toArray(), &out->terminals);
	parseArrayArrayString(object["productions"].toArray(), &out->productions);
	out->loopVariableI = object["loop_variable_i"].toInt();
	out->loopVariableJ = object["loop_variable_j"].toInt();
	out->loopVariableK = object["loop_variable_k"].toInt();
	out->modifiedFlag = object["modified_flag"].toBool();
	parseArrayString(object["nonterminal_orders"].toArray(),
					 &out->nonterminalOrders);
	parseArrayString(object["process_symbol"].toArray(), &out->processedSymbol);
	out->currentProcessSymbol = object["current_symbol"].toString();
	parseHashStringArrayString(object["first"].toObject(), &out->firstSet);
	parseLRItemClosureMap(object["closure_map"].toObject(), &out->closureMap);
	parseArrayLRItem(object["current_closure"].toArray(), &out->currentClosure);
	parseArrayHashStringString(object["action_table"].toArray(),
							   &out->actionTable);
	parseArrayHashStringInt(object["goto_table"].toArray(), &out->gotoTable);
}

void ipc::parseLR1ExitResult(QJsonObject object, LR1ExitResult *out) {
	out->code = object["code"].toInt();
	parseLR1Variables(object["variables"].toObject(), &out->variable);
}

void ipc::parseReplaceProduction(QJsonObject object, ReplaceProduction *out) {
	parseArrayString(object["original"].toArray(), &out->original);
	parseArrayString(object["replace"].toArray(), &out->replace);
}

void ipc::parseLRItem(QJsonObject object, LRItem *out) {
	out->production = object["prod"].toInt();
	out->progress = object["progress"].toInt();
	out->lookahead = object["lookahead"].toString();
}

void ipc::parseLRItemClosureMapEdge(QJsonObject object,
									LRItemClosureMapEdge *out) {
	out->from = object["from"].toInt();
	out->to = object["to"].toInt();
	out->symbol = object["symbol"].toString();
}

void ipc::parseLRItemClosureMap(QJsonObject object, LRItemClosureMap *out) {
	parseArrayArrayLRItem(object["closures"].toArray(), &out->closures);
	parseArrayLRItemClosureMapEdge(object["edges"].toArray(), &out->edges);
}