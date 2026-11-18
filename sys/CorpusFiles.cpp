#include "CorpusFiles.h"
#include <algorithm>
#include <map>

namespace fs = std::filesystem;

static std::u32string folded (const fs::path &path) {
	std::u32string text = Melder_peek8to32_u (path.generic_u8string().c_str());
	for (auto &c : text) c = Melder_toLowerCase (c);
	return text;
}

std::vector<CorpusFileGroup> CorpusFiles_scan (const fs::path &folder) {
	Melder_require (fs::is_directory (folder), U"Choose a corpus folder.");
	std::map<std::u32string, CorpusFileGroup> matches;
	for (const auto &entry : fs::recursive_directory_iterator (folder)) {
		if (! entry.is_regular_file()) continue;
		const auto relative = entry.path().lexically_relative (folder);
		const auto suffix = folded (relative.extension());
		std::u32string key = Melder_peek8to32_u (relative.parent_path().generic_u8string().c_str());
		if (! key.empty()) key += U'/';
		key += folded (relative.stem());
		if (suffix == U".textgrid")
			matches [key].textGrids.push_back (relative);
		else if (suffix == U".wav" || suffix == U".flac" || suffix == U".aiff" || suffix == U".aif" ||
			suffix == U".aifc" || suffix == U".mp3" || suffix == U".ogg" || suffix == U".opus")
			matches [key].recordings.push_back (relative);
	}
	std::vector<CorpusFileGroup> groups;
	for (auto &entry : matches) {
		auto &group = entry.second;
		std::sort (group.recordings.begin(), group.recordings.end());
		std::sort (group.textGrids.begin(), group.textGrids.end());
		groups.push_back (std::move (group));
	}
	return groups;
}
