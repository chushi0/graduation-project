#include "DemoWidget.h"
#include <QMouseEvent>
#include <QPainter>

static constexpr auto EMPTY_TERMINAL = "ε";
static constexpr auto EOF_TERMINAL = "$";

DemoWidget::DemoWidget(QWidget *parent)
	: QWidget(parent), x(0), y(0), mousePressed(false), mode(None) {
	resize(200, 200);
}

DemoWidget::~DemoWidget() {
}

void DemoWidget::setMode(Mode mode) {
	this->mode = mode;
	switch (mode) {
		case LL_ProductionList:
			setWindowTitle("产生式列表");
			break;
		case LL_FirstTable:
			setWindowTitle("First 集");
			break;
		case LL_FollowTable:
			setWindowTitle("Follow 集");
			break;
		case LL_FirstFollowTable:
			setWindowTitle("First、Follow 集");
			break;
		case LL_SelectTable:
			setWindowTitle("Select 集");
			break;
		case LL_AutomationTable:
			setWindowTitle("LL 自动机");
			break;
	}
	update();
}

void DemoWidget::setVariableAndPoint(const ipc::LLBreakpointVariables &variable,
									 const ipc::Breakpoint &point) {
	llVariable = variable;
	this->point = point;
	update();
}

void DemoWidget::setVariableAndPoint(
	const ipc::LR0BreakpointVariables &variable, const ipc::Breakpoint &point) {
	lr0Variable = variable;
	this->point = point;
	update();
}

void DemoWidget::setVariableAndPoint(
	const ipc::LR1BreakpointVariables &variable, const ipc::Breakpoint &point) {
	lr1Variable = variable;
	this->point = point;
	update();
}

void DemoWidget::translateDefault() {
	x = 0;
	y = 0;
	update();
}

void DemoWidget::mousePressEvent(QMouseEvent *event) {
	mousePressed = true;
	lastMouseX = event->x();
	lastMouseY = event->y();
}

void DemoWidget::mouseReleaseEvent(QMouseEvent *) {
	mousePressed = false;
}

void DemoWidget::mouseMoveEvent(QMouseEvent *event) {
	if (!mousePressed) {
		return;
	}
	float x = event->x();
	float y = event->y();
	this->x += x - lastMouseX;
	this->y += y - lastMouseY;
	lastMouseX = x;
	lastMouseY = y;
	update();
}

void DemoWidget::paintEvent(QPaintEvent *event) {
	QPainter painter(this);
	if (mode == None) {
		return;
	}

	auto normalFont = font();
	auto smallFont = normalFont;
	smallFont.setPixelSize(12);

	auto normalFontMetrics = QFontMetrics(normalFont);
	auto smallFontMetrics = QFontMetrics(smallFont);

	painter.setFont(normalFont);
	painter.save();
	painter.translate(x + 30, y + 20);

	PaintContext ctx;
	ctx.painter = &painter;
	ctx.normalFont = &normalFont;
	ctx.smallFont = &smallFont;
	ctx.normalFontMetrics = &normalFontMetrics;
	ctx.smallFontMetrics = &smallFontMetrics;

	switch (mode) {
		case LL_ProductionList:
			drawimpl_LL_ProductionList(ctx);
			break;
		case LL_FirstTable:
			drawimpl_LL_FirstTable(ctx);
			break;
		case LL_FollowTable:
			drawimpl_LL_FollowTable(ctx);
			break;
		case LL_FirstFollowTable:
			drawimpl_LL_FirstFollowTable(ctx);
			break;
		case LL_SelectTable:
			drawimpl_LL_SelectTable(ctx);
			break;
		case LL_AutomationTable:
			drawimpl_LL_AutomationTable(ctx);
			break;
	}

	painter.restore();
}

void DemoWidget::drawimpl_LL_ProductionList(const PaintContext &ctx) {
	const auto &variable = llVariable;
	constexpr bool autoDisableColor = true;

	int height = ctx.normalFontMetrics->height();
	int y = 0;
	bool addDrawBefore = variable.removeProductions.isEmpty() &&
						 !variable.addProductions.isEmpty();
	for (auto arrProd : variable.productions) {
		if (autoDisableColor &&
			isNonterminalEqual(variable.loopVariableI, arrProd[0],
							   variable.nonterminalOrders)) {
			ctx.painter->setPen(QColor(0x80, 0x80, 0x80));
		}
		if (addDrawBefore &&
			isNonterminalEqual(variable.loopVariableI - 1, arrProd[0],
							   variable.nonterminalOrders)) {
			addDrawBefore = false;
			ctx.painter->setPen(QColor(0, 0xff, 0, 0xaa));
			for (auto arrProd : variable.addProductions) {
				QString prod = arrProd[0] + " :=";
				for (int i = 1; i < arrProd.size(); i++) {
					prod += " " + arrProd[i];
				}
				ctx.painter->drawText(0, y, prod);
				y += height;
			}
			ctx.painter->setPen(QColor(0, 0, 0));
		}
		QString prod = arrProd[0] + " :=";
		for (int i = 1; i < arrProd.size(); i++) {
			prod += " " + arrProd[i];
		}
		auto bounding = ctx.normalFontMetrics->boundingRect(prod);
		if (variable.currentProcessProduction == arrProd) {
			ctx.painter->fillRect(bounding.left(), y + bounding.top(),
								  bounding.width(), bounding.height(),
								  QColor(0xff, 0x99, 0));
		} else if (isNonterminalEqual(variable.loopVariableI - 1, arrProd[0],
									  variable.nonterminalOrders)) {
			ctx.painter->fillRect(bounding.left(), y + bounding.top(),
								  bounding.width(), bounding.height(),
								  QColor(0xff, 0x99, 0, 0x80));
			if (isProdPrefixEqual(arrProd, variable.commonPrefix)) {
				QString commonPrefix;
				for (auto n : variable.commonPrefix) {
					commonPrefix += " " + n;
				}
				auto left =
					ctx.normalFontMetrics->boundingRect(arrProd[0] + " := ")
						.right();
				auto width =
					ctx.normalFontMetrics->boundingRect(commonPrefix).width();
				ctx.painter->fillRect(left, y + bounding.top(), width,
									  bounding.height(), QColor(0xff, 0x99, 0));
			}
		} else if (isNonterminalEqual(variable.loopVariableJ - 1, arrProd[0],
									  variable.nonterminalOrders)) {
			ctx.painter->fillRect(bounding.left(), y + bounding.top(),
								  bounding.width(), bounding.height(),
								  QColor(0, 0xff, 0xff, 0x80));
		}
		ctx.painter->drawText(0, y, prod);
		if (isRemoveProduction(arrProd, variable.removeProductions,
							   variable.replaceProduction)) {
			ctx.painter->setPen(QColor(0, 0, 0));
			auto lineY = y + bounding.top() + bounding.height() / 2;
			ctx.painter->drawLine(0, lineY, bounding.width(), lineY);
		}
		auto paintAfter = showAfterProductionDraw(
			arrProd, variable.removeProductions, variable.replaceProduction,
			variable.addProductions);
		if (!paintAfter.isEmpty()) {
			ctx.painter->setPen(QColor(0, 0xff, 0, 0xaa));
			for (auto arrProd : paintAfter) {
				y += height;
				QString prod = arrProd[0] + " :=";
				for (int i = 1; i < arrProd.size(); i++) {
					prod += " " + arrProd[i];
				}
				ctx.painter->drawText(0, y, prod);
			}
			ctx.painter->setPen(QColor(0, 0, 0));
		}
		y += height;
	}
}

void DemoWidget::drawimpl_LL_FirstTable(const PaintContext &ctx) {
	const auto &variable = llVariable;

	QList<QStringList> table;
	QStringList head;
	QStringList firstBody;
	head << "非终结符";
	firstBody << "First 集";
	for (auto nonterminal : llVariable.nonterminalOrders) {
		head << nonterminal;
		auto firsts = variable.firstSet[nonterminal];
		QString firstText;
		for (auto first : firsts) {
			if (first == "") {
				first = EMPTY_TERMINAL;
			}
			firstText += first + "  ";
		}
		firstBody << firstText;
	}
	table << head;
	table << firstBody;
	paintTable(ctx, 0, 0, table);
}

void DemoWidget::drawimpl_LL_FollowTable(const PaintContext &ctx) {
	const auto &variable = llVariable;

	QList<QStringList> table;
	QStringList head;
	QStringList followBody;
	head << "非终结符";
	followBody << "Follow 集";
	for (auto nonterminal : llVariable.nonterminalOrders) {
		head << nonterminal;
		auto follows = variable.followSet[nonterminal];
		QString followText;
		for (auto follow : follows) {
			if (follow == "$") {
				follow = EOF_TERMINAL;
			}
			followText += follow + "  ";
		}
		followBody << followText;
	}
	table << head;
	table << followBody;
	paintTable(ctx, 0, 0, table);
}

void DemoWidget::drawimpl_LL_FirstFollowTable(const PaintContext &ctx) {
	const auto &variable = llVariable;

	QList<QStringList> table;
	QStringList head;
	QStringList firstBody;
	QStringList followBody;
	head << "非终结符";
	firstBody << "First 集";
	followBody << "Follow 集";
	for (auto nonterminal : llVariable.nonterminalOrders) {
		head << nonterminal;
		auto firsts = variable.firstSet[nonterminal];
		QString firstText;
		for (auto first : firsts) {
			if (first == "") {
				first = EMPTY_TERMINAL;
			}
			firstText += first + "  ";
		}
		firstBody << firstText;
		auto follows = variable.followSet[nonterminal];
		QString followText;
		for (auto follow : follows) {
			if (follow == "$") {
				follow = EOF_TERMINAL;
			}
			followText += follow + "  ";
		}
		followBody << followText;
	}
	table << head;
	table << firstBody;
	table << followBody;
	paintTable(ctx, 0, 0, table);
}

void DemoWidget::drawimpl_LL_SelectTable(const PaintContext &ctx) {
	const auto &variable = llVariable;

	QList<QStringList> table;
	QStringList head;
	QStringList selectBody;
	head << "产生式";
	selectBody << "Select 集";
	for (int i = 0; i < variable.productions.size(); i++) {
		auto prod = prodToString(variable.productions[i]);
		head << prod;
		if (variable.selectSet.size() > i) {
			auto selects = variable.selectSet[i];
			QString selectText = "";
			for (auto select : selects) {
				if (select == "$") {
					select = EOF_TERMINAL;
				}
				selectText += select + " ";
			}
			selectBody << selectText;
		} else {
			selectBody << "";
		}
	}
	table << head;
	table << selectBody;
	paintTable(ctx, 0, 0, table);
}

void DemoWidget::drawimpl_LL_AutomationTable(const PaintContext &ctx) {
	const auto &variable = llVariable;

	QList<QStringList> table;
	QStringList head;
	head << "";
	for (auto nonterminal : variable.nonterminalOrders) {
		head << nonterminal;
	}
	table << head;
	QStringList terminals = variable.terminals;
	terminals << "$";
	terminals.sort();
	for (auto terminal : terminals) {
		QStringList column;
		column << terminal;
		for (auto nonterminal : variable.nonterminalOrders) {
			auto prodIndex =
				variable.automation[nonterminal].value(terminal, -1);
			if (prodIndex == -1) {
				column << "";
			} else if (prodIndex == -2) {
				column << "冲突";
			} else {
				column << prodToString(variable.productions[prodIndex]);
			}
		}
		table << column;
	}
	paintTable(ctx, 0, 0, table, {"冲突"});
}

void DemoWidget::paintTable(const PaintContext &ctx, int x, int y,
							QList<QStringList> content, QStringList warnText) {
	for (auto &column : content) {
		x += paintTableColumn(ctx, x, y, column, warnText);
	}
}

int DemoWidget::paintTableColumn(const PaintContext &ctx, int x, int y,
								 QStringList content, QStringList warnText) {
	int height = ctx.normalFontMetrics->height() + 8;
	int curY = y;
	int width = 0;
	for (auto row : content) {
		curY += height;
		width =
			std::max(width, ctx.normalFontMetrics->boundingRect(row).width());
		if (warnText.contains(row)) {
			ctx.painter->setPen(QColor(0xff, 0, 0));
		}
		ctx.painter->drawText(x, curY, row);
		ctx.painter->setPen(QColor(0, 0, 0));
	}
	ctx.painter->drawLine(x - 8, y + 4, x - 8, curY + 4);
	ctx.painter->drawLine(x + width + 8, y + 4, x + width + 8, curY + 4);
	for (int i = 0; i < content.size() + 1; i++) {
		ctx.painter->drawLine(x - 8, y + 4, x + width + 8, y + 4);
		y += height;
	}
	return width + 16;
}

bool DemoWidget::isNonterminalEqual(int index, QString nonterminal,
									QStringList nonterminalList) {
	if (index < 0 || index >= nonterminalList.size()) {
		return false;
	}
	return nonterminalList[index] == nonterminal;
}

bool DemoWidget::isProdPrefixEqual(QStringList prod, QStringList prefix) {
	if (prefix.isEmpty()) {
		return false;
	}
	if (prod.size() < prefix.size()) {
		return false;
	}
	for (int i = 0; i < prefix.size(); i++) {
		if (prefix[i] != prod[i + 1]) {
			return false;
		}
	}
	return true;
}

bool DemoWidget::isRemoveProduction(QStringList prod,
									QList<QStringList> removeList,
									QList<ipc::ReplaceProduction> replaceList) {
	for (auto rmProd : removeList) {
		if (rmProd == prod) {
			return true;
		}
	}
	for (auto rp : replaceList) {
		if (prod == rp.original) {
			return true;
		}
	}
	return false;
}

QList<QStringList> DemoWidget::showAfterProductionDraw(
	QStringList prod, QList<QStringList> removeList,
	QList<ipc::ReplaceProduction> replaceList,
	QList<QStringList> addProduction) {
	for (auto rp : replaceList) {
		if (prod == rp.original) {
			QList<QStringList> resp;
			resp << rp.replace;
			return resp;
		}
	}
	for (auto rmProd : removeList) {
		if (rmProd == prod) {
			return addProduction;
		}
	}
	return QList<QStringList>();
}

QString DemoWidget::prodToString(QStringList prod) {
	QString res = prod[0] + " :=";
	for (int i = 1; i < prod.size(); i++) {
		res += " " + prod[i];
	}
	return res;
}