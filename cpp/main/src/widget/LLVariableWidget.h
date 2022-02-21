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
	void translateDefault();

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

	// paintProductionList 安全宽度（右侧可渲染）
	int productionSafeWidth;

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
	void paintComputeFirst(const PaintContext &ctx);
	void paintComputeFollow(const PaintContext &ctx);

	void paintNonterminalOrder(const PaintContext &ctx);
	void paintProductionList(const PaintContext &ctx, bool autoDisableColor);
	void paintProductionOrder(const PaintContext &ctx);
	void paintFirstTable(const PaintContext &ctx);
	void paintFollowTable(const PaintContext &ctx);

	QRect computeProductionCellBounding(const PaintContext &ctx,
										QStringList production, int index);
};