#ifndef _GuiHoverHelp_geometry_h_
#define _GuiHoverHelp_geometry_h_
#include <algorithm>
#include <cmath>
#include <vector>
struct GuiHoverHelpRect {
	int left, top, right, bottom;
	bool contains (int x, int y) const { return x >= left && x < right && y >= top && y < bottom; }
};
enum class GuiHoverHelpAlignment { START, CENTRE, END };
template <typename Region>
int GuiHoverHelp_hit (const std::vector<Region> &regions, int x, int y) {
	int result = -1;
	double smallest = 1e100;
	for (size_t i = 0; i < regions.size(); ++ i) if (regions [i].bounds.contains (x, y)) {
		const auto &r = regions [i].bounds;
		const double area = double (r.right - r.left) * (r.bottom - r.top);
		if (area <= smallest) { smallest = area; result = int (i); }
	}
	return result;
}
inline GuiHoverHelpRect GuiHoverHelp_labelBounds (double x, double y, double width, double height,
	GuiHoverHelpAlignment horizontal, GuiHoverHelpAlignment vertical)
{
	if (horizontal == GuiHoverHelpAlignment::CENTRE) x -= width / 2;
	if (horizontal == GuiHoverHelpAlignment::END) x -= width;
	if (vertical == GuiHoverHelpAlignment::CENTRE) y -= height / 2;
	if (vertical == GuiHoverHelpAlignment::END) y -= height;
	return { int (std::floor (x)) - 3, int (std::floor (y)) - 3,
		int (std::ceil (x + width)) + 3, int (std::ceil (y + height)) + 3 };
}
#endif
