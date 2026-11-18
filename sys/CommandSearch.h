#ifndef _CommandSearch_h_
#define _CommandSearch_h_
#include <string>
#include <vector>
// Inputs are Unicode case-folded by the caller using Praat's Unicode tables.
std::u32string CommandSearch_normalize (const std::u32string &text);
int CommandSearch_score (const std::u32string &query, const std::u32string &title,
    const std::u32string &keywords, const std::u32string &description);
#endif
