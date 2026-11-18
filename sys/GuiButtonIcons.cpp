/* Small original vector symbols for Praat buttons. GPL v3 or later. */
#include "GuiButtonIcons.h"
#include <algorithm>
#include <initializer_list>

// Only reviewed words/command families. Unknown and user-defined titles stay textual.
// These identify presentation, never execution; the label/accessibility name is unchanged.
GuiButtonIcon GuiButtonIcons_forTitle (std::wstring_view title) {
	std::wstring name;
	for (wchar_t c : title) {
		if (c == L'|' || c == L'\n' || c == L'\r') break;   // old command aliases are not button labels
		if (c == L'\u202f' || c == L'\u00a0' || c == L'\t') c = L' ';
		if (c == L'&' || c == L'\u207a') continue;
		if (c >= L'A' && c <= L'Z') c += L'a' - L'A';
		if (c == L' ' && (name.empty() || name.back() == L' ')) continue;
		name += c;
	}
	while (! name.empty() && (name.back() == L' ' || name.back() == L'.' || name.back() == L'\u2026' ||
		name.back() == L'>' || name.back() == L'-' || name.back() == L'?')) name.pop_back();
	using I = GuiButtonIcon;
	struct Rule { std::wstring_view name; I icon; };
	static constexpr Rule exact [] = {
		{L"all", I::zoomAll}, {L"in", I::zoomIn}, {L"out", I::zoomOut}, {L"sel", I::zoomSelection}, {L"bak", I::back},
		{L"help", I::help}, {L"info", I::info}, {L"inspect", I::inspect}, {L"ok", I::accept}, {L"apply", I::accept},
		{L"cancel", I::cancel}, {L"close", I::cancel}, {L"stop", I::stop}, {L"pause", I::pause},
		{L"standards", I::back}, {L"revert", I::back}, {L"undo", I::back}, {L"back", I::back},
		{L"record", I::record}, {L"record mono sound", I::record}, {L"record stereo sound", I::record},
		{L"formula", I::settings}, {L"hack", I::settings}
	};
	for (const auto &rule : exact) if (name == rule.name) return rule.icon;
	if (name.size() > 5 && name.compare (name.size()-5, 5, L" help") == 0) return I::help;
	static constexpr Rule families [] = {
		{L"analyse periodicity", I::periodicity}, {L"analyze periodicity", I::periodicity},
		{L"analyse spectrum", I::spectrum}, {L"analyze spectrum", I::spectrum},
		{L"to intensity", I::intensity}, {L"to spectrum", I::spectrum}, {L"to spectrogram", I::spectrum},
		{L"to sound", I::sound}, {L"to textgrid", I::annotate}, {L"into textgrid", I::annotate},
		{L"view edit", I::edit}, {L"view", I::edit}, {L"edit", I::edit}, {L"rename", I::edit},
		{L"play", I::play}, {L"run", I::play}, {L"draw", I::draw}, {L"paint", I::draw},
		{L"query", I::query}, {L"get", I::query}, {L"count", I::query},
		{L"modify", I::settings}, {L"settings", I::settings}, {L"preferences", I::settings},
		{L"annotate", I::annotate}, {L"transcribe", I::annotate}, {L"label", I::annotate},
		{L"analyse", I::inspect}, {L"analyze", I::inspect}, {L"manipulate", I::manipulate},
		{L"convert", I::convert}, {L"cast", I::convert}, {L"to", I::convert}, {L"down to", I::convert}, {L"up to", I::convert},
		{L"filter", I::filter}, {L"combine", I::combine}, {L"concatenate", I::combine}, {L"merge", I::combine},
		{L"append", I::combine}, {L"collect", I::combine}, {L"copy", I::copy},
		{L"remove", I::remove}, {L"delete", I::remove}, {L"search", I::search}, {L"find", I::search},
		{L"open", I::open}, {L"read", I::open}, {L"browse", I::open},
		{L"save", I::save}, {L"write", I::save}, {L"export", I::save},
		{L"new", I::create}, {L"create", I::create}, {L"add", I::create},
		{L"extract", I::extract}, {L"tabulate", I::table}, {L"list", I::table}, {L"sound", I::sound},
		{L"synthesize", I::sound}, {L"synthesise", I::sound}
	};
	for (const auto &rule : families)
		if (name == rule.name || (name.size() > rule.name.size() && name.compare (0, rule.name.size(), rule.name) == 0 &&
			name [rule.name.size()] == L' ')) return rule.icon;
	return I::none;
}


#if defined (_WIN32)
void GuiButtonIcons_draw (HDC dc, GuiButtonIcon icon, RECT bounds, COLORREF colour) {
	if (icon == GuiButtonIcon::none || bounds.right <= bounds.left || bounds.bottom <= bounds.top) return;
	const int saved = SaveDC (dc);
	IntersectClipRect (dc, bounds.left, bounds.top, bounds.right, bounds.bottom);
	const int side = std::min (bounds.right - bounds.left, bounds.bottom - bounds.top);
	const auto x = [&] (int value) { return bounds.left + value * (side-1) / 16; };
	const auto y = [&] (int value) { return bounds.top + value * (side-1) / 16; };
	HPEN pen = CreatePen (PS_SOLID, std::max (1, side / 12), colour);
	SelectObject (dc, pen); SelectObject (dc, GetStockObject (NULL_BRUSH));
	SetDCBrushColor (dc, colour);
	const auto line = [&] (std::initializer_list<POINT> vertices) {
		POINT points [12]; int count = 0;
		for (const auto &v : vertices) points [count ++] = { x (v.x), y (v.y) };
		Polyline (dc, points, count);
	};
	const auto box = [&] (int l, int t, int r, int b) { Rectangle (dc, x(l), y(t), x(r)+1, y(b)+1); };
	const auto circle = [&] (int l, int t, int r, int b) { Ellipse (dc, x(l), y(t), x(r)+1, y(b)+1); };
	const auto solid = [&] (bool fill) { SelectObject (dc, GetStockObject (fill ? DC_BRUSH : NULL_BRUSH)); };
	const auto magnifier = [&] { circle (1, 1, 11, 11); line ({{10,10},{15,15}}); };
	const auto arrowRight = [&] (int yy) { line ({{2,yy},{14,yy},{10,yy-4}}); line ({{14,yy},{10,yy+4}}); };
	using I = GuiButtonIcon;
	switch (icon) {
		case I::play: { POINT points [] {{x(4),y(2)}, {x(14),y(8)}, {x(4),y(14)}}; solid(true); Polygon(dc, points, 3); } break;
		case I::stop: solid(true); box(3,3,13,13); break;
		case I::pause: solid(true); box(3,2,6,14); box(10,2,13,14); break;
		case I::record: solid(true); circle(2,2,14,14); break;
		case I::help:
			circle(1,1,15,15); line({{5,5},{6,3},{10,3},{11,5},{10,7},{8,8},{8,10}}); line({{8,12},{8,13}}); break;
		case I::query: box(1,4,15,12); line({{4,4},{4,8}}); line({{8,4},{8,10}}); line({{12,4},{12,8}}); break;
		case I::info: circle(1,1,15,15); line({{8,7},{8,12}}); line({{8,4},{8,5}}); break;
		case I::edit: line({{2,11},{10,3},{14,7},{6,15},{2,15},{2,11},{6,15}}); line({{8,5},{12,9}}); break;
		case I::draw: line({{2,1},{2,14},{15,14}}); line({{4,11},{7,6},{10,8},{14,2}}); break;
		case I::settings:
			line({{3,1},{3,15}}); line({{8,1},{8,15}}); line({{13,1},{13,15}});
			box(1,4,5,6); box(6,10,10,12); box(11,5,15,7); break;
		case I::annotate: line({{1,2},{15,2},{15,11},{8,11},{4,15},{4,11},{1,11},{1,2}});
			line({{4,5},{12,5}}); line({{4,8},{10,8}}); break;
		case I::periodicity: line({{1,8},{3,3},{5,8},{7,13},{9,8},{11,3},{13,8},{15,13}}); break;
		case I::sound: line({{1,8},{3,8},{5,3},{7,14},{9,1},{11,12},{13,6},{15,8}}); break;
		case I::spectrum: line({{1,15},{15,15}}); box(2,9,4,14); box(7,2,9,14); box(12,6,14,14); break;
		case I::intensity: solid(true); box(2,10,4,14); box(7,6,9,14); box(12,2,14,14); break;
		case I::manipulate: line({{2,12},{8,4},{14,9}}); box(1,11,3,13); box(7,3,9,5); box(13,8,15,10); break;
		case I::convert: line({{1,4},{14,4},{11,1}}); line({{14,4},{11,7}});
			line({{15,12},{2,12},{5,9}}); line({{2,12},{5,15}}); break;
		case I::filter: line({{1,2},{15,2},{10,8},{10,13},{6,15},{6,8},{1,2}}); break;
		case I::combine: line({{1,2},{6,2},{9,8},{15,8},{12,5}}); line({{15,8},{12,11}}); line({{1,14},{6,14},{9,8}}); break;
		case I::copy: box(1,1,10,11); box(5,5,15,15); break;
		case I::inspect: case I::search: magnifier(); break;
		case I::remove: line({{2,4},{14,4}}); line({{6,3},{6,1},{10,1},{10,3}});
			line({{3,6},{4,15},{12,15},{13,6}}); line({{6,7},{6,12}}); line({{10,7},{10,12}}); break;
		case I::open: line({{1,14},{1,3},{6,3},{8,5},{15,5},{15,14},{1,14}}); line({{2,8},{14,8}}); break;
		case I::save: box(1,1,15,15); box(4,1,11,6); box(4,10,12,15); break;
		case I::create: box(2,1,13,15); line({{5,8},{11,8}}); line({{8,5},{8,11}}); break;
		case I::accept: line({{2,8},{6,12},{14,3}}); break;
		case I::cancel: line({{3,3},{13,13}}); line({{3,13},{13,3}}); break;
		case I::back: line({{7,2},{1,7},{7,12}}); line({{1,7},{11,7},{14,10},{14,14}}); break;
		case I::zoomIn: case I::zoomOut: magnifier(); line({{3,6},{9,6}});
			if (icon == I::zoomIn) line({{6,3},{6,9}}); break;
		case I::zoomAll: line({{1,3},{1,13}}); line({{15,3},{15,13}});
			line({{6,5},{3,8},{13,8},{10,5}}); line({{3,8},{6,11}}); line({{13,8},{10,11}}); break;
		case I::zoomSelection: line({{5,2},{2,2},{2,14},{5,14}}); line({{11,2},{14,2},{14,14},{11,14}});
			line({{6,8},{10,8}}); line({{8,6},{8,10}}); break;
		case I::extract: line({{6,2},{1,2},{1,14},{6,14}}); arrowRight(8); break;
		case I::table: box(1,1,15,15); line({{1,6},{15,6}}); line({{1,11},{15,11}}); line({{6,1},{6,15}}); break;
		case I::none: case I::count: break;
	}
	RestoreDC (dc, saved); DeleteObject (pen);
}
#endif
