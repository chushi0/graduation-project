#pragma once
#include <QLabel>

class ClickableLabel : public QLabel {
	Q_OBJECT

public:
	explicit ClickableLabel(QWidget *parnet = nullptr);
	~ClickableLabel();

signals:
	void clicked();

protected:
	virtual void mousePressEvent(QMouseEvent *ev) override;
};