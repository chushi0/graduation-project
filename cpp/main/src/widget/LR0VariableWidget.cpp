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

	paintLR0ItemClosure(ctx);

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

void LR0VariableWidget::paintLR0ItemClosure(const PaintContext &ctx) {
	int height = ctx.normalFontMetrics->height();
	// 绘制点
	for (int i = 0; i < points.size(); i++) {
		auto &point = points[i];
		ctx.painter->drawRect(point.x, point.y, point.width, point.height);
		ctx.painter->drawText(point.x, point.y + height,
							  QString("item %1").arg(i));
		auto &closure = variable.closureMap.closures[i];
		for (int j = 0; j < closure.size(); j++) {
			auto &item = closure[j];
			auto prod = variable.productions[item.production];
			QString text = prod[0] + " :=";
			for (int i = 1; i < prod.size(); i++) {
				text += " " + prod[i];
			}
			ctx.painter->drawText(point.x + 8, point.y + height * (j + 2),
								  text);
		}
	}
	// 绘制边
	ctx.painter->setFont(*ctx.smallFont);
	for (int i = 0; i < edges.size(); i++) {
		auto &edge = edges[i];
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
	}
	ctx.painter->setFont(*ctx.normalFont);
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
	auto prod = variable.productions[item.production];
	if (prod.size() == 0) {
		return 0;
	}
	QString text = prod[0] + " :=";
	for (int i = 1; i < prod.size(); i++) {
		text += " " + prod[i];
	}
	auto bounding = ctx.normalFontMetrics->boundingRect(text);
	return bounding.width() + 16;
}