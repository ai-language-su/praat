/* Question headings in native tooltips are plain text. Mathematical bold
   letters make the headings visually distinct without requiring rich-text
   controls or changing the tooltip behavior on any platform. */
#ifndef _praat_tooltip_h_
#define _praat_tooltip_h_

#include <string>

static std::u32string praat_boldTooltipQuestions (conststring32 guide) {
	std::u32string result;
	for (const char32_t *line = guide; *line; ) {
		const char32_t *end = line;
		while (*end && *end != U'\n')
			end ++;
		const char32_t *question = line;
		while (question < end && question - line < 48 && *question != U'?')
			question ++;
		const bool hasQuestion = question < end && *question == U'?';
		for (const char32_t *p = line; p < end; p ++) {
			const char32_t character = *p;
			if (hasQuestion && p <= question && character >= U'A' && character <= U'Z')
				result += char32_t (0x1D5D4 + character - U'A');
			else if (hasQuestion && p <= question && character >= U'a' && character <= U'z')
				result += char32_t (0x1D5EE + character - U'a');
			else
				result += character;
		}
		if (*end == U'\n')
			result += U'\n';
		line = *end ? end + 1 : end;
	}
	return result;
}

#endif
