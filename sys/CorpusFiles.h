#ifndef _CorpusFiles_h_
#define _CorpusFiles_h_

#include "melder.h"
#include <filesystem>
#include <vector>

struct CorpusFileGroup {
	// Paths are relative to the corpus directory. Each group is scoped to one folder.
	std::vector<std::filesystem::path> recordings, textGrids;
};

std::vector<CorpusFileGroup> CorpusFiles_scan (const std::filesystem::path &folder);

#endif
