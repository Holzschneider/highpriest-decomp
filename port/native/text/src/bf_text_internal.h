// Shared between the files of bf_text; not part of the contract.
#ifndef BF_TEXT_INTERNAL_H
#define BF_TEXT_INTERNAL_H

#include <stdint.h>
#include <vector>

// Decodes text in codepage (or UTF-16/UTF-8) to UTF-16 units. offsets, if
// given, receives for each unit the offset in text of the character it
// came from. Returns -1 for an unknown code page.
int bf_text_decode(int codepage, const void *text, int len, std::vector<uint16_t> &units, std::vector<int> *offsets);

// Test aid: rows of the cached glyphs that lie outside the cell (and are
// therefore not drawn), summed over the glyphs the font has cached.
struct bf_text_font;
int bf_text_debug_rows_outside(const bf_text_font *font);

#endif
