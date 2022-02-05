#pragma once

#include "../ipc/types.h"
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
	bool isNonterminalEqual(int index, QString nonterminal);
	bool isRemoveProduction(QStringList prod);
};