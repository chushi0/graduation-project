#pragma once
#include <QList>

namespace layout {
	static constexpr int MAX_KEY_POINT = 6;
	struct Point {
		int x;		  // out
		int y;		  // out
		int ptr;	  // in out
		int width;	  // in out
		int height;	  // in out
		int depth;	  // internal
		int index;	  // internal
		int inCount;  // internal
		int outCount; // internal
		bool unused;  // out
	};
	struct Edge {
		int from;					  // in
		int to;						  // in
		int textX;					  // out
		int textY;					  // out
		int textWidth;				  // in out
		int textHeight;				  // in out
		int keyPointX[MAX_KEY_POINT]; // out
		int keyPointY[MAX_KEY_POINT]; // out
		int keyPointCount;			  // out
	};

	void layoutLRItemImage(QList<Point> &points, QList<Edge> &edges,
						   int *imgWidth, int *imgHeight);

} // namespace layout