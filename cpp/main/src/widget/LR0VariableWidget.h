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

	QList<layout::Point> points;
	QList<layout::Edge> edges;

private:
	struct PaintContext {
		QPainter *painter;
		QFont *normalFont, *smallFont;
		QFontMetrics *normalFontMetrics, *smallFontMetrics;
	};

	void paintLR0ItemClosure(const PaintContext &ctx);

	void computeItemLayout(const PaintContext &ctx,
						   QList<layout::Point> &points,
						   QList<layout::Edge> &edges);
	void computeItemWidthAndHeight(const PaintContext &ctx,
								   ipc::LRItemClosure item,
								   layout::Point &point);
	int computeItemWidth(const PaintContext &ctx, ipc::LRItem item);
};