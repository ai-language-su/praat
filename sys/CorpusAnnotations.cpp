#include "CorpusAnnotations.h"
#include "CorpusFiles.h"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <random>
#include <set>
#if defined (_WIN32)
	#include <windows.h>
#endif

namespace CorpusAnnotations {
namespace fs = std::filesystem;
namespace {

constexpr char templateHeader [] = "Praat Annotation Template 1\n";

autoTextGrid readGrid (const fs::path &path) {
	structMelderFile file {};
	Melder_pathToFile (Melder_peek8to32_u (path.u8string().c_str()), & file);
	autoDaata object = Data_readFromFile (& file);
	Melder_require (object && object -> classInfo == classTextGrid, U"This file is not a TextGrid.");
	return object.static_cast_move<structTextGrid>();
}

template<typename Visit>
void visitLabels (Function tier, Visit visit) {
	if (tier -> classInfo == classIntervalTier) {
		const auto intervals = static_cast<IntervalTier> (tier);
		for (integer i = 1; i <= intervals -> intervals.size; ++ i) {
			const auto interval = intervals -> intervals.at [i];
			if (interval -> text && *interval -> text.get())
				visit (i, interval -> text.get(), interval -> xmin, interval -> xmax);
		}
	} else if (tier -> classInfo == classTextTier) {
		const auto points = static_cast<TextTier> (tier);
		for (integer i = 1; i <= points -> points.size; ++ i) {
			const auto point = points -> points.at [i];
			if (point -> mark && *point -> mark.get())
				visit (i, point -> mark.get(), point -> number, point -> number);
		}
	}
}
conststring32 tierName (Function tier) { return tier -> name ? tier -> name.get() : U""; }

} // namespace

SearchResult search (const fs::path &folder, conststring32 label, conststring32 tierFilter,
	bool exact, bool matchCase, GridLookup lookup)
{
	Melder_require (label && *label, U"Enter a label to find.");
	const auto groups = CorpusFiles_scan (folder);
	const auto criterion = exact ? kMelder_string::EQUAL_TO : kMelder_string::CONTAINS;
	SearchResult result;
	integer total = 0, processed = 0;
	for (const auto &group : groups) total += (integer) group.textGrids.size();
	if (! total) return result;
	autoMelderProgress progress (U"Searching TextGrids");
	for (const auto &group : groups) {
		const auto recording = group.recordings.size() == 1 ? group.recordings.front() : fs::path {};
		conststring32 recordingIssue = group.recordings.empty() ? U"Recording missing" :
			group.recordings.size() > 1 ? U"Several recordings share this name" : U"";
		for (const auto &path : group.textGrids) {
			try {
				Melder_progress (double (processed++) / total, U"Searching ", Melder_peek8to32_u (path.generic_u8string().c_str()));
			} catch (MelderError) {
				if (! str32str (Melder_getError(), U"Interrupted!")) throw;
				Melder_clearError ();
				result.cancelled = true;
				return result;
			}
			try {
				const auto absolute = folder / path;
				TextGrid grid = lookup ? lookup (absolute) : nullptr;
				autoTextGrid loaded;
				if (! grid) {
					loaded = readGrid (absolute);
					grid = loaded.get();
				}
				for (integer i = 1; i <= grid -> tiers -> size; ++ i) {
					const auto tier = grid -> tiers -> at [i];
					const auto name = tierName (tier);
					if (tierFilter && *tierFilter && ! str32equ (name, tierFilter)) continue;
					visitLabels (tier, [&] (integer annotation, conststring32 text, double start, double end) {
						if (Melder_stringMatchesCriterion (text, criterion, label, matchCase))
							result.hits.push_back ({ path, recording, name, text, recordingIssue, i, annotation, start, end });
					});
				}
				++ result.gridsSearched;
			} catch (MelderError) {
				result.errors.emplace_back (Melder_cat (Melder_peek8to32_u (path.generic_u8string().c_str()), U": ", Melder_getError()));
				Melder_clearError ();
			}
		}
	}
	return result;
}

bool stillMatches (TextGrid grid, const Hit &hit) {
	if (hit.tier < 1 || hit.tier > grid -> tiers -> size) return false;
	const auto tier = grid -> tiers -> at [hit.tier];
	if (hit.tierName != tierName (tier)) return false;
	if (tier -> classInfo == classIntervalTier) {
		const auto intervals = static_cast<IntervalTier> (tier);
		const auto i = IntervalTier_timeToIndex (intervals, hit.start);
		if (! i) return false;
		const auto interval = intervals -> intervals.at [i];
		return interval -> xmin == hit.start && interval -> xmax == hit.end && interval -> text && hit.label == interval -> text.get();
	}
	if (tier -> classInfo == classTextTier && hit.start == hit.end) {
		const auto points = static_cast<TextTier> (tier);
		const auto i = AnyTier_hasPoint (points -> asAnyTier(), hit.start);
		return i && points -> points.at [i] -> mark && hit.label == points -> points.at [i] -> mark.get();
	}
	return false;
}

Template parseTemplate (conststring32 body) {
	const std::u32string text = body ? body : U"";
	Template layout;
	std::set<std::u32string> names;
	for (size_t offset = 0, number = 1; offset < text.size(); ++ number) {
		const auto end = text.find (U'\n', offset);
		auto line = text.substr (offset, end == text.npos ? end : end - offset);
		offset = end == text.npos ? text.size() : end + 1;
		if (! line.empty() && line.back() == U'\r') line.pop_back();
		const auto first = line.find_first_not_of (U" \t");
		if (first == line.npos || line [first] == U'#') continue;
		if (line [first] == U'=') {
			const auto label = line.substr (first + 1);
			Melder_require (! layout.empty() && Melder_findInk (label.c_str()), U"Line ", number, U": Enter a tier before its labels and use a nonempty label.");
			auto &labels = layout.back().labels;
			Melder_require (std::find (labels.begin(), labels.end(), label) == labels.end(), U"Line ", number, U": Duplicate allowed label.");
			labels.push_back (label);
			continue;
		}
		const bool point = line.compare (first, 7, U"point: ") == 0;
		const bool interval = line.compare (first, 10, U"interval: ") == 0;
		Melder_require (point || interval, U"Line ", number, U": Start a tier with ‘interval: ’ or ‘point: ’.");
		const auto name = line.substr (first + (point ? 7 : 10));
		Melder_require (Melder_findInk (name.c_str()) && names.insert (name).second, U"Line ", number, U": Use a nonempty, unique tier name.");
		layout.push_back ({ name, point, {} });
	}
	Melder_require (! layout.empty(), U"Add at least one tier to the template.");
	return layout;
}

std::u32string templateBody (const Template &layout) {
	std::u32string body;
	for (const auto &tier : layout) {
		Melder_require (tier.name.find_first_of (U"\r\n") == tier.name.npos, U"A template tier name must fit on one line.");
		body += (tier.point ? U"point: " : U"interval: ") + tier.name + U'\n';
		for (const auto &label : tier.labels) {
			Melder_require (label.find_first_of (U"\r\n") == label.npos, U"An allowed label must fit on one line.");
			body += U"  =" + label + U'\n';
		}
	}
	return body;
}

Template readTemplate (const fs::path &path) {
	std::ifstream file (path, std::ios::binary);
	Melder_require (file.is_open(), U"Could not open the annotation template.");
	file.exceptions (std::ios::badbit);
	std::string text (1024 * 1024 + 1, '\0');
	file.read (text.data(), text.size());
	text.resize (file.gcount());
	Melder_require (text.size() <= 1024 * 1024 && text.find ('\0') == text.npos && Melder_str8IsValidUtf8 (text.c_str()),
		U"An annotation template must be a UTF-8 text file no larger than 1 MiB.");
	if (text.compare (0, 3, "\xEF\xBB\xBF") == 0) text.erase (0, 3);
	const auto newline = text.find ('\n');
	Melder_require (newline != text.npos, U"Missing annotation template header.");
	const auto header = text.substr (0, newline);
	Melder_require (header + '\n' == templateHeader || header + '\n' == "Praat Annotation Template 1\r\n", U"Unsupported annotation template format.");
	return parseTemplate (Melder_peek8to32_u (text.c_str() + newline + 1));
}

void writeTemplate (const fs::path &path, const Template &layout, bool replace) {
	const auto body = templateBody (layout);
	parseTemplate (body.c_str());
	const std::string text = std::string (templateHeader) + Melder_peek32to8 (body.c_str());
	Melder_require (text.size() <= 1024 * 1024, U"The template is larger than 1 MiB.");
	Melder_require (replace || ! fs::exists (path), U"An annotation template with that name already exists.");
	const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
	const auto staging = path.parent_path() / (".praat-template-" + std::to_string (stamp) + "-" + std::to_string (std::random_device{}()));
	Melder_require (fs::create_directory (staging), U"Could not create the temporary template folder.");
	const auto pending = staging / "template";
	try {
		std::ofstream file (pending, std::ios::binary);
		file.exceptions (std::ios::failbit | std::ios::badbit);
		file << text;
		file.close();
		#if defined (_WIN32)
			Melder_require (MoveFileExW (pending.c_str(), path.c_str(), MOVEFILE_WRITE_THROUGH | (replace ? MOVEFILE_REPLACE_EXISTING : 0)),
				U"Could not save the annotation template (Windows error ", (integer) GetLastError (), U").");
		#else
			if (replace) fs::rename (pending, path);
			else fs::create_hard_link (pending, path);
		#endif
	} catch (...) {
		std::error_code ignored;
		fs::remove (pending, ignored);
		fs::remove (staging, ignored);
		throw;
	}
	std::error_code ignored;
	fs::remove (pending, ignored);
	fs::remove (staging, ignored);
}

Template fromGrid (TextGrid grid, bool collectLabels) {
	Template layout;
	for (integer i = 1; i <= grid -> tiers -> size; ++ i) {
		const auto original = grid -> tiers -> at [i];
		Tier tier { tierName (original), original -> classInfo == classTextTier, {} };
		if (collectLabels) {
			std::set<std::u32string> seen;
			visitLabels (original, [&] (integer, conststring32 label, double, double) {
				if (Melder_findInk (label) && seen.insert (label).second) tier.labels.emplace_back (label);
			});
		}
		layout.push_back (std::move (tier));
	}
	return layout;
}

autoTextGrid createGrid (const Template &layout, double start, double end) {
	Melder_require (! layout.empty() && std::isfinite (start) && std::isfinite (end) && start < end,
		U"Choose a template and a recording with a positive duration.");
	auto grid = TextGrid_createWithoutTiers (start, end);
	for (const auto &entry : layout) {
		autoFunction tier;
		if (entry.point) tier = TextTier_create (start, end);
		else tier = IntervalTier_create (start, end);
		Thing_setName (tier.get(), entry.name.c_str());
		TextGrid_addTier_move (grid.get(), tier.move());
	}
	return grid;
}

std::vector<std::u32string> validateGrid (TextGrid grid, const Template &layout) {
	std::vector<std::u32string> issues;
	for (size_t i = 0; i < layout.size() || i < (size_t) grid -> tiers -> size; ++ i) {
		if (i >= layout.size()) {
			issues.emplace_back (Melder_cat (U"Unexpected tier: ", tierName (grid -> tiers -> at [i + 1])));
			continue;
		}
		const auto &expected = layout [i];
		const auto actual = i < (size_t) grid -> tiers -> size ? grid -> tiers -> at [i + 1] : nullptr;
		if (! actual || tierName (actual) != expected.name || (actual -> classInfo == classTextTier) != expected.point) {
			issues.emplace_back (Melder_cat (U"Tier ", i + 1, U" should be ", expected.point ? U"point “" : U"interval “", expected.name.c_str(), U"”."));
			continue;
		}
		if (! expected.labels.empty())
			visitLabels (actual, [&] (integer, conststring32 label, double start, double) {
				if (Melder_findInk (label) && std::find (expected.labels.begin(), expected.labels.end(), label) == expected.labels.end())
					issues.emplace_back (Melder_cat (expected.name.c_str(), U" at ", Melder_double (start), U" s: label “", label, U"” is outside the vocabulary."));
			});
	}
	return issues;
}

} // namespace CorpusAnnotations
