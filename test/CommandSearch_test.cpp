// Standalone deterministic matching tests; does not initialize Praat or a GUI.
#include "../sys/CommandSearch.h"
#include <cassert>
#include <iostream>
static int score (const char32_t *q, const char32_t *title, const char32_t *keys = U"", const char32_t *description = U"") {
    return CommandSearch_score (CommandSearch_normalize (q), CommandSearch_normalize (title),
        CommandSearch_normalize (keys), CommandSearch_normalize (description));
}
int main () {
    assert (score (U"streo", U"Combine to stereo") > 0);
    assert (score (U"spectorgam", U"To Spectrogram...") > 0);
    assert (score (U"join recordings", U"Concatenate", U"join recordings") > 0);
    assert (score (U"join zebras", U"Concatenate", U"join recordings") == 0);
    assert (score (U"hann", U"Filter (pass Hann band)...") > 0);
    assert (score (U"hann", U"Filter (stop Hann band)...") > 0);
    assert (score (U"Pitch", U"Pitch") > score (U"Pitch", U"To Pitch..."));
    assert (score (U"Pitch", U"To Pitch...") > score (U"Pitch", U"Other", U"pitch"));
    assert (score (U"Pitch", U"Other", U"pitch") > score (U"Pitch", U"Unrelated", U"", U"pitch"));
    assert (score (U"To TextGrid…", U"To TextGrid...") == 60000);
    assert (score (U"český", U"český příkaz") > 0);
    assert (score (U"", U"Anything") > 0);
    assert (score (U"xy", U"Play") == 0);
    assert (score (U"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", U"Play") == 0);
    std::cout << "CommandSearch matching tests passed\n";
}
