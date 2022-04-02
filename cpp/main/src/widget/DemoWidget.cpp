#include "DemoWidget.h"
#include <QMouseEvent>
#include <QPainter>

static constexpr auto EMPTY_TERMINAL = "ε";
static constexpr auto EOF_TERMINAL = "$";

DemoWidget::DemoWidget(QWidget *parent)
	: QWidget(parent), x(0), y(0), mousePressed(false), mode(None),
	  variableRefresh(true) {
	resize(200, 200);
}

DemoWidget::~DemoWidget() {
}

void DemoWidget::setMode(Mode mode) {
	this->mode = mode;
	switch (mode) {
		case LL_ProductionList:
		case LR0_ProductionList:
			setWindowTitle("产生式列表");
			break;
		case LL_FirstTable:
		case LR0_FirstTable:
			setWindowTitle("First 集");
			break;
		case LL_FollowTable:
		case LR0_FollowTable:
			setWindowTitle("Follow 集");
			break;
		case LL_FirstFollowTable:
		case LR0_FirstFollowTable:
			setWindowTitle("First、Follow 集");
			break;
		case LL_SelectTable:
			setWindowTitle("Select 集");
			break;
		case LL_AutomationTable:
			setWindowTitle("LL 自动机");
			break;
		case LR0_ItemClosure:
			setWindowTitle("项目集闭包");
			break;
		case LR0_AutomationTable:
			setWindowTitle("LR 自动机");
			break;
	}
	update();
}

void DemoWidget::setVariableAndPoint(const ipc::LLBreakpointVariables &variable,
									 const ipc::Breakpoint &point) {
	llVariable = variable;
	this->point = point;
	variableRefresh = true;
	update();
}

void DemoWidget::setVariableAndPoint(
	const ipc::LR0BreakpointVariables &variable, const ipc::Breakpoint &point) {
	lr0Variable = variable;
	this->point = point;
	variableRefresh = true;
	update();
}

void DemoWidget::setVariableAndPoint(
	const ipc::LR1BreakpointVariables &variable, const ipc::Breakpoint &point) {
	lr1Variable = variable;
	this->point = point;
	variableRefresh = true;
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
		case LR0_ProductionList:
			drawimpl_LR0_ProductionList(ctx);
			break;
		case LR0_FirstTable:
			drawimpl_LR0_FirstTable(ctx);
			break;
		case LR0_FollowTable:
			drawimpl_LR0_FollowTable(ctx);
			break;
		case LR0_FirstFollowTable:
			drawimpl_LR0_FirstFollowTable(ctx);
			break;
		case LR0_ItemClosure:
			drawimpl_LR0_ItemClosure(ctx);
			break;
		case LR0_AutomationTable:
			drawimpl_LR0_AutomationTable(ctx);
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
	for (auto nonterminal : variable.nonterminalOrders) {
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
	for (auto nonterminal : variable.nonterminalOrders) {
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
	for (auto nonterminal : variable.nonterminalOrders) {
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

void DemoWidget::drawimpl_LR0_ProductionList(const PaintContext &ctx) {
	const auto &variable = lr0Variable;
	auto color = false;
	if (point.name == "ComputeFirstSet" || point.name == "ComputeFollowSet") {
		color = true;
	}
	int height = ctx.normalFontMetrics->height();
	int y = height;
	for (int i = 0; i < variable.productions.size(); i++) {
		auto arrProd = variable.productions[i];
		QString prod = arrProd[0] + " :=";
		for (int i = 1; i < arrProd.size(); i++) {
			prod += " " + arrProd[i];
		}
		auto bounding = ctx.normalFontMetrics->boundingRect(prod);
		if (color && variable.loopVariableI == i) {
			ctx.painter->fillRect(bounding.left(), y + bounding.top(),
								  bounding.width(), bounding.height(),
								  QColor(0xff, 0x99, 0, 0x80));
			if (variable.loopVariableJ > 0 &&
				variable.loopVariableJ < arrProd.size()) {
				auto bounding = computeProductionCellBounding(
					ctx, arrProd, variable.loopVariableJ);
				ctx.painter->fillRect(bounding.left(), y + bounding.top(),
									  bounding.width(), bounding.height(),
									  QColor(0, 0xff, 0xff));
			}
			if (variable.loopVariableK > 0 &&
				variable.loopVariableK < arrProd.size()) {
				auto bounding = computeProductionCellBounding(
					ctx, arrProd, variable.loopVariableK);
				ctx.painter->fillRect(bounding.left(), y + bounding.top(),
									  bounding.width(), bounding.height(),
									  QColor(0x99, 0xff, 0));
			}
		}
		ctx.painter->drawText(0, y, prod);
		y += height;
	}
}

void DemoWidget::drawimpl_LR0_FirstTable(const PaintContext &ctx) {
	const auto &variable = lr0Variable;

	QList<QStringList> table;
	QStringList head;
	QStringList firstBody;
	head << "非终结符";
	firstBody << "First 集";
	for (auto nonterminal : variable.nonterminalOrders) {
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

void DemoWidget::drawimpl_LR0_FollowTable(const PaintContext &ctx) {
	const auto &variable = lr0Variable;

	QList<QStringList> table;
	QStringList head;
	QStringList followBody;
	head << "非终结符";
	followBody << "Follow 集";
	for (auto nonterminal : variable.nonterminalOrders) {
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

void DemoWidget::drawimpl_LR0_FirstFollowTable(const PaintContext &ctx) {
	const auto &variable = lr0Variable;

	QList<QStringList> table;
	QStringList head;
	QStringList firstBody;
	QStringList followBody;
	head << "非终结符";
	firstBody << "First 集";
	followBody << "Follow 集";
	for (auto nonterminal : variable.nonterminalOrders) {
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

void DemoWidget::drawimpl_LR0_ItemClosure(const PaintContext &ctx) {
	const auto &variable = lr0Variable;
	if (variableRefresh) {
		variableRefresh = false;
		points.clear();
		edges.clear();
		computeItemLayout(ctx, variable.closureMap, variable.productions,
						  points, edges);
	}

	int height = ctx.normalFontMetrics->height();
	// 绘制点
	for (int i = 0; i < points.size(); i++) {
		auto &point = points[i];
		ctx.painter->drawRect(point.x, point.y, point.width, point.height);
		if (variable.loopVariableK == i) {
			ctx.painter->fillRect(point.x, point.y, point.width, point.height,
								  QColor(0x44, 0xff, 0x44, 0x44));
		} else if (variable.loopVariableI == i) {
			ctx.painter->fillRect(point.x, point.y, point.width, point.height,
								  QColor(0xff, 0x99, 0, 0x44));
		}
		ctx.painter->drawText(point.x, point.y + height,
							  QString("item %1").arg(i));
		auto &closure = variable.closureMap.closures[i];
		for (int j = 0; j < closure.size(); j++) {
			auto &item = closure[j];
			QString text = itemToString(item, variable.productions);
			if (variable.loopVariableJ == j && variable.loopVariableI == i) {
				ctx.painter->setPen(QColor(0xff, 0x99, 0));
			}
			ctx.painter->drawText(point.x + 8, point.y + height * (j + 2),
								  text);
			ctx.painter->setPen(QColor(0, 0, 0));
		}
	}
	// 绘制边
	ctx.painter->setFont(*ctx.smallFont);
	for (int i = 0; i < edges.size(); i++) {
		auto &edge = edges[i];
		if (edge.from == variable.loopVariableI &&
			edge.to == variable.loopVariableK) {
			ctx.painter->setPen(QColor(0xff, 0x88, 0));
		}
		ctx.painter->drawText(edge.keyPointX[0] + 8, edge.keyPointY[0] - 4,
							  variable.closureMap.edges[i].symbol);
		for (int i = 0; i < edge.keyPointCount - 1; i++) {
			ctx.painter->drawLine(edge.keyPointX[i], edge.keyPointY[i],
								  edge.keyPointX[i + 1], edge.keyPointY[i + 1]);
		}
		int x = edge.keyPointX[edge.keyPointCount - 1];
		int y = edge.keyPointY[edge.keyPointCount - 1];
		ctx.painter->drawLine(x, y, x - 8, y - 6);
		ctx.painter->drawLine(x, y, x - 8, y + 6);
		ctx.painter->setPen(QColor(0, 0, 0));
	}
	ctx.painter->setFont(*ctx.normalFont);
}

void DemoWidget::drawimpl_LR0_AutomationTable(const PaintContext &ctx) {
	const auto &variable = lr0Variable;

	int height = ctx.normalFontMetrics->height() * 1.2;
	int columnX[3];
	int columnWidth[3];
	columnX[0] = 8;
	// 第一列
	int y = height;
	ctx.painter->drawText(columnX[0], y, "状");
	columnWidth[0] = ctx.normalFontMetrics->boundingRect("状").width();
	y += height;
	ctx.painter->drawText(columnX[0], y, "态");
	columnWidth[0] = std::max(
		columnWidth[0], ctx.normalFontMetrics->boundingRect("态").width());
	for (int i = 0; i < variable.closureMap.closures.size(); i++) {
		y += height;
		QString status = QString("%1").arg(i);
		ctx.painter->drawText(columnX[0], y, status);
		columnWidth[0] =
			std::max(columnWidth[0],
					 ctx.normalFontMetrics->boundingRect(status).width());
	}
	// 第二列 Action
	columnX[1] = columnX[0] + columnWidth[0] + 16;
	QList<int> column2X, column2Width;
	auto terminals = variable.terminals;
	terminals << "$";
	terminals.sort();
	for (int i = 0; i < terminals.size(); i++) {
		if (i == 0) {
			column2X << columnX[1];
		} else {
			column2X << column2X[i - 1] + column2Width[i - 1] + 16;
		}
		column2Width << 0;
		y = height;
		auto draw = [&](QString str) {
			y += height;
			column2Width[i] =
				std::max(column2Width[i],
						 ctx.normalFontMetrics->boundingRect(str).width());
			ctx.painter->drawText(column2X[i], y, str);
		};
		draw(terminals[i]);
		for (int j = 0; j < variable.actionTable.size(); j++) {
			draw(variable.actionTable[j][terminals[i]]);
		}
	}
	y = height;
	auto actionWidth = ctx.normalFontMetrics->boundingRect("Action").width();
	int totalWidth = 0;
	for (auto w : column2Width) {
		totalWidth += w + 16;
	}
	totalWidth -= 16;
	if (totalWidth < actionWidth) {
		totalWidth = actionWidth;
	}
	ctx.painter->drawText(columnX[1] + (totalWidth - actionWidth) / 2, y,
						  "Action");
	columnWidth[1] = totalWidth;
	// 第三列 Goto
	columnX[2] = columnX[1] + columnWidth[1] + 16;
	QList<int> column3X, column3Width;
	auto nonterminals = variable.nonterminalOrders;
	for (int i = 0; i < nonterminals.size(); i++) {
		if (i == 0) {
			column3X << columnX[2];
		} else {
			column3X << column3X[i - 1] + column3Width[i - 1] + 16;
		}
		column3Width << 0;
		y = height;
		auto draw = [&](QString str) {
			y += height;
			column3Width[i] =
				std::max(column3Width[i],
						 ctx.normalFontMetrics->boundingRect(str).width());
			ctx.painter->drawText(column3X[i], y, str);
		};
		draw(nonterminals[i]);
		for (int j = 0; j < variable.gotoTable.size(); j++) {
			auto jump = variable.gotoTable[j][nonterminals[i]];
			if (jump == -1) {
				draw("");
			} else {
				draw(QString("%1").arg(jump));
			}
		}
	}
	y = height;
	auto gotoWidth = ctx.normalFontMetrics->boundingRect("Goto").width();
	totalWidth = 0;
	for (auto w : column3Width) {
		totalWidth += w + 16;
	}
	totalWidth -= 16;
	if (totalWidth < gotoWidth) {
		totalWidth = gotoWidth;
	}
	ctx.painter->drawText(columnX[2] + (totalWidth - gotoWidth) / 2, y, "Goto");
	columnWidth[2] = totalWidth;

	// 画线
	int right = columnX[2] + columnWidth[2] + 8;
	ctx.painter->drawLine(0, 8, right, 8);
	ctx.painter->drawLine(columnX[1] - 8, height + 8, right, height + 8);
	for (int i = 0; i < variable.closureMap.closures.size() + 1; i++) {
		int y = height * (i + 2);
		ctx.painter->drawLine(0, y + 8, right, y + 8);
	}
	int bottom = height * (variable.closureMap.closures.size() + 2);
	ctx.painter->drawLine(0, 8, 0, bottom + 8);
	ctx.painter->drawLine(columnX[1] - 8, 8, columnX[1] - 8, bottom + 8);
	ctx.painter->drawLine(columnX[2] - 8, 8, columnX[2] - 8, bottom + 8);
	ctx.painter->drawLine(right, 8, right, bottom + 8);
	for (int i = 1; i < column2X.size(); i++) {
		ctx.painter->drawLine(column2X[i] - 8, height + 8, column2X[i] - 8,
							  bottom + 8);
	}
	for (int i = 1; i < column3X.size(); i++) {
		ctx.painter->drawLine(column3X[i] - 8, height + 8, column3X[i] - 8,
							  bottom + 8);
	}
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

QRect DemoWidget::computeProductionCellBounding(const PaintContext &ctx,
												QStringList production,
												int index) {
	QString prod = production[0] + " :=";
	for (int i = 1; i <= index; i++) {
		prod += " " + production[i];
	}
	auto base = ctx.normalFontMetrics->boundingRect(production[index]);
	auto prefix = ctx.normalFontMetrics->boundingRect(prod);
	int width = base.width();
	int height = base.height();
	int left = prefix.right() - width;
	int top = prefix.bottom() - height;
	return QRect(left, base.top(), width, height);
}

void DemoWidget::computeItemLayout(const PaintContext &ctx,
								   const ipc::LRItemClosureMap &closureMap,
								   const QList<QStringList> &productions,
								   QList<layout::Point> &points,
								   QList<layout::Edge> &edges) {
	for (auto items : closureMap.closures) {
		layout::Point point;
		computeItemWidthAndHeight(ctx, items, point, productions);
		points << point;
	}

	for (auto goedge : closureMap.edges) {
		layout ::Edge edge;
		edge.from = goedge.from;
		edge.to = goedge.to;
		edges << edge;
	}
	int width, height;
	layout::layoutLRItemImage(points, edges, &width, &height);
}

void DemoWidget::computeItemWidthAndHeight(
	const PaintContext &ctx, ipc::LRItemClosure item, layout::Point &point,
	const QList<QStringList> &productions) {
	int height = ctx.normalFontMetrics->height() * (item.size() + 1) + 16;
	int width = 0;
	for (auto &i : item) {
		width = std::max(width, computeItemWidth(ctx, i, productions));
	}
	point.width = width;
	point.height = height;
}

int DemoWidget::computeItemWidth(const PaintContext &ctx, ipc::LRItem item,
								 const QList<QStringList> &productions) {
	if (productions.size() <= item.production || item.production < 0) {
		return 0;
	}
	QString text = itemToString(item, productions);
	auto bounding = ctx.normalFontMetrics->boundingRect(text);
	return bounding.width() + 16;
}

QString DemoWidget::itemToString(const ipc::LRItem &item,
								 const QList<QStringList> &productions) {
	auto &production = productions[item.production];
	QString prod = production[0] + " :=";
	for (int i = 1; i < production.size(); i++) {
		prod += " ";
		if (i - 1 == item.progress) {
			prod += "·";
		}
		prod += production[i];
	}
	if (production.size() - 1 == item.progress) {
		prod += " ·";
	}
	return prod;
}