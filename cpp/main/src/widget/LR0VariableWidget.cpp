#include "LR0VariableWidget.h"
#include <QColor>
#include <QMouseEvent>
#include <cmath>
#include <sstream>

static constexpr auto EMPTY_TERMINAL = "ε";
static constexpr auto EOF_TERMINAL = "$";

LR0VariableWidget::LR0VariableWidget(QWidget *parent)
	: QWidget(parent), variableValid(false), x(0), y(0), mousePressed(false) {
}

LR0VariableWidget::~LR0VariableWidget() {
}

void LR0VariableWidget::setVariableAndPoint(
	const ipc::LR0BreakpointVariables &variable, const ipc::Breakpoint &point) {
	this->variable = variable;
	this->point = point;
	this->variableValid = true;

	// 布局
	points.clear();
	edges.clear();
	auto normalFont = font();
	auto normalFontMetrics = QFontMetrics(normalFont);
	PaintContext ctx;
	ctx.normalFont = &normalFont;
	ctx.normalFontMetrics = &normalFontMetrics;
	computeItemLayout(ctx, points, edges);

	update();
}

void LR0VariableWidget::translateDefault() {
	x = 0;
	y = 0;

	update();
}

void LR0VariableWidget::paintEvent(QPaintEvent *event) {
	QPainter painter(this);
	if (!variableValid) {
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

	if (point.name == "ComputeFirstSet") {
		paintComputeFirst(ctx);
	} else if (point.name == "ComputeFollowSet") {
		paintComputeFollow(ctx);
	} else if (point.name == "ComputeItemClosure") {
		paintComputeItemClosure(ctx);
	} else if (point.name == "GenerateLR0Automaton") {
		paintLRGenerateAutomaton(ctx);
	} else if (point.name == "GenerateSLRAutomaton") {
		paintSLRGenerateAutomaton(ctx);
	}

	painter.restore();
}

void LR0VariableWidget::mousePressEvent(QMouseEvent *event) {
	mousePressed = true;
	lastMouseX = event->x();
	lastMouseY = event->y();
}

void LR0VariableWidget::mouseReleaseEvent(QMouseEvent *event) {
	mousePressed = false;
}

void LR0VariableWidget::mouseMoveEvent(QMouseEvent *event) {
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

void LR0VariableWidget::paintComputeFirst(const PaintContext &ctx) {
	paintProductionOrder(ctx);
	paintFirstTable(ctx);
}

void LR0VariableWidget::paintComputeFollow(const PaintContext &ctx) {
	paintProductionOrder(ctx);
	paintFollowTable(ctx);
}

void LR0VariableWidget::paintComputeItemClosure(const PaintContext &ctx) {
	paintLR0ItemClosure(ctx, 0, 0);
}

void LR0VariableWidget::paintLRGenerateAutomaton(const PaintContext &ctx) {
	paintLR0ItemClosure(ctx, 0, 0);
	paintAutomatonTable(ctx, itemClosureWidth + 16, 0);
}

void LR0VariableWidget::paintSLRGenerateAutomaton(const PaintContext &ctx) {
	paintFollowTable(ctx);
	paintLR0ItemClosure(ctx, followWidth, 0);
	paintAutomatonTable(ctx, itemClosureWidth + followWidth + 16, 0);
}

void LR0VariableWidget::paintProductionOrder(const PaintContext &ctx) {
	productionSafeWidth = 0;
	int height = ctx.normalFontMetrics->height();
	int y = height;
	ctx.painter->drawText(0, y, "产生式：");
	y += height;
	for (int i = 0; i < variable.productions.size(); i++) {
		auto arrProd = variable.productions[i];
		QString prod = arrProd[0] + " :=";
		for (int i = 1; i < arrProd.size(); i++) {
			prod += " " + arrProd[i];
		}
		auto bounding = ctx.normalFontMetrics->boundingRect(prod);
		if (variable.loopVariableI == i) {
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
		productionSafeWidth = std::max(productionSafeWidth, bounding.width());
		ctx.painter->drawText(0, y, prod);
		y += height;
	}
}

void LR0VariableWidget::paintFirstTable(const PaintContext &ctx) {
	int left = productionSafeWidth + 16 + 8;
	int height = ctx.normalFontMetrics->height() + 8;
	int width = 0;
	int y = height;
	auto nonterminalsText = "非终结符";
	ctx.painter->drawText(left, y, nonterminalsText);
	width = ctx.normalFontMetrics->boundingRect(nonterminalsText).width();
	for (auto nonterminal : variable.nonterminalOrders) {
		y += height;
		auto bounding = ctx.normalFontMetrics->boundingRect(nonterminal);
		width = std::max(width, bounding.width());
		ctx.painter->drawText(left, y, nonterminal);
	}
	int x = left + width + 16;
	y = height;
	int w2 = 0;
	auto firstSetText = "First 集";
	ctx.painter->drawText(x, y, firstSetText);
	w2 = ctx.normalFontMetrics->boundingRect(firstSetText).width();
	for (auto nonterminal : variable.nonterminalOrders) {
		y += height;
		auto firsts = variable.firstSet[nonterminal];
		QString firstText = "";
		for (auto first : firsts) {
			if (first == "") {
				first = EMPTY_TERMINAL;
			}
			firstText += first + "  ";
		}
		auto bounding = ctx.normalFontMetrics->boundingRect(firstText);
		w2 = std::max(w2, bounding.width());
		ctx.painter->drawText(x, y, firstText);
	}
	ctx.painter->drawLine(left - 8, 4, left - 8,
						  (variable.nonterminalOrders.size() + 1) * height + 4);
	ctx.painter->drawLine(x - 8, 4, x - 8,
						  (variable.nonterminalOrders.size() + 1) * height + 4);
	ctx.painter->drawLine(x + w2 + 8, 4, x + w2 + 8,
						  (variable.nonterminalOrders.size() + 1) * height + 4);
	y = 0;
	for (int i = 0; i < variable.nonterminalOrders.size() + 2; i++) {
		ctx.painter->drawLine(left - 8, y + 4, x + w2 + 8, y + 4);
		y += height;
	}
}

void LR0VariableWidget::paintFollowTable(const PaintContext &ctx) {
	int left = productionSafeWidth + 16 + 8;
	int height = ctx.normalFontMetrics->height() + 8;
	int width = 0;
	int y = height;
	auto nonterminalsText = "非终结符";
	ctx.painter->drawText(left, y, nonterminalsText);
	width = ctx.normalFontMetrics->boundingRect(nonterminalsText).width();
	for (auto nonterminal : variable.nonterminalOrders) {
		y += height;
		auto bounding = ctx.normalFontMetrics->boundingRect(nonterminal);
		width = std::max(width, bounding.width());
		ctx.painter->drawText(left, y, nonterminal);
	}
	int x = left + width + 16;
	y = height;
	int w2 = 0;
	auto firstSetText = "First 集";
	ctx.painter->drawText(x, y, firstSetText);
	w2 = ctx.normalFontMetrics->boundingRect(firstSetText).width();
	for (auto nonterminal : variable.nonterminalOrders) {
		y += height;
		auto firsts = variable.firstSet[nonterminal];
		QString firstText = "";
		for (auto first : firsts) {
			if (first == "") {
				first = EMPTY_TERMINAL;
			}
			firstText += first + "  ";
		}
		auto bounding = ctx.normalFontMetrics->boundingRect(firstText);
		w2 = std::max(w2, bounding.width());
		ctx.painter->drawText(x, y, firstText);
	}
	int x2 = x + w2 + 16;
	y = height;
	int w3 = 0;
	auto followSetText = "Follow 集";
	ctx.painter->drawText(x2, y, followSetText);
	w3 = ctx.normalFontMetrics->boundingRect(followSetText).width();
	for (auto nonterminal : variable.nonterminalOrders) {
		y += height;
		auto follows = variable.followSet[nonterminal];
		QString followText = "";
		for (auto follow : follows) {
			if (follow == "$") {
				follow = EOF_TERMINAL;
			}
			followText += follow + " ";
		}
		auto bounding = ctx.normalFontMetrics->boundingRect(followText);
		w3 = std::max(w3, bounding.width());
		ctx.painter->drawText(x2, y, followText);
	}
	ctx.painter->drawLine(left - 8, 4, left - 8,
						  (variable.nonterminalOrders.size() + 1) * height + 4);
	ctx.painter->drawLine(x - 8, 4, x - 8,
						  (variable.nonterminalOrders.size() + 1) * height + 4);
	ctx.painter->drawLine(x2 - 8, 4, x2 - 8,
						  (variable.nonterminalOrders.size() + 1) * height + 4);
	ctx.painter->drawLine(x2 + w3 + 8, 4, x2 + w3 + 8,
						  (variable.nonterminalOrders.size() + 1) * height + 4);
	y = 0;
	for (int i = 0; i < variable.nonterminalOrders.size() + 2; i++) {
		ctx.painter->drawLine(left - 8, y + 4, x + w2 + 8, y + 4);
		ctx.painter->drawLine(left - 8, y + 4, x2 + w3 + 8, y + 4);
		y += height;
	}
	followWidth = x2 + w3 + 8;
	followHeight = y;
}

void LR0VariableWidget::paintLR0ItemClosure(const PaintContext &ctx, int left,
											int top) {
	left += 8;
	top += 8;
	int height = ctx.normalFontMetrics->height();
	// 绘制点
	for (int i = 0; i < points.size(); i++) {
		auto &point = points[i];
		ctx.painter->drawRect(left + point.x, top + point.y, point.width,
							  point.height);
		if (variable.loopVariableK == i) {
			ctx.painter->fillRect(left + point.x, top + point.y, point.width,
								  point.height, QColor(0x44, 0xff, 0x44, 0x44));
		} else if (variable.loopVariableI == i) {
			ctx.painter->fillRect(left + point.x, top + point.y, point.width,
								  point.height, QColor(0xff, 0x99, 0, 0x44));
		}
		ctx.painter->drawText(left + point.x, top + point.y + height,
							  QString("item %1").arg(i));
		auto &closure = variable.closureMap.closures[i];
		for (int j = 0; j < closure.size(); j++) {
			auto &item = closure[j];
			QString text = getItemString(item);
			if (variable.loopVariableJ == j && variable.loopVariableI == i) {
				ctx.painter->setPen(QColor(0xff, 0x99, 0));
			}
			ctx.painter->drawText(left + point.x + 8,
								  top + point.y + height * (j + 2), text);
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
		ctx.painter->drawText(left + edge.keyPointX[0] + 8,
							  top + edge.keyPointY[0] - 4,
							  variable.closureMap.edges[i].symbol);
		for (int i = 0; i < edge.keyPointCount - 1; i++) {
			ctx.painter->drawLine(
				left + edge.keyPointX[i], top + edge.keyPointY[i],
				left + edge.keyPointX[i + 1], top + edge.keyPointY[i + 1]);
		}
		int x = edge.keyPointX[edge.keyPointCount - 1];
		int y = edge.keyPointY[edge.keyPointCount - 1];
		ctx.painter->drawLine(left + x, top + y, left + x - 8, top + y - 6);
		ctx.painter->drawLine(left + x, top + y, left + x - 8, top + y + 6);
		ctx.painter->setPen(QColor(0, 0, 0));
	}
	ctx.painter->setFont(*ctx.normalFont);
}

void LR0VariableWidget::paintAutomatonTable(const PaintContext &ctx, int left,
											int top) {
	int height = ctx.normalFontMetrics->height() * 1.2;
	int columnX[3];
	int columnWidth[3];
	columnX[0] = left + 8;
	// 第一列
	int y = top + height;
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
		y = top + height;
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
	y = top + height;
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
		y = top + height;
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
	y = top + height;
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
	ctx.painter->drawLine(left, top + 8, right, top + 8);
	ctx.painter->drawLine(columnX[1] - 8, top + height + 8, right,
						  top + height + 8);
	for (int i = 0; i < variable.closureMap.closures.size() + 1; i++) {
		int y = top + height * (i + 2);
		ctx.painter->drawLine(left, y + 8, right, y + 8);
	}
	int bottom = top + height * (variable.closureMap.closures.size() + 2);
	ctx.painter->drawLine(left, top + 8, left, bottom + 8);
	ctx.painter->drawLine(columnX[1] - 8, top + 8, columnX[1] - 8, bottom + 8);
	ctx.painter->drawLine(columnX[2] - 8, top + 8, columnX[2] - 8, bottom + 8);
	ctx.painter->drawLine(right, top + 8, right, bottom + 8);
	for (int i = 1; i < column2X.size(); i++) {
		ctx.painter->drawLine(column2X[i] - 8, top + height + 8,
							  column2X[i] - 8, bottom + 8);
	}
	for (int i = 1; i < column3X.size(); i++) {
		ctx.painter->drawLine(column3X[i] - 8, top + height + 8,
							  column3X[i] - 8, bottom + 8);
	}
}

void LR0VariableWidget::computeItemLayout(const PaintContext &ctx,
										  QList<layout::Point> &points,
										  QList<layout::Edge> &edges) {
	for (auto items : variable.closureMap.closures) {
		layout::Point point;
		computeItemWidthAndHeight(ctx, items, point);
		points << point;
	}

	for (auto goedge : variable.closureMap.edges) {
		layout ::Edge edge;
		edge.from = goedge.from;
		edge.to = goedge.to;
		edges << edge;
	}
	int width, height;
	layout::layoutLRItemImage(points, edges, &width, &height);
	itemClosureWidth = width;
	itemClosureHeight = height;
}

void LR0VariableWidget::computeItemWidthAndHeight(const PaintContext &ctx,
												  ipc::LRItemClosure item,
												  layout::Point &point) {
	int height = ctx.normalFontMetrics->height() * (item.size() + 1) + 16;
	int width = 0;
	for (auto &i : item) {
		width = std::max(width, computeItemWidth(ctx, i));
	}
	point.width = width;
	point.height = height;
}

int LR0VariableWidget::computeItemWidth(const PaintContext &ctx,
										ipc::LRItem item) {
	if (variable.productions.size() <= item.production || item.production < 0) {
		return 0;
	}
	QString text = getItemString(item);
	auto bounding = ctx.normalFontMetrics->boundingRect(text);
	return bounding.width() + 16;
}

QRect LR0VariableWidget::computeProductionCellBounding(const PaintContext &ctx,
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

QString LR0VariableWidget::getItemString(const ipc::LRItem &item) {
	auto &production = variable.productions[item.production];
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