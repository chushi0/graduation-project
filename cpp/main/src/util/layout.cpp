#include "layout.h"
#include <QSet>
#include <cmath>

static constexpr int GRID_SIZE = 24;

typedef struct {
	QList<layout::Point> *points;
	QList<layout::Edge> *edges;
	QList<int> *depthX;
	QList<int> *depthY;
	int *topY;
} E, &Env;

inline int normalize(int v) {
	return (v + GRID_SIZE - 1) / GRID_SIZE;
}

inline int unnormalize(int v) {
	return v * GRID_SIZE;
}

static void shiftRight(Env env, int fromX, int c = 1) {
	for (auto &point : (*env.points)) {
		if (point.x > fromX) {
			point.x += c;
		}
	}
	for (auto &edge : (*env.edges)) {
		for (int i = 0; i < edge.keyPointCount; i++) {
			if (edge.keyPointX[i] > fromX) {
				edge.keyPointX[i] += c;
			}
		}
	}
	for (auto &x : (*env.depthX)) {
		if (x != -1 && x > fromX) {
			x += c;
		}
	}
}

static void addPoint(Env env, layout::Point &point) {
	point.x = (*env.depthX)[point.depth];
	point.y = (*env.depthY)[point.depth];
	(*env.depthY)[point.depth] += point.height + 1;

	int xmin = point.x + point.width + 3;
	if ((*env.depthX)[point.depth + 1] == -1) {
		(*env.depthX)[point.depth + 1] = xmin;
	} else if ((*env.depthX)[point.depth + 1] < xmin) {
		shiftRight(env, point.x + 1, xmin - (*env.depthX)[point.depth + 1]);
	}
}

static void getOutPoint(Env env, int point, int &x, int &y) {
	auto &p = (*env.points)[point];
	int out = p.outCount;
	p.outCount++;
	x = p.x + p.width;
	y = p.y + out;
}

static void getInPoint(Env env, int point, int &x, int &y) {
	auto &p = (*env.points)[point];
	int in = p.inCount;
	p.inCount++;
	x = p.x;
	y = p.y + in;
}

// true - dont need shift
// false - need shift
// fromX < toX
static bool findSpaceVertical(Env env, int fromX, int toX, int fromY, int toY,
							  int &outX) {
	QSet<int> set;
	for (int i = fromX; i <= toX; i++) {
		set << i;
	}
	// make sure fromY < toY
	if (toY < fromY) {
		std::swap(toY, fromY);
	}
	for (auto &edge : (*env.edges)) {
		if (edge.keyPointCount == 4) {
			if (set.contains(edge.keyPointX[1])) {
				int y1 = edge.keyPointY[1];
				int y2 = edge.keyPointY[2];
				// make sure y1 < y2
				if (y1 > y2) {
					std::swap(y1, y2);
				}
				if (toY >= y1 && fromY <= y2) {
					set.remove(edge.keyPointX[1]);
				}
			}
		} else if (edge.keyPointCount == 6) {
			if (set.contains(edge.keyPointX[1])) {
				int y1 = edge.keyPointY[1];
				int y2 = edge.keyPointY[2];
				if (y1 > y2) {
					std::swap(y1, y2);
				}
				if (toY >= y1 && fromY <= y2) {
					set.remove(edge.keyPointX[1]);
				}
			}
			if (set.contains(edge.keyPointX[3])) {
				int y1 = edge.keyPointY[3];
				int y2 = edge.keyPointY[4];
				if (y1 > y2) {
					std::swap(y1, y2);
				}
				if (toY >= y1 && fromY <= y2) {
					set.remove(edge.keyPointX[3]);
				}
			}
		}
	}
	if (set.isEmpty()) {
		return false;
	}
	outX = *set.begin();
	return true;
}

// from < to
static void findSpaceHorizontal(Env env, int fromX, int toX, int &outY) {
	QSet<int> set;
	for (int i = *env.topY; i < 0; i++) {
		set << i;
	}
	for (auto &edge : (*env.edges)) {
		if (edge.keyPointCount == 6) {
			if (set.contains(edge.keyPointY[2])) {
				int x1 = edge.keyPointX[3];
				int x2 = edge.keyPointX[2];
				if (fromX <= x2 && toX >= x1) {
					set.remove(edge.keyPointY[2]);
				}
			}
		}
	}
	if (set.isEmpty()) {
		(*env.topY)--;
		outY = *env.topY;
	} else {
		outY = *set.begin();
	}
}

static void lineToRight(Env env, layout::Edge &edge, int ox, int oy, int ix,
						int iy) {
	if (ox == ix) {
		edge.keyPointCount = 2;
		edge.keyPointX[0] = ox;
		edge.keyPointX[1] = ix;
		edge.keyPointY[0] = oy;
		edge.keyPointY[1] = iy;
		return;
	}

	int vx;
	if (!findSpaceVertical(env, ox + 1, ix - 1, oy, iy, vx)) {
		shiftRight(env, ox);
		ix++;
		findSpaceVertical(env, ox + 1, ix - 1, oy, iy, vx);
	}

	edge.keyPointCount = 4;
	edge.keyPointX[0] = ox;
	edge.keyPointX[1] = vx;
	edge.keyPointX[2] = vx;
	edge.keyPointX[3] = ix;
	edge.keyPointY[0] = oy;
	edge.keyPointY[1] = oy;
	edge.keyPointY[2] = iy;
	edge.keyPointY[3] = iy;
}

static void lineToTopLeft(Env env, layout::Edge &edge, int ox, int oy, int ix,
						  int iy) {
	int x1, y1, x2;

	int xmin;
	if ((*env.points)[edge.to].depth == 0) {
		xmin = -1;
		shiftRight(env, -1);
		ox++;
		ix++;
	} else {
		xmin = (*env.depthX)[(*env.points)[edge.to].depth - 1];
		int width = 0;
		for (auto &point : *env.points) {
			width = std::max(width, point.width);
		}
		xmin += width;
	}

	if (!findSpaceVertical(env, xmin + 1, ix - 1, 0, iy, x2)) {
		shiftRight(env, xmin);
		ix++;
		ox++;
		findSpaceVertical(env, xmin + 1, ix - 1, 0, iy, x2);
	}

	int xmax = (*env.depthX)[(*env.points)[edge.from].depth + 1];
	if (xmax == -1) {
		xmax = (*env.depthX)[(*env.points)[edge.from].depth] + 3;
		(*env.depthX)[(*env.points)[edge.from].depth + 1] = xmax;
	}

	if (!findSpaceVertical(env, ox + 1, xmax - 1, 0, oy, x1)) {
		shiftRight(env, ox);
		xmax += 1;
		findSpaceVertical(env, ox + 1, xmax - 1, 0, oy, x1);
	}

	findSpaceHorizontal(env, x2, x1, y1);

	edge.keyPointCount = 6;
	edge.keyPointX[0] = ox;
	edge.keyPointX[1] = x1;
	edge.keyPointX[2] = x1;
	edge.keyPointX[3] = x2;
	edge.keyPointX[4] = x2;
	edge.keyPointX[5] = ix;
	edge.keyPointY[0] = oy;
	edge.keyPointY[1] = oy;
	edge.keyPointY[2] = y1;
	edge.keyPointY[3] = y1;
	edge.keyPointY[4] = iy;
	edge.keyPointY[5] = iy;
}

static void addEdge(Env env, layout::Edge &edge) {
	int outX, outY, inX, inY;
	getOutPoint(env, edge.from, outX, outY);
	getInPoint(env, edge.to, inX, inY);
	if (outX < inX) {
		lineToRight(env, edge, outX, outY, inX, inY);
	} else {
		lineToTopLeft(env, edge, outX, outY, inX, inY);
	}
}

static void computeWidthAndHeight(Env env, int *width, int *height) {
	int minY = 0, maxY = 0;
	int maxX = 0;
	for (auto &point : (*env.points)) {
		maxX = std::max(maxX, point.x + point.width);
		minY = std::min(minY, point.y);
		maxY = std::max(maxY, point.y + point.height);
	}
	for (auto &edge : (*env.edges)) {
		for (int i = 0; i < edge.keyPointCount; i++) {
			maxX = std::max(maxX, edge.keyPointX[i]);
			minY = std::min(minY, edge.keyPointY[i]);
			maxY = std::max(maxY, edge.keyPointY[i]);
		}
	}

	*width = maxX;
	*height = maxY - minY;
}

void layout::layoutLRItemImage(QList<Point> &points, QList<Edge> &edges,
							   int *width, int *height) {
	if (points.size() == 0) {
		*width = 0;
		*height = 0;
		return;
	}

	// 缩放到计算用的大小，初始化内部变量
	for (auto &point : points) {
		point.width = normalize(point.width);
		point.height = normalize(point.height);
		point.depth = -1;
		point.unused = true;
		point.inCount = 0;
		point.outCount = 0;
		point.x = -1;
		point.y = -1;
	}
	for (auto &edge : edges) {
		edge.textWidth = normalize(edge.textWidth);
		edge.textHeight = normalize(edge.textHeight);
		edge.keyPointCount = 0;
	}

	// 确定每个点的相对位置
	points[0].depth = 0;
	points[0].index = 0;
	points[0].unused = false;
	int curDepth = 0;
	int curIndex = 0;
	int depthCount = 0;
loop:
	for (int i = 0; i < points.size(); i++) {
		const auto &point = points[i];
		if (point.depth != curDepth || point.index != curIndex) {
			continue;
		}
		for (const auto &edge : edges) {
			if (edge.from != i) {
				continue;
			}
			auto &p = points[edge.to];
			if (p.depth == -1) {
				p.depth = curDepth + 1;
				p.index = depthCount;
				p.unused = false;
				depthCount++;
			}
		}
		curIndex++;
		goto loop;
	}
	if (depthCount > 0) {
		curDepth++;
		curIndex = 0;
		depthCount = 0;
		goto loop;
	}
	depthCount = curDepth + 1;

	// 重新计算各点高度
	for (auto &edge : edges) {
		points[edge.from].outCount++;
		points[edge.to].inCount++;
	}
	for (auto &point : points) {
		point.height =
			std::max(point.height, std::max(point.inCount, point.outCount));
		point.inCount = point.outCount = 0;
	}

	// 按顺序布局
	QList<int> depthX, depthY;
	E env;
	env.points = &points;
	env.edges = &edges;
	env.depthX = &depthX;
	env.depthY = &depthY;
	(*env.depthX).resize(depthCount + 1);
	(*env.depthY).resize(depthCount + 1);
	(*env.depthX).fill(-1);
	(*env.depthX)[0] = 0;
	int topY = 0;
	env.topY = &topY;
	curDepth = 0;
	curIndex = 0;
layout:
	for (int i = 0; i < points.size(); i++) {
		auto &point = points[i];
		if (point.depth != curDepth || point.index != curIndex) {
			continue;
		}

		if (point.x == -1) {
			addPoint(env, point);
		}

		for (auto &edge : edges) {
			if (edge.from != i) {
				continue;
			}
			if ((*env.points)[edge.to].x == -1) {
				addPoint(env, (*env.points)[edge.to]);
			}
			addEdge(env, edge);
		}

		curIndex++;
		goto layout;
	}
	curDepth++;
	curIndex = 0;
	if (curDepth < depthCount) {
		goto layout;
	}

	// 恢复原大小
	for (auto &point : points) {
		point.width = unnormalize(point.width);
		point.height = unnormalize(point.height);
		point.x = unnormalize(point.x);
		point.y = unnormalize(point.y);
	}
	for (auto &edge : edges) {
		edge.textWidth = normalize(edge.textWidth);
		edge.textHeight = normalize(edge.textHeight);
		for (int i = 0; i < edge.keyPointCount; i++) {
			edge.keyPointX[i] = unnormalize(edge.keyPointX[i]);
			edge.keyPointY[i] = unnormalize(edge.keyPointY[i]) + GRID_SIZE / 2;
		}
	}

	computeWidthAndHeight(env, width, height);
}
