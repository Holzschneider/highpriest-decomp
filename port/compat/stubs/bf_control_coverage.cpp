// The `coverage` part of the control socket (bf_control_coverage.h).
//   coverage {entries, untranslated, placeholder, korean, foreign,
//             list: [{kind, text, drawn, hex, table, where, count}]}
// The same content as the BF_COVERAGE file; empty `list`: nothing of the
// session fell back to Korean or to a placeholder.
#include <stdlib.h>
#include <string.h>

#include "bf_control.h"
#include "bf_control_coverage.h"
#include "bf_coverage.h"

namespace {

struct Counts {
	long long untranslated = 0, placeholder = 0, korean = 0, foreign = 0, entries = 0;
};

void Count(const bf_coverage_entry *e, void *user)
{
	Counts *c = (Counts *)user;
	++c->entries;
	if (strcmp(e->kind, "untranslated") == 0) ++c->untranslated;
	else if (strcmp(e->kind, "placeholder") == 0) ++c->placeholder;
	else if (strcmp(e->kind, "korean") == 0) ++c->korean;
	else ++c->foreign;
}

void Str(bf_control_writer *w, const char *key, const char *v)
{
	bf_control_write_key(w, key);
	bf_control_write_string(w, v);   // NULL as null
}

void WriteEntry(const bf_coverage_entry *e, void *user)
{
	bf_control_writer *w = (bf_control_writer *)user;
	bf_control_write_begin_object(w);
	Str(w, "kind", e->kind);
	Str(w, "text", e->text);
	Str(w, "drawn", e->drawn);
	Str(w, "hex", e->hex);
	Str(w, "table", e->table);
	Str(w, "where", e->where);
	bf_control_write_key(w, "count");
	bf_control_write_int(w, e->count);
	bf_control_write_end_object(w);
}

void WriteCoverage(bf_control_writer *w, void *)
{
	Counts c;
	bf_coverage_each(Count, &c);
	bf_control_write_begin_object(w);
	bf_control_write_key(w, "entries");
	bf_control_write_int(w, c.entries);
	bf_control_write_key(w, "untranslated");
	bf_control_write_int(w, c.untranslated);
	bf_control_write_key(w, "placeholder");
	bf_control_write_int(w, c.placeholder);
	bf_control_write_key(w, "korean");
	bf_control_write_int(w, c.korean);
	bf_control_write_key(w, "foreign");
	bf_control_write_int(w, c.foreign);
	bf_control_write_key(w, "list");
	bf_control_write_begin_array(w);
	bf_coverage_each(WriteEntry, w);
	bf_control_write_end_array(w);
	bf_control_write_end_object(w);
}

}  // namespace

void bf_control_coverage_install(void)
{
	bf_control_register_state("coverage", WriteCoverage, nullptr);
}

void bf_control_coverage_write(void)
{
	const char *path = getenv("BF_COVERAGE");
	if (path && *path)
		bf_coverage_write(path);
}
