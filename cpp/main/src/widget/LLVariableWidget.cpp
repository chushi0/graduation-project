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
	y += height + height;

	painter.drawText(0, y, "产生式：");
	y += height;
	for (auto arrProd : variable.productions) {
		if (isNonterminalEqual(variable.loopVariableI, arrProd[0])) {
			painter.setPen(QColor(0x80, 0x80, 0x80));
		}
		QString prod = arrProd[0] + " :=";
		for (int i = 1; i < arrProd.size(); i++) {
			prod += " " + arrProd[i];
		}
		auto bounding = normalFontMetrics.boundingRect(prod);
		if (variable.currentProcessProduction == arrProd) {
			painter.fillRect(bounding.left(), y + bounding.top(),
							 bounding.width(), bounding.height(),
							 QColor(0xff, 0x99, 0));
		} else if (isNonterminalEqual(variable.loopVariableI - 1, arrProd[0])) {
			painter.fillRect(bounding.left(), y + bounding.top(),
							 bounding.width(), bounding.height(),
							 QColor(0xff, 0x99, 0, 0x80));
		} else if (isNonterminalEqual(variable.loopVariableJ - 1, arrProd[0])) {
			painter.fillRect(bounding.left(), y + bounding.top(),
							 bounding.width(), bounding.height(),
							 QColor(0, 0xff, 0xff, 0x80));
		}
		painter.drawText(0, y, prod);
		if (isRemoveProduction(arrProd)) {
			painter.setPen(QColor(0, 0, 0));
			painter.drawLine(0, y - bounding.height() / 2, bounding.width(),
							 y - bounding.height() / 2);
			painter.setPen(QColor(0, 0xff, 0, 0xaa));
			for (auto arrProd : variable.addProductions) {
				y += height;
				QString prod = arrProd[0] + " :=";
				for (int i = 1; i < arrProd.size(); i++) {
					prod += " " + arrProd[i];
				}
				painter.drawText(0, y, prod);
			}
		}
		y += height;
	}

	painter.restore();
}

bool LLVariableWidget::isNonterminalEqual(int index, QString nonterminal) {
	if (index < 0 || index >= variable.nonterminalOrders.size()) {
		return false;
	}
	return variable.nonterminalOrders[index] == nonterminal;
}

bool LLVariableWidget::isRemoveProduction(QStringList prod) {
	for (auto rmProd : variable.removeProductions) {
		if (rmProd == prod) {
			return true;
		}
	}
	return false;
}