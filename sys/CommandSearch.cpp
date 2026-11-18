#include "CommandSearch.h"
#include <algorithm>
#include <numeric>

std::u32string CommandSearch_normalize (const std::u32string &text) {
    std::u32string result;
    for (char32_t c : text) {
        if (c >= U'A' && c <= U'Z') c += U'a' - U'A';
        if (c == U'&') continue;
        if (c <= U' ' || c == U'.' || c == U'…' || c == U'>' || c == U'-' || c == U'(' || c == U')' || c == U':' || c == U'/') {
            if (! result.empty() && result.back() != U' ') result += U' ';
        } else result += c;
    }
    if (! result.empty() && result.back() == U' ') result.pop_back();
    return result;
}
static std::vector<std::u32string> words (const std::u32string &s) {
    std::vector<std::u32string> out;
    size_t start = 0;
    while (start < s.size()) {
        size_t end = s.find (U' ', start);
        if (end == s.npos) end = s.size();
        out.push_back (s.substr (start, end - start)); start = end + 1;
    }
    return out;
}
static int distance (const std::u32string &a, const std::u32string &b) {
    if (a.size() > b.size() + 2 || b.size() > a.size() + 2) return 3;
    std::vector<int> previous (b.size() + 1), older, row (b.size() + 1);
    std::iota (previous.begin(), previous.end(), 0);
    for (size_t i = 1; i <= a.size(); ++ i) {
        row [0] = int (i);
        for (size_t j = 1; j <= b.size(); ++ j) {
            row [j] = std::min ({ row [j-1] + 1, previous [j] + 1, previous [j-1] + (a [i-1] != b [j-1]) });
            if (i > 1 && j > 1 && a [i-1] == b [j-2] && a [i-2] == b [j-1]) row [j] = std::min (row [j], older [j-2] + 1);
        }
        older = previous; previous.swap (row);
    }
    return previous.back();
}
int CommandSearch_score (const std::u32string &query, const std::u32string &title,
    const std::u32string &keywords, const std::u32string &description)
{
    if (query.empty()) return 1;
    if (query == title) return 60000;
    const auto tokens = words (query), names = words (title), keys = words (keywords), notes = words (description);
    int weakest = 5, total = 0;
    for (const auto &token : tokens) {
        int tier = 0;
        for (const auto &name : names) {
            if (name == token) tier = std::max (tier, 5);
            else if (name.find (token) == 0) tier = std::max (tier, 4);
            else if (token.size() >= 4 && distance (token, name) <= (token.size() >= 8 ? 2 : 1)) tier = std::max (tier, 2);
        }
        if (std::find (keys.begin(), keys.end(), token) != keys.end()) tier = std::max (tier, 3);
        if (std::find (notes.begin(), notes.end(), token) != notes.end()) tier = std::max (tier, 1);
        if (! tier) return 0;
        weakest = std::min (weakest, tier); total += tier;
    }
    return weakest * 10000 + std::min (total, 999);
}
