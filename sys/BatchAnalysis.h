#ifndef _BatchAnalysis_h_
#define _BatchAnalysis_h_

#include "melder.h"
#include <filesystem>
#include <vector>

namespace BatchAnalysis {

struct Preset {
	std::u32string tier = U"vowels", label;
	bool duration = true, formants = true, pitch = false;
	double formantCeiling = 5500.0, pitchFloor = 50.0, pitchTop = 800.0;
};

struct Pair {
	// Relative to the input folder, so repeated basenames remain distinguishable.
	std::filesystem::path audio, grid;
	std::u32string problem;
	integer matchingIntervals = 0;
};

struct Row {
	integer pair, interval;
	std::u32string label, problem;
	double start, end, f1 = undefined, f2 = undefined, f0 = undefined;
};

struct Result {
	std::vector<Row> rows;
	std::vector<std::pair<integer, std::u32string>> failures;
	integer attempted = 0, succeeded = 0;
	bool cancelled = false;
};

void validate (const Preset &preset);
double parseNumber (conststring32 text, conststring32 label);
std::string presetText (const Preset &preset);
Preset loadPreset (const std::filesystem::path &path);
void savePreset (const Preset &preset, const std::filesystem::path &path, bool replace);
std::vector<Pair> scan (const std::filesystem::path &folder, const Preset &preset);
Result run (const std::filesystem::path &folder, const Preset &preset, const std::vector<Pair> &pairs,
	const std::vector<integer> &indices);
std::filesystem::path exportRun (const std::filesystem::path &destination, const std::filesystem::path &input,
	const Preset &preset, const std::vector<Pair> &pairs, const Result &result);

} // namespace BatchAnalysis

void BatchAnalysisWindow_show ();

#endif
