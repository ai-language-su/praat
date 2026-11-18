#include "../sys/GuiHoverHelp_geometry.h"
#include <cassert>
#include <iostream>
struct Region { GuiHoverHelpRect bounds; const char *name; };
int main () {
	using A = GuiHoverHelpAlignment;
	for (double scale : { 1.0, 1.5, 2.0 }) {
		const int x = int (200 * scale), y = int (120 * scale);
		const auto leftOfAxis = GuiHoverHelp_labelBounds (x, y, 80 * scale, 18 * scale, A::END, A::CENTRE);
		assert (leftOfAxis.contains (x - int (40 * scale), y));
		assert (! leftOfAxis.contains (x + 5, y));
		const auto rightOfAxis = GuiHoverHelp_labelBounds (x, y, 80 * scale, 18 * scale, A::START, A::CENTRE);
		assert (rightOfAxis.contains (x + int (40 * scale), y));
		assert (! rightOfAxis.contains (x - 5, y));
		const auto centred = GuiHoverHelp_labelBounds (x, y, 80 * scale, 18 * scale, A::CENTRE, A::END);
		assert (centred.contains (x, y - int (9 * scale)));
		assert (! centred.contains (x, y + 5));
	}
	std::vector<Region> regions {
		{ { 0, 0, 1000, 600 }, "analysis background" },
		{ { 900, 10, 980, 35 }, "pitch scale" },
		{ { 810, 10, 890, 35 }, "intensity scale" }
	};
	assert (GuiHoverHelp_hit (regions, 950, 22) == 1);
	assert (GuiHoverHelp_hit (regions, 850, 22) == 2);
	assert (GuiHoverHelp_hit (regions, 500, 300) == 0);
	assert (GuiHoverHelp_hit (regions, 1200, 300) == -1);
	// An equal-sized later readout wins over stale/overlapping registrations.
	regions.push_back ({ { 900, 10, 980, 35 }, "current pitch readout" });
	assert (GuiHoverHelp_hit (regions, 950, 22) == 3);
	regions.clear();
	assert (GuiHoverHelp_hit (regions, 950, 22) == -1);
	std::cout << "Hover label geometry and overlapping-scale checks passed\n";
}
