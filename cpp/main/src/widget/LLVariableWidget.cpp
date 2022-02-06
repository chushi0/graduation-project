#include "LLVariableWidget.h"
#include <QColor>
#include <QMouseEvent>

LLVariableWidget::LLVariableWidget(QWidget *parent)
	: QWidget(parent), variableValid(false), x(0), y(0), mousePressed(false) {
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

void LLVariableWidget::paintEvent(QPaintEvent *event) {
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

	if (point.name == "RemoveLeftRecusion") {
		paintRemoveLeftRecusion(ctx);
	} else if (point.name == "ExtractCommonPrefix") {
		paintExtractCommonPrefix(ctx);
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
	for (auto rp : variable.replaceProduction) {
		if (prod == rp.original) {
			return true;
		}
	}
	return false;
}

QList<QStringList> LLVariableWidget::showAfterProductionDraw(QStringList prod) {
	for (auto rp : variable.replaceProduction) {
		if (prod == rp.original) {
			QList<QStringList> resp;
			resp << rp.replace;
			return resp;
		}
	}
	for (auto rmProd : variable.removeProductions) {
		if (rmProd == prod) {
			return variable.addProductions;
		}
	}
	return QList<QStringList>();
}

void LLVariableWidget::mousePressEvent(QMouseEvent *event) {
	mousePressed = true;
	lastMouseX = event->x();
	lastMouseY = event->y();
}

void LLVariableWidget::mouseReleaseEvent(QMouseEvent *) {
	mousePressed = false;
}

void LLVariableWidget::mouseMoveEvent(QMouseEvent *event) {
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

void LLVariableWidget::paintRemoveLeftRecusion(const PaintContext &ctx) {
	paintNonterminalOrder(ctx);
	int height = ctx.normalFontMetrics->height();
	int y = height * 5;
	ctx.painter->translate(0, y);
	paintProductionList(ctx, true);
}

void LLVariableWidget::paintExtractCommonPrefix(const PaintContext &ctx) {
	paintNonterminalOrder(ctx);
	int height = ctx.normalFontMetrics->height();
	int y = height * 4;
	ctx.painter->translate(0, y);
	paintProductionList(ctx, true);
}

void LLVariableWidget::paintNonterminalOrder(const PaintContext &ctx) {
	int height = ctx.normalFontMetrics->height();
	int y = height;
	ctx.painter->drawText(0, y, "非终结符顺序：");
	y += height * 2;

	int x = 0;
	for (int i = 0; i < variable.nonterminalOrders.size(); i++) {
		auto &nonterminal = variable.nonterminalOrders[i];
		auto bounding = ctx.normalFontMetrics->boundingRect(nonterminal);
		if (variable.loopVariableI == i + 1) {
			ctx.painter->setPen(QColor(0xff, 0x99, 0));
			auto iBounding = ctx.smallFontMetrics->boundingRect("循环变量：i");
			ctx.painter->setFont(*ctx.smallFont);
			ctx.painter->drawText(x + (bounding.width() - iBounding.width()) /
										  2,
								  y - height, "循环变量：i");
			ctx.painter->fillRect(x + bounding.left(), y + bounding.top(),
								  bounding.width(), bounding.height(),
								  QColor(0xff, 0x99, 0));
			ctx.painter->setPen(QColor(0, 0, 0));
			ctx.painter->setFont(*ctx.normalFont);
		} else if (variable.loopVariableJ == i + 1) {
			ctx.painter->setPen(QColor(0, 0xff, 0xff));
			auto iBounding = ctx.smallFontMetrics->boundingRect("循环变量：j");
			ctx.painter->setFont(*ctx.smallFont);
			ctx.painter->drawText(x + (bounding.width() - iBounding.width()) /
										  2,
								  y + height, "循环变量：j");
			ctx.painter->fillRect(x + bounding.left(), y + bounding.top(),
								  bounding.width(), bounding.height(),
								  QColor(0, 0xff, 0xff));
			ctx.painter->setPen(QColor(0, 0, 0));
			ctx.painter->setFont(*ctx.normalFont);
		}
		ctx.painter->drawText(x, y, nonterminal);
		x += bounding.width() + 20;
	}
}

void LLVariableWidget::paintProductionList(const PaintContext &ctx,
										   bool autoDisableColor) {
	int height = ctx.normalFontMetrics->height();
	int y = 0;
	ctx.painter->drawText(0, y, "产生式：");
	y += height;
	bool addDrawBefore = variable.removeProductions.isEmpty() &&
						 !variable.addProductions.isEmpty();
	for (auto arrProd : variable.productions) {
		if (autoDisableColor &&
			isNonterminalEqual(variable.loopVariableI, arrProd[0])) {
			ctx.painter->setPen(QColor(0x80, 0x80, 0x80));
		}
		if (addDrawBefore &&
			isNonterminalEqual(variable.loopVariableI - 1, arrProd[0])) {
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
		} else if (isNonterminalEqual(variable.loopVariableI - 1, arrProd[0])) {
			ctx.painter->fillRect(bounding.left(), y + bounding.top(),
								  bounding.width(), bounding.height(),
								  QColor(0xff, 0x99, 0, 0x80));
			if (isProdPrefixEqual(arrProd)) {
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
		}
		ctx.painter->drawText(0, y, prod);
		if (isRemoveProduction(arrProd)) {
			ctx.painter->setPen(QColor(0, 0, 0));
			auto lineY = y + bounding.top() + bounding.height() / 2;
			ctx.painter->drawLine(0, lineY, bounding.width(), lineY);
		}
		auto paintAfter = showAfterProductionDraw(arrProd);
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

bool LLVariableWidget::isProdPrefixEqual(QStringList prod) {
	if (variable.commonPrefix.isEmpty()) {
		return false;
	}
	if (prod.size() < variable.commonPrefix.size()) {
		return false;
	}
	for (int i = 0; i < variable.commonPrefix.size(); i++) {
		if (variable.commonPrefix[i] != prod[i + 1]) {
			return false;
		}
	}
	return true;
}