#pragma once

#include "../ipc/types.h"
#include <QPainter>
#include <QWidget>

class LLVariableWidget : public QWidget {
	Q_OBJECT

public:
	LLVariableWidget(QWidget *parent);
	~LLVariableWidget();

	void setVariableAndPoint(const ipc::LLBreakpointVariables &variable,
							 const ipc::Breakpoint &point);

protected:
	virtual void paintEvent(QPaintEvent *) override;
	virtual void mousePressEvent(QMouseEvent *) override;
	virtual void mouseReleaseEvent(QMouseEvent *) override;
	virtual void mouseMoveEvent(QMouseEvent *) override;

private:
	ipc::LLBreakpointVariables variable;
	ipc::Breakpoint point;
	bool variableValid;

	float x, y;
	float lastMouseX, lastMouseY;
	bool mousePressed;

private:
	struct PaintContext {
		QPainter *painter;
		QFont *normalFont, *smallFont;
		QFontMetrics *normalFontMetrics, *smallFontMetrics;
	};

private:
	bool isNonterminalEqual(int index, QString nonterminal);
	bool isProdPrefixEqual(QStringList prod);
	bool isRemoveProduction(QStringList prod);
	QList<QStringList> showAfterProductionDraw(QStringList prod);

	void paintRemoveLeftRecusion(const PaintContext &ctx);
	void paintExtractCommonPrefix(const PaintContext &ctx);

	void paintNonterminalOrder(const PaintContext &ctx);
	void paintProductionList(const PaintContext &ctx, bool autoDisableColor);
};