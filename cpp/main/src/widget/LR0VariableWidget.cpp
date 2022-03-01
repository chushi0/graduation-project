#include "LR0VariableWidget.h"
#include <QColor>
#include <QMouseEvent>
#include <cmath>

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