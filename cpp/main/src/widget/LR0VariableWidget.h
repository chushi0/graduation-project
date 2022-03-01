#pragma once

#include "../ipc/types.h"
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

private:
	struct PaintContext {
		QPainter *painter;
		QFont *normalFont, *smallFont;
		QFontMetrics *normalFontMetrics, *smallFontMetrics;
	};
};