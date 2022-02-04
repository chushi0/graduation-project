#include "LLVariableWidget.h"
#include <QColor>
#include <QPainter>

LLVariableWidget::LLVariableWidget(QWidget *parent)
	: QWidget(parent), variableValid(false), x(0), y(0) {
}

LLVariableWidget::~LLVariableWidget() {
}

void LLVariableWidget::setVariableAndPoint(
	const ipc::LLBreakpointVariables &variable, const ipc::Breakpoint &point) {
	this->variable = variable;
	this->point = point;
	this->variableValid = true;

	update();
}

void LLVariableWidget::paintEvent(QPaintEvent *) {
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

	int height = normalFontMetrics.height();
	int y = height;
	painter.drawText(0, y, "非终结符顺序：");
	y += height * 2;

	int x = 0;
	for (int i = 0; i < variable.nonterminalOrders.size(); i++) {
		auto &nonterminal = variable.nonterminalOrders[i];
		auto bounding = normalFontMetrics.boundingRect(nonterminal);
		if (variable.loopVariableI == i + 1) {
			painter.setPen(QColor(0xff, 0x99, 0));
			auto iBounding = smallFontMetrics.boundingRect("循环变量：i");
			painter.setFont(smallFont);
			painter.drawText(x + (bounding.width() - iBounding.width()) / 2,
							 y - height, "循环变量：i");
			painter.fillRect(x + bounding.left(), y + bounding.top(),
							 bounding.width(), bounding.height(),
							 QColor(0xff, 0x99, 0));
			painter.setPen(QColor(0, 0, 0));
			painter.setFont(normalFont);
		} else if (variable.loopVariableJ == i + 1) {
			painter.setPen(QColor(0, 0xff, 0xff));
			auto iBounding = smallFontMetrics.boundingRect("循环变量：j");
			painter.setFont(smallFont);
			painter.drawText(x + (bounding.width() - iBounding.width()) / 2,
							 y + height, "循环变量：j");
			painter.fillRect(x + bounding.left(), y + bounding.top(),
							 bounding.width(), bounding.height(),
							 QColor(0, 0xff, 0xff));
			painter.setPen(QColor(0, 0, 0));
			painter.setFont(normalFont);
		}
		painter.drawText(x, y, nonterminal);
		x += bounding.width() + 20;
	}
	y += height;

	painter.restore();
}