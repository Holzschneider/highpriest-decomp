// A minimal reader for the game's GRF archives (version 0x200), for the
// test program only: it finds files by name and returns their contents,
// undoing the archive's DES variant and zlib. The client has its own reader.
#ifndef BF_GRANNY_TEST_GRF_H
#define BF_GRANNY_TEST_GRF_H

#include <stdint.h>
#include <stdio.h>

#include <map>
#include <string>
#include <vector>

class Grf {
public:
	~Grf();
	bool Open(const std::string &path);
	// Lower-case names with backslashes, as stored ("data\\model\\...").
	std::vector<std::string> Names(const std::string &suffix) const;
	bool Read(const std::string &name, std::vector<uint8_t> &out, int *flags = nullptr) const;

private:
	struct Entry { uint32_t packed, aligned, size, offset; uint8_t flags; };
	FILE *m_fp = nullptr;
	std::map<std::string, Entry> m_entries;
};

#endif
