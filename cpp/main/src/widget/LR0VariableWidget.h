#pragma once

#include "../ipc/types.h"
#include "../util/layout.h"
#include <QPainter>
#include <QWidget>

class LR0VariableWidget : public QWidget {
	Q_OBJECT

public:
	LR0VariableWidget(QWidget *parent);
	~LR0VariableWidget();

	void setVariableAndPoint(const ipc::LR0BreakpointVariables &variable,
							 const ipc::Breakpoint &point);
	void translateDefault();

protected:
	virtual void paintEvent(QPaintEvent *) override;
	virtual void mousePressEvent(QMouseEvent *) override;
	virtual void mouseReleaseEvent(QMouseEvent *) override;
	virtual void mouseMoveEvent(QMouseEvent *) override;

private:
	ipc::LR0BreakpointVariables variable;
	ipc::Breakpoint point;
	bool variableValid;

	float x, y;
	float lastMouseX, lastMouseY;
	bool mousePressed;
	// paintProductionList 安全宽度（右侧可渲染）
	int productionSafeWidth;
	// paintLR0ItemClosure 宽高
	int itemClosureWidth, itemClosureHeight;

	QList<layout::Point> points;
	QList<layout::Edge> edges;

private:
	struct PaintContext {
		QPainter *painter;
		QFont *normalFont, *smallFont;
		QFontMetrics *normalFontMetrics, *smallFontMetrics;
	};

	void paintComputeFirst(const PaintContext &ctx);
	void paintComputeFollow(const PaintContext &ctx);
	void paintComputeItemClosure(const PaintContext &ctx);
	void paintLRGenerateAutomaton(const PaintContext &ctx);
	void paintSLRGenerateAutomaton(const PaintContext &ctx);

	void paintProductionOrder(const PaintContext &ctx);
	void paintFirstTable(const PaintContext &ctx);
	void paintFollowTable(const PaintContext &ctx);
	void paintLR0ItemClosure(const PaintContext &ctx);

	void computeItemLayout(const PaintContext &ctx,
						   QList<layout::Point> &points,
						   QList<layout::Edge> &edges);
	void computeItemWidthAndHeight(const PaintContext &ctx,
								   ipc::LRItemClosure item,
								   layout::Point &point);
	int computeItemWidth(const PaintContext &ctx, ipc::LRItem item);
	QRect computeProductionCellBounding(const PaintContext &ctx,
										QStringList production, int index);
	QString getItemString(const ipc::LRItem &item);
};