/* Batch measurement using Praat's audio and TextGrid analysis routines. */
#include "BatchAnalysis.h"
#include "CorpusFiles.h"
#include "Data.h"
#include "Sound_to_Formant.h"
#include "Sound_to_Pitch.h"
#include "TextGrid.h"
#include <algorithm>
#include <charconv>
#include <chrono>
#include <fstream>
#include <map>
#include <random>
#include <sstream>
#if defined (_WIN32)
	#include <windows.h>
#endif

namespace BatchAnalysis {
namespace fs = std::filesystem;
namespace {

constexpr double formantStep = 0.0, formantCount = 5.0, formantWindow = 0.025, preemphasis = 50.0;
constexpr double pitchStep = 0.0, attenuation = 0.03, silence = 0.09, voicing = 0.50;
constexpr double octave = 0.055, octaveJump = 0.35, voicedUnvoiced = 0.14;
constexpr integer candidates = 15;
constexpr bool veryAccurate = false;
constexpr const char *presetHeader = "Praat Batch Preset 1";

std::string utf8 (conststring32 text) {
	return text ? Melder_peek32to8 (text) : "";
}
std::u32string utf32 (const std::string &text) {
	return Melder_peek8to32_u (text.c_str());
}
structMelderFile asFile (const fs::path &path) {
	structMelderFile file {};
	Melder_pathToFile (Melder_peek8to32_u (path.u8string().c_str()), & file);
	return file;
}
autoTextGrid readGrid (const fs::path &path) {
	auto file = asFile (path);
	autoDaata object = Data_readFromFile (& file);
	Melder_require (object && object -> classInfo == classTextGrid, U"The annotation file is not a TextGrid.");
	return object.static_cast_move<structTextGrid>();
}
IntervalTier findTier (TextGrid grid, const Preset &preset) {
	integer found = 0;
	for (integer i = 1; i <= grid -> tiers -> size; ++ i) {
		const auto tier = grid -> tiers -> at [i];
		if (tier -> name && str32equ (tier -> name.get(), preset.tier.c_str())) {
			Melder_require (! found, U"The TextGrid has more than one tier named “", preset.tier.c_str(), U"”.");
			found = i;
		}
	}
	Melder_require (found, U"Tier “", preset.tier.c_str(), U"” is missing.");
	return TextGrid_checkSpecifiedTierIsIntervalTier (grid, found);
}
bool labelMatches (TextInterval interval, const Preset &preset) {
	conststring32 text = interval -> text.get();
	return text && *text && (preset.label.empty() || str32equ (text, preset.label.c_str()));
}
std::string exactNumber (double value) { return utf8 (Melder_double (value)); }
void appendIssue (std::u32string &issues, conststring32 issue) {
	if (! issues.empty()) issues += U"; ";
	issues += issue;
}
std::vector<Row> measurePair (const fs::path &folder, const Preset &preset, const Pair &pair, integer pairIndex) {
	auto grid = readGrid (folder / pair.grid);
	const auto tier = findTier (grid.get(), preset);
	auto audioFile = asFile (folder / pair.audio);
	autoDaata audioObject = Data_readFromFile (& audioFile);
	Melder_require (audioObject && audioObject -> classInfo == classSound, U"The recording could not be read as a Sound.");
	const auto sound = static_cast<Sound> (audioObject.get());
	autoFormant formant;
	autoPitch pitch;
	std::u32string formantError, pitchError;
	if (preset.formants) {
		try {
			formant = Sound_to_Formant_burg (sound, formantStep, formantCount, preset.formantCeiling, formantWindow, preemphasis);
		} catch (MelderError) {
			formantError = Melder_getError();
			Melder_clearError ();
		}
	}
	if (preset.pitch) {
		try {
			pitch = Sound_to_Pitch_filteredAc (sound, pitchStep, preset.pitchFloor, preset.pitchTop,
				candidates, veryAccurate, attenuation, silence, voicing, octave, octaveJump, voicedUnvoiced);
		} catch (MelderError) {
			pitchError = Melder_getError();
			Melder_clearError ();
		}
	}
	std::vector<Row> rows;
	for (integer i = 1; i <= tier -> intervals.size; ++ i) {
		const auto interval = tier -> intervals.at [i];
		if (! labelMatches (interval, preset)) continue;
		Row row { pairIndex, i, interval -> text.get(), {}, interval -> xmin, interval -> xmax };
		if (row.end <= row.start || row.start < sound -> xmin - 1e-9 || row.end > sound -> xmax + 1e-9) {
			row.problem = U"Interval is empty or outside the recording";
		} else {
			if (formant) {
				const double midpoint = row.start + 0.5 * (row.end - row.start);
				row.f1 = Formant_getValueAtTime (formant.get(), 1, midpoint, kFormant_unit::HERTZ);
				row.f2 = Formant_getValueAtTime (formant.get(), 2, midpoint, kFormant_unit::HERTZ);
				if (! isdefined (row.f1) || ! isdefined (row.f2)) appendIssue (row.problem, U"Undefined midpoint formant");
			} else if (! formantError.empty()) {
				appendIssue (row.problem, (U"Formants: " + formantError).c_str());
			}
			if (pitch) {
				row.f0 = Pitch_getQuantile (pitch.get(), row.start, row.end, 0.5, kPitch_unit::HERTZ);
				if (! isdefined (row.f0)) appendIssue (row.problem, U"No voiced pitch frames in interval");
			} else if (! pitchError.empty()) {
				appendIssue (row.problem, (U"Pitch: " + pitchError).c_str());
			}
		}
		rows.push_back (std::move (row));
	}
	Melder_require (! rows.empty(), U"No intervals match this preset.");
	return rows;
}
void csvRow (std::ostream &out, const std::vector<std::string> &fields) {
	bool first = true;
	for (const auto &field : fields) {
		if (! first) out.put (',');
		first = false;
		out.put ('"');
		for (char c : field) {
			if (c == '"') out.put ('"');
			out.put (c);
		}
		out.put ('"');
	}
	out << "\r\n";
}
std::ofstream writer (const fs::path &output) {
	std::ofstream out (output, std::ios::binary);
	out.exceptions (std::ios::badbit | std::ios::failbit);
	return out;
}
std::string runId () {
	const auto microseconds = std::chrono::duration_cast<std::chrono::microseconds> (
		std::chrono::system_clock::now().time_since_epoch()).count();
	return std::to_string (microseconds) + "-" + std::to_string (std::random_device{}());
}
} // namespace

double parseNumber (conststring32 text, conststring32 label) {
	auto bytes = utf8 (text);
	const auto begin = bytes.find_first_not_of (" \t");
	Melder_require (begin != std::string::npos, label, U" needs a number.");
	const auto end = bytes.find_last_not_of (" \t") + 1;
	double value = 0.0;
	const auto read = std::from_chars (bytes.data() + begin, bytes.data() + end, value);
	Melder_require (read.ec == std::errc() && read.ptr == bytes.data() + end && std::isfinite (value),
		label, U" needs a finite number. Use a period for decimals.");
	return value;
}

void validate (const Preset &preset) {
	Melder_require (! preset.tier.empty() && preset.tier.find_first_of (U"\r\n") == std::u32string::npos,
		U"Enter a tier name on one line.");
	Melder_require (preset.label.find_first_of (U"\r\n") == std::u32string::npos, U"Enter the label filter on one line.");
	Melder_require (preset.duration || preset.formants || preset.pitch, U"Select at least one measurement.");
	Melder_require (std::isfinite (preset.formantCeiling) && preset.formantCeiling >= 500 && preset.formantCeiling <= 20000,
		U"Enter a formant ceiling between 500 and 20000 Hz.");
	Melder_require (std::isfinite (preset.pitchFloor) && std::isfinite (preset.pitchTop) &&
		preset.pitchFloor >= 20 && preset.pitchTop > preset.pitchFloor && preset.pitchTop <= 5000,
		U"Enter a pitch floor of at least 20 Hz and a higher pitch top of at most 5000 Hz.");
}

std::string presetText (const Preset &preset) {
	validate (preset);
	std::ostringstream out;
	out << presetHeader << '\n'
		<< "tier=" << utf8 (preset.tier.c_str()) << '\n'
		<< "label=" << utf8 (preset.label.c_str()) << '\n'
		<< "duration=" << preset.duration << '\n'
		<< "formants=" << preset.formants << '\n'
		<< "pitch=" << preset.pitch << '\n'
		<< "formant_ceiling_hz=" << exactNumber (preset.formantCeiling) << '\n'
		<< "pitch_floor_hz=" << exactNumber (preset.pitchFloor) << '\n'
		<< "pitch_top_hz=" << exactNumber (preset.pitchTop) << '\n';
	return out.str();
}

Preset loadPreset (const fs::path &path) {
	Melder_require (fs::file_size (path) <= 65536, U"The file is too large to be a batch preset.");
	std::ifstream file (path, std::ios::binary);
	Melder_require (file.is_open(), U"Could not open the batch preset.");
	file.exceptions (std::ios::badbit);
	std::string text (65537, '\0');
	file.read (text.data(), text.size());
	text.resize (file.gcount());
	Melder_require (text.size() <= 65536 && text.find ('\0') == std::string::npos && Melder_str8IsValidUtf8 (text.c_str()),
		U"The preset must be a UTF-8 text file no larger than 64 KiB.");
	if (text.compare (0, 3, "\xEF\xBB\xBF") == 0) text.erase (0, 3);
	std::istringstream in (text);
	std::string line;
	auto readLine = [&] {
		if (! std::getline (in, line)) return false;
		if (! line.empty() && line.back() == '\r') line.pop_back();
		return true;
	};
	Melder_require (readLine () && line == presetHeader, U"Unsupported batch preset format.");
	std::map<std::string, std::string> fields;
	while (readLine ()) {
		const auto separator = line.find ('=');
		Melder_require (separator != std::string::npos &&
			fields.emplace (line.substr (0, separator), line.substr (separator + 1)).second, U"Invalid or duplicate preset setting.");
	}
	auto field = [&] (const char *name) {
		const auto found = fields.find (name);
		Melder_require (found != fields.end(), U"Missing preset setting: ", Melder_peek8to32_u (name));
		auto value = found -> second;
		fields.erase (found);
		return value;
	};
	auto boolean = [&] (const char *name) {
		auto value = field (name);
		Melder_require (value == "0" || value == "1", U"Invalid preset checkbox value.");
		return value == "1";
	};
	Preset preset;
	preset.tier = utf32 (field ("tier"));
	preset.label = utf32 (field ("label"));
	preset.duration = boolean ("duration");
	preset.formants = boolean ("formants");
	preset.pitch = boolean ("pitch");
	preset.formantCeiling = parseNumber (utf32 (field ("formant_ceiling_hz")).c_str(), U"Formant ceiling");
	preset.pitchFloor = parseNumber (utf32 (field ("pitch_floor_hz")).c_str(), U"Pitch floor");
	preset.pitchTop = parseNumber (utf32 (field ("pitch_top_hz")).c_str(), U"Pitch top");
	Melder_require (fields.empty(), U"The file contains an unknown preset setting.");
	validate (preset);
	return preset;
}

void savePreset (const Preset &preset, const fs::path &path, bool replace) {
	const auto contents = presetText (preset);
	Melder_require (replace || ! fs::exists (path), U"A preset with that name already exists.");
	const auto staging = path.parent_path() / (".praat-preset-" + runId ());
	Melder_require (fs::create_directory (staging), U"Could not create the temporary preset folder.");
	const auto pending = staging / "preset";
	try {
		auto out = writer (pending);
		out << contents;
		out.close();
		#if defined (_WIN32)
			const DWORD flags = MOVEFILE_WRITE_THROUGH | (replace ? MOVEFILE_REPLACE_EXISTING : 0);
			Melder_require (MoveFileExW (pending.c_str(), path.c_str(), flags),
				U"Could not save the preset (Windows error ", (integer) GetLastError (), U").");
		#else
			if (replace)
				fs::rename (pending, path);
			else
				fs::create_hard_link (pending, path);
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

std::vector<Pair> scan (const fs::path &folder, const Preset &preset) {
	validate (preset);
	std::vector<Pair> pairs;
	for (const auto &group : CorpusFiles_scan (folder)) {
		Pair pair;
		if (! group.recordings.empty()) pair.audio = group.recordings.front();
		if (! group.textGrids.empty()) pair.grid = group.textGrids.front();
		if (group.recordings.size() > 1 || group.textGrids.size() > 1)
			pair.problem = U"Ambiguous match: several files share this name";
		else if (pair.audio.empty()) pair.problem = U"Missing recording";
		else if (pair.grid.empty()) pair.problem = U"Missing TextGrid";
		else {
			try {
				auto grid = readGrid (folder / pair.grid);
				const auto tier = findTier (grid.get(), preset);
				for (integer i = 1; i <= tier -> intervals.size; ++ i)
					if (labelMatches (tier -> intervals.at [i], preset)) ++ pair.matchingIntervals;
				if (! pair.matchingIntervals) pair.problem = U"No matching labeled intervals";
			} catch (MelderError) {
				pair.problem = Melder_getError();
				Melder_clearError ();
			}
		}
		pairs.push_back (std::move (pair));
	}
	return pairs;
}

Result run (const fs::path &folder, const Preset &preset, const std::vector<Pair> &pairs, const std::vector<integer> &indices) {
	validate (preset);
	Result result;
	autoMelderProgress progress (U"Batch analysis");
	for (integer index : indices) {
		Melder_require (index >= 0 && index < (integer) pairs.size(), U"Invalid batch file selection.");
		const auto &pair = pairs [index];
		if (! pair.problem.empty()) continue;
		try {
			Melder_progress (double (result.attempted) / indices.size(),
				U"Measuring ", Melder_peek8to32_u (pair.audio.generic_u8string().c_str()));
		} catch (MelderError) {
			if (! str32str (Melder_getError(), U"Interrupted!")) throw;
			Melder_clearError ();
			result.cancelled = true;
			break;
		}
		++ result.attempted;
		try {
			autoMelderProgressOff innerProgress;
			auto rows = measurePair (folder, preset, pair, index);
			std::move (rows.begin(), rows.end(), std::back_inserter (result.rows));
			++ result.succeeded;
		} catch (MelderError) {
			result.failures.emplace_back (index, Melder_getError());
			Melder_clearError ();
		}
	}
	return result;
}

fs::path exportRun (const fs::path &destination, const fs::path &input, const Preset &preset,
	const std::vector<Pair> &pairs, const Result &result)
{
	const auto id = runId ();
	const auto finalFolder = destination / ("Praat analysis " + id);
	const auto staging = destination / (".praat-analysis-" + id);
	Melder_require (fs::create_directory (staging), U"Could not create the temporary export folder.");
	try {
		auto table = writer (staging / "measurements.csv");
		std::vector<std::string> fields { "recording", "textgrid", "tier", "interval", "label", "start_s", "end_s" };
		if (preset.duration) fields.push_back ("duration_s");
		if (preset.formants) { fields.push_back ("F1_midpoint_Hz"); fields.push_back ("F2_midpoint_Hz"); }
		if (preset.pitch) fields.push_back ("F0_voiced_median_Hz");
		fields.push_back ("issue");
		csvRow (table, fields);
		for (const auto &row : result.rows) {
			const auto &pair = pairs [row.pair];
			fields = { pair.audio.generic_u8string(), pair.grid.generic_u8string(), utf8 (preset.tier.c_str()),
				std::to_string (row.interval), utf8 (row.label.c_str()), exactNumber (row.start), exactNumber (row.end) };
			if (preset.duration) fields.push_back (exactNumber (row.end - row.start));
			if (preset.formants) {
				fields.push_back (isdefined (row.f1) ? exactNumber (row.f1) : "");
				fields.push_back (isdefined (row.f2) ? exactNumber (row.f2) : "");
			}
			if (preset.pitch) fields.push_back (isdefined (row.f0) ? exactNumber (row.f0) : "");
			fields.push_back (utf8 (row.problem.c_str()));
			csvRow (table, fields);
		}
		table.close();
		auto issues = writer (staging / "files.csv");
		csvRow (issues, { "recording", "textgrid", "issue" });
		for (const auto &pair : pairs)
			if (! pair.problem.empty()) csvRow (issues, { pair.audio.generic_u8string(), pair.grid.generic_u8string(), utf8 (pair.problem.c_str()) });
		for (const auto &[index, problem] : result.failures) {
			const auto &pair = pairs [index];
			csvRow (issues, { pair.audio.generic_u8string(), pair.grid.generic_u8string(), utf8 (problem.c_str()) });
		}
		issues.close();
		auto settings = writer (staging / "run-settings.txt");
		settings << "Praat " << utf8 (Melder_appVersionSTR()) << '\n'
			<< "Input folder: " << input.generic_u8string() << '\n'
			<< "Attempted: " << result.attempted << "\nCompleted: " << result.succeeded << "\nCancelled: " << result.cancelled << "\n\n"
			<< presetText (preset)
			<< "\nFormant method: Burg, midpoint, step " << formantStep << ", count " << formantCount
			<< ", window " << formantWindow << " s, pre-emphasis " << preemphasis << " Hz\n"
			<< "Pitch method: filtered autocorrelation, median of voiced frames, step " << pitchStep
			<< ", candidates " << candidates << ", very accurate " << veryAccurate
			<< ", attenuation " << attenuation << ", silence threshold " << silence << ", voicing threshold " << voicing
			<< ", octave cost " << octave << ", octave jump cost " << octaveJump << ", voiced/unvoiced cost " << voicedUnvoiced << '\n'
			<< "A step of 0 means automatic.\n";
		settings.close();
		savePreset (preset, staging / "settings.PraatBatchPreset", false);
		fs::rename (staging, finalFolder);
		return finalFolder;
	} catch (...) {
		std::error_code ignored;
		for (const auto *name : { "measurements.csv", "files.csv", "run-settings.txt", "settings.PraatBatchPreset" })
			fs::remove (staging / name, ignored);
		fs::remove (staging, ignored);
		throw;
	}
}
} // namespace BatchAnalysis
