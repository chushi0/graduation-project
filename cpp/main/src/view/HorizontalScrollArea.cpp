#include "HorizontalScrollArea.h"
#include <QEnterEvent>
#include <QEvent>
#include <QScrollBar>

HorizontalScrollArea::HorizontalScrollArea(QWidget *parent)
	: QScrollArea(parent) {
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	horizontalScrollBar()->setStyleSheet("height:8px;");
}

void HorizontalScrollArea::enterEvent(QEnterEvent *) {
	setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
}

void HorizontalScrollArea::leaveEvent(QEvent *) {
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}