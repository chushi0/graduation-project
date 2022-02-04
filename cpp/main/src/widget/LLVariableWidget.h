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

private:
	ipc::LLBreakpointVariables variable;
	ipc::Breakpoint point;
	bool variableValid;

	float x, y;
};