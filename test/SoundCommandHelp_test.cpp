// Text-only semantic checks for distinctions that prompted the tooltip audit.
#include <cassert>
#include <iostream>
#include <set>
#include <string>
struct Entry { const char32_t *title, *summary, *guide; };
#define SOUND_HELP(title, summary, guide) { title, summary, guide },
static const Entry entries [] = {
#include "../sys/praat_SoundCommandHelp.inc"
};
#undef SOUND_HELP
static const Entry & get (const char32_t *title) {
    for (const auto &e : entries) if (std::u32string (e.title) == title) return e;
    throw "Missing expected help";
}
static bool contains (const char32_t *text, const char32_t *term) {
    return text && std::u32string (text).find (term) != std::u32string::npos;
}
int main () {
    std::set<std::u32string> titles;
    for (const auto &e : entries) {
        assert (titles.insert (e.title).second);
        if (e.summary) assert (std::u32string (e.summary).size() >= 35);
        if (e.guide) assert (contains (e.guide, U"?") && std::u32string (e.guide).size() >= 130);
    }
    // Acronyms must be explained in the quick tip, not only in the long text.
    assert (contains (get (U"To Ltas...").summary, U"long-term average spectrum"));
    assert (contains (get (U"To MFCC...").summary, U"mel-frequency cepstral coefficients"));
    assert (contains (get (U"To CPP...").summary, U"cepstral peak prominence"));
    assert (contains (get (U"To Sound (bss)...").summary, U"blind source separation"));
    // Similar labels have materially different output types and meanings.
    assert (contains (get (U"To Harmonicity (gne)...").summary, U"Matrix"));
    assert (contains (get (U"To Harmonicity (cc)...").summary, U"Harmonicity object"));
    assert (contains (get (U"Cross-correlate...").summary, U"Sound"));
    assert (contains (get (U"To CrossCorrelationTable...").summary, U"table"));
    assert (contains (get (U"Get sampling frequency").guide, U"pitch"));
    assert (contains (get (U"Override sampling frequency...").summary, U"pitch change"));
    assert (contains (get (U"Resample...").summary, U"preserving its duration"));
    assert (contains (get (U"Get number of samples").summary, U"per channel"));
    assert (contains (get (U"To Ltas (pitch-corrected)...").summary, U"does not retune"));
    assert (contains (get (U"To TextGrid (speech activity, Silero)...").summary, U"does not transcribe"));
    std::cout << titles.size() << " reviewed Sound help entries: semantic checks passed\n";
}
