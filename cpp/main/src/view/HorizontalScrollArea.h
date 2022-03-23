#pragma once

#include <QScrollArea>

class HorizontalScrollArea : public QScrollArea {
	Q_OBJECT
public:
	explicit HorizontalScrollArea(QWidget *parent);

protected:
	virtual void enterEvent(QEnterEvent *) override;
	virtual void leaveEvent(QEvent *) override;
};