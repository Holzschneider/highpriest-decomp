// Between bf_strings.cpp and bf_coverage.cpp (same library), not a contract.
#ifndef BF_COVERAGE_INTERNAL_H
#define BF_COVERAGE_INTERNAL_H

#include <string>

// "symbol+0x12" for a code address (the image's file name when no symbol is
// near), demangled.
std::string bf_coverage_symbol(const void *addr);

// The bf_tr fallbacks with the caller of their first lookup.
void bf_strings_each_fallback_site(void (*fn)(const std::string &original, long long count, const std::string &site,
                                              void *user),
                                   void *user);

#endif
