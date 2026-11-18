#ifndef _CorpusAnnotations_h_
#define _CorpusAnnotations_h_

#include "TextGrid.h"
#include <filesystem>
#include <vector>

namespace CorpusAnnotations {

struct Tier {
	std::u32string name;
	bool point;
	std::vector<std::u32string> labels;
};
using Template = std::vector<Tier>;

struct Hit {
	std::filesystem::path grid, recording;
	std::u32string tierName, label, recordingIssue;
	integer tier, annotation;
	double start, end;
};
struct SearchResult {
	std::vector<Hit> hits;
	std::vector<std::u32string> errors;
	integer gridsSearched = 0;
	bool cancelled = false;
};
using GridLookup = TextGrid (*) (const std::filesystem::path &path);

SearchResult search (const std::filesystem::path &folder, conststring32 label, conststring32 tier,
	bool exact, bool matchCase, GridLookup lookup = nullptr);
bool stillMatches (TextGrid grid, const Hit &hit);
Template parseTemplate (conststring32 body);
std::u32string templateBody (const Template &layout);
Template readTemplate (const std::filesystem::path &path);
void writeTemplate (const std::filesystem::path &path, const Template &layout, bool replace);
Template fromGrid (TextGrid grid, bool collectLabels);
autoTextGrid createGrid (const Template &layout, double start, double end);
std::vector<std::u32string> validateGrid (TextGrid grid, const Template &layout);

} // namespace CorpusAnnotations

void CorpusSearchWindow_show ();
void AnnotationTemplatesWindow_show ();

#endif
