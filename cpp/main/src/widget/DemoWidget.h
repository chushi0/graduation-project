#pragma once

#include "../ipc/types.h"
#include "../util/layout.h"
#include <QWidget>

class DemoWidget : public QWidget {
public:
	enum Mode {
		None,
		LL_ProductionList,
		LL_FirstTable,
		LL_FollowTable,
		LL_FirstFollowTable,
		LL_SelectTable,
		LL_AutomationTable,
		LR0_ProductionList,
		LR0_FirstTable,
		LR0_FollowTable,
		LR0_FirstFollowTable,
		LR0_ItemClosure,
		LR0_AutomationTable,
	};

public:
	explicit DemoWidget(QWidget *parent = nullptr);
	~DemoWidget();

	void setMode(Mode mode);
	void setVariableAndPoint(const ipc::LLBreakpointVariables &variable,
							 const ipc::Breakpoint &point);
	void setVariableAndPoint(const ipc::LR0BreakpointVariables &variable,
							 const ipc::Breakpoint &point);
	void setVariableAndPoint(const ipc::LR1BreakpointVariables &variable,
							 const ipc::Breakpoint &point);
	void translateDefault();

protected:
	virtual void paintEvent(QPaintEvent *) override;
	virtual void mousePressEvent(QMouseEvent *) override;
	virtual void mouseReleaseEvent(QMouseEvent *) override;
	virtual void mouseMoveEvent(QMouseEvent *) override;

private:
	ipc::LLBreakpointVariables llVariable;
	ipc::LR0BreakpointVariables lr0Variable;
	ipc::LR1BreakpointVariables lr1Variable;
	ipc::Breakpoint point;
	Mode mode;
	bool variableRefresh;

	float x, y;
	float lastMouseX, lastMouseY;
	bool mousePressed;

	// LR0 ItemClosure cache

	QList<layout::Point> points;
	QList<layout::Edge> edges;

private:
	struct PaintContext {
		QPainter *painter;
		QFont *normalFont, *smallFont;
		QFontMetrics *normalFontMetrics, *smallFontMetrics;
	};

private:
	void drawimpl_LL_ProductionList(const PaintContext &ctx);
	void drawimpl_LL_FirstTable(const PaintContext &ctx);
	void drawimpl_LL_FollowTable(const PaintContext &ctx);
	void drawimpl_LL_FirstFollowTable(const PaintContext &ctx);
	void drawimpl_LL_SelectTable(const PaintContext &ctx);
	void drawimpl_LL_AutomationTable(const PaintContext &ctx);
	void drawimpl_LR0_ProductionList(const PaintContext &ctx);
	void drawimpl_LR0_FirstTable(const PaintContext &ctx);
	void drawimpl_LR0_FollowTable(const PaintContext &ctx);
	void drawimpl_LR0_FirstFollowTable(const PaintContext &ctx);
	void drawimpl_LR0_ItemClosure(const PaintContext &ctx);
	void drawimpl_LR0_AutomationTable(const PaintContext &ctx);

	void paintTable(const PaintContext &ctx, int x, int y,
					QList<QStringList> content,
					QStringList warnText = QStringList());
	int paintTableColumn(const PaintContext &ctx, int x, int y,
						 QStringList content,
						 QStringList warnText = QStringList());

	bool isNonterminalEqual(int index, QString nonterminal,
							QStringList nonterminalList);
	bool isProdPrefixEqual(QStringList prod, QStringList prefix);
	bool isRemoveProduction(QStringList prod, QList<QStringList> removeList,
							QList<ipc::ReplaceProduction> replaceList);
	QList<QStringList>
		showAfterProductionDraw(QStringList prod, QList<QStringList> removeList,
								QList<ipc::ReplaceProduction> replaceList,
								QList<QStringList> addProduction);
	QString prodToString(QStringList prod);
	QRect computeProductionCellBounding(const PaintContext &ctx,
										QStringList production, int index);
	void computeItemLayout(const PaintContext &ctx,
						   const ipc::LRItemClosureMap &closureMap,
						   const QList<QStringList> &productions,
						   QList<layout::Point> &points,
						   QList<layout::Edge> &edges);
	void computeItemWidthAndHeight(const PaintContext &ctx,
								   ipc::LRItemClosure item,
								   layout::Point &point,
								   const QList<QStringList> &productions);
	int computeItemWidth(const PaintContext &ctx, ipc::LRItem item,
						 const QList<QStringList> &productions);
	QString itemToString(const ipc::LRItem &item,
						 const QList<QStringList> &productions);
};