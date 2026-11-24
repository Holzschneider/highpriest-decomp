// The Granny container: header, section table, section decompression,
// pointer fix-ups and the self-describing type tree. Layout in FORMAT.md.

#include "bf_granny_internal.h"

#include "bf_trace.h"

#include <string.h>

namespace bf_granny {

namespace {

const uint8_t kMagicLE32[16] = {
	0xb8, 0x67, 0xb0, 0xca, 0xf8, 0x6d, 0xb1, 0x0f, 0x84, 0x72, 0x8c, 0x7e, 0x5e, 0x19, 0x00, 0x1e,
};

uint32_t Get32(const uint8_t *p)
{
	return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

const uint32_t kMemberRecord = 32;   // one member definition in a 32-bit file

} // namespace

const char *MemberTypeName(int type)
{
	static const char *names[] = {
		"end", "inline", "reference", "reference-to-array", "array-of-references", "variant-reference",
		"switchable", "reference-to-variant-array", "string", "transform", "real32", "int8", "uint8",
		"binormal-int8", "normal-uint8", "int16", "uint16", "binormal-int16", "normal-uint16", "int32", "uint32",
	};
	return type >= 0 && type <= 20 ? names[type] : "?";
}

const Member *Type::Find(const char *name) const
{
	for (const Member &m : members)
		if (m.name == name)
			return &m;
	return nullptr;
}

bool File::Open(const uint8_t *data, size_t size, std::string &error)
{
	if (size < 0x20 + 56) {
		error = "too short for a header";
		return false;
	}
	if (memcmp(data, kMagicLE32, 16) != 0) {
		error = "not a little-endian 32-bit Granny file (magic)";
		return false;
	}
	uint32_t headerSize = Get32(data + 16);
	const uint8_t *h = data + 0x20;
	version = Get32(h + 0);
	totalSize = Get32(h + 4);
	crc = Get32(h + 8);
	uint32_t sectionArray = Get32(h + 12);
	uint32_t sectionCount = Get32(h + 16);
	rootType.section = (int)Get32(h + 20);
	rootType.offset = Get32(h + 24);
	rootObject.section = (int)Get32(h + 28);
	rootObject.offset = Get32(h + 32);
	typeTag = Get32(h + 36);
	if (version != 6) {
		error = "format version " + std::to_string(version) + "; this reader knows 6";
		return false;
	}
	const uint32_t kSectionRecord = 44;
	if (sectionCount == 0 || sectionCount > 64 || 0x20 + (size_t)sectionArray + sectionCount * kSectionRecord > size
		|| 0x20 + sectionArray + sectionCount * kSectionRecord > headerSize + 0x20) {
		error = "bad section table";
		return false;
	}
	sections.assign(sectionCount, Section());
	for (uint32_t i = 0; i < sectionCount; ++i) {
		const uint8_t *s = h + sectionArray + i * kSectionRecord;
		Section &sec = sections[i];
		sec.compression = Get32(s + 0);
		sec.fileOffset = Get32(s + 4);
		sec.packedSize = Get32(s + 8);
		sec.size = Get32(s + 12);
		sec.alignment = Get32(s + 16);
		sec.stop16 = Get32(s + 20);
		sec.stop8 = Get32(s + 24);
		sec.fixupOffset = Get32(s + 28);
		sec.fixupCount = Get32(s + 32);
		sec.marshalOffset = Get32(s + 36);
		sec.marshalCount = Get32(s + 40);
	}

	bool allDecoded = true;
	for (uint32_t i = 0; i < sectionCount; ++i) {
		Section &sec = sections[i];
		if ((size_t)sec.fileOffset + sec.packedSize > size) {
			sec.error = "data beyond end of file";
		} else if (sec.size == 0) {
			sec.decoded = true;
		} else if (sec.compression == 0) {
			if (sec.packedSize < sec.size)
				sec.error = "uncompressed section shorter than its size";
			else {
				sec.data.assign(data + sec.fileOffset, data + sec.fileOffset + sec.size);
				sec.decoded = true;
			}
		} else if (sec.compression == 1) {
			sec.decoded = DecompressOodle0(data + sec.fileOffset, sec.packedSize, sec.size, sec.stop16, sec.stop8,
				sec.data, sec.error);
		} else {
			sec.error = "compression " + std::to_string(sec.compression) + " (only 0 and Oodle0 are known)";
		}
		if (!sec.decoded) {
			allDecoded = false;
			sec.data.clear();
			BF_TRACE(BFT_ERR, "granny: section %u: %s", i, sec.error.c_str());
		}
	}

	// Pointer fix-ups: 12-byte records {offset in this section, target
	// section, target offset}, stored uncompressed in format 6.
	for (uint32_t i = 0; i < sectionCount; ++i) {
		const Section &sec = sections[i];
		if ((size_t)sec.fixupOffset + (size_t)sec.fixupCount * 12 > size) {
			error = "fix-up table beyond end of file";
			return false;
		}
		for (uint32_t k = 0; k < sec.fixupCount; ++k) {
			const uint8_t *f = data + sec.fixupOffset + 12 * k;
			Ref from, to;
			from.section = (int)i;
			from.offset = Get32(f);
			to.section = (int)Get32(f + 4);
			to.offset = Get32(f + 8);
			if (to.section < 0 || to.section >= (int)sectionCount) {
				BF_TRACE(BFT_ERR, "granny: fix-up %u of section %u points to section %d", k, i, to.section);
				continue;
			}
			m_fixups[Key(from)] = to;
			++fixupTotal;
		}
	}
	if (!allDecoded) {
		error = "a section could not be decoded";
		return false;
	}
	if (rootType.section < 0 || rootType.section >= (int)sectionCount || rootObject.section < 0
		|| rootObject.section >= (int)sectionCount) {
		error = "root reference outside the sections";
		return false;
	}
	return true;
}

const uint8_t *File::Bytes(Ref r, uint32_t count) const
{
	if (r.section < 0 || r.section >= (int)sections.size()) {
		m_bad = true;
		return nullptr;
	}
	const std::vector<uint8_t> &d = sections[r.section].data;
	if ((size_t)r.offset + count > d.size()) {
		m_bad = true;
		return nullptr;
	}
	return d.data() + r.offset;
}

uint32_t File::U32(Ref r) const
{
	const uint8_t *p = Bytes(r, 4);
	return p ? Get32(p) : 0;
}

float File::F32(Ref r) const
{
	uint32_t bits = U32(r);
	float f;
	memcpy(&f, &bits, 4);
	return f;
}

Ref File::Pointer(Ref r) const
{
	auto it = m_fixups.find(Key(r));
	return it == m_fixups.end() ? Ref() : it->second;
}

const char *File::StringAt(Ref r) const
{
	if (r.Null())
		return "";
	if (r.section >= (int)sections.size())
		return "";
	const std::vector<uint8_t> &d = sections[r.section].data;
	if (r.offset >= d.size())
		return "";
	const void *end = memchr(d.data() + r.offset, 0, d.size() - r.offset);
	return end ? (const char *)d.data() + r.offset : "";
}

const char *File::String(Ref r) const
{
	return StringAt(Pointer(r));
}

uint32_t File::MemberSize(const Member &m, int depth) const
{
	uint32_t count = m.arrayWidth > 0 ? (uint32_t)m.arrayWidth : 1;
	switch (m.type) {
	case kInline: {
		const Type *t = ParseType(m.referenceType, depth + 1);
		return t ? t->size * count : 0;
	}
	case kReference: case kString: return 4 * count;
	case kReferenceToArray: case kArrayOfReferences: case kVariantReference: return 8 * count;
	case kReferenceToVariantArray: return 12 * count;
	case kSwitchableType: return 8 * count;   // not seen in the RO files
	case kTransform: return 68 * count;
	case kReal32: case kInt32: case kUInt32: return 4 * count;
	case kInt8: case kUInt8: case kBinormalInt8: case kNormalUInt8: return count;
	case kInt16: case kUInt16: case kBinormalInt16: case kNormalUInt16: return 2 * count;
	}
	return 0;
}

const Type *File::ParseType(Ref r, int depth) const
{
	if (r.Null() || depth > 32)
		return nullptr;
	auto it = m_types.find(Key(r));
	if (it != m_types.end())
		return it->second;
	std::unique_ptr<Type> t(new Type);
	t->at = r;
	Type *raw = t.get();
	m_types[Key(r)] = raw;          // before members: types may refer to themselves
	m_typeStore.push_back(std::move(t));
	uint32_t offset = 0;
	for (int i = 0; i < 4096; ++i) {
		Ref rec = r.At(i * kMemberRecord);
		if (!Bytes(rec, kMemberRecord)) {
			BF_TRACE(BFT_ERR, "granny: type at %d:%#x runs off its section", r.section, r.offset);
			break;
		}
		Member m;
		m.type = (int)U32(rec);
		if (m.type == kEnd)
			break;
		m.name = String(rec.At(4));
		m.referenceType = Pointer(rec.At(8));
		m.arrayWidth = I32(rec.At(12));
		m.offset = offset;
		raw->members.push_back(m);
		raw->members.back().size = MemberSize(raw->members.back(), depth);
		offset += raw->members.back().size;
	}
	raw->size = offset;
	return raw;
}

const Type *File::TypeAt(Ref r) const
{
	return ParseType(r, 0);
}

// --- Object -----------------------------------------------------------------

Ref Object::FieldRef(const char *name) const
{
	const Member *m = Field(name);
	if (!m || !Valid())
		return Ref();
	return at.At(m->offset);
}

int32_t Object::Int(const char *name, int32_t fallback) const
{
	const Member *m = Field(name);
	if (!m || !Valid())
		return fallback;
	Ref r = at.At(m->offset);
	switch (m->type) {
	case kInt32: case kUInt32: return file->I32(r);
	case kInt16: { const uint8_t *p = file->Bytes(r, 2); return p ? (int16_t)(p[0] | (p[1] << 8)) : fallback; }
	case kUInt16: { const uint8_t *p = file->Bytes(r, 2); return p ? (p[0] | (p[1] << 8)) : fallback; }
	case kInt8: { const uint8_t *p = file->Bytes(r, 1); return p ? (int8_t)p[0] : fallback; }
	case kUInt8: { const uint8_t *p = file->Bytes(r, 1); return p ? p[0] : fallback; }
	case kReal32: return (int32_t)file->F32(r);
	}
	return fallback;
}

float Object::Real(const char *name, float fallback) const
{
	const Member *m = Field(name);
	if (!m || !Valid() || m->type != kReal32)
		return fallback;
	return file->F32(at.At(m->offset));
}

const char *Object::Str(const char *name) const
{
	const Member *m = Field(name);
	if (!m || !Valid() || m->type != kString)
		return "";
	return file->String(at.At(m->offset));
}

Object Object::Ref1(const char *name) const
{
	Object o;
	const Member *m = Field(name);
	if (!m || !Valid() || m->type != kReference)
		return o;
	o.file = file;
	o.type = file->TypeAt(m->referenceType);
	o.at = file->Pointer(at.At(m->offset));
	return o;
}

int Object::ArrayCount(const char *name) const
{
	const Member *m = Field(name);
	if (!m || !Valid())
		return 0;
	int32_t count;
	Ref first;
	uint32_t element;
	if (m->type == kReferenceToArray || m->type == kArrayOfReferences) {
		count = file->I32(at.At(m->offset));
		first = file->Pointer(at.At(m->offset + 4));
		const Type *t = m->type == kReferenceToArray ? file->TypeAt(m->referenceType) : nullptr;
		element = m->type == kArrayOfReferences ? 4 : (t && t->size ? t->size : 1);
	} else if (m->type == kReferenceToVariantArray) {
		count = file->I32(at.At(m->offset + 4));
		first = file->Pointer(at.At(m->offset + 8));
		const Type *t = file->TypeAt(file->Pointer(at.At(m->offset)));
		element = t && t->size ? t->size : 1;
	} else {
		return 0;
	}
	if (count <= 0)
		return 0;
	// The elements must lie in the section the array points into.
	if (!file->Bytes(first, 0) || (uint64_t)count * element > file->sections[first.section].data.size() - first.offset) {
		BF_TRACE(BFT_ERR, "granny: array %s of %d elements does not fit its section", name, count);
		return 0;
	}
	return count;
}

Object Object::ArrayElement(const char *name, int index) const
{
	Object o;
	const Member *m = Field(name);
	if (!m || !Valid() || m->type != kReferenceToArray)
		return o;
	int count = file->I32(at.At(m->offset));
	if (index < 0 || index >= count)
		return o;
	o.file = file;
	o.type = file->TypeAt(m->referenceType);
	Ref first = file->Pointer(at.At(m->offset + 4));
	if (!o.type || first.Null())
		return Object();
	o.at = first.At(o.type->size * index);
	return o;
}

Object Object::ReferencedElement(const char *name, int index) const
{
	Object o;
	const Member *m = Field(name);
	if (!m || !Valid() || m->type != kArrayOfReferences)
		return o;
	int count = file->I32(at.At(m->offset));
	if (index < 0 || index >= count)
		return o;
	Ref table = file->Pointer(at.At(m->offset + 4));
	if (table.Null())
		return o;
	o.file = file;
	o.type = file->TypeAt(m->referenceType);
	o.at = file->Pointer(table.At(4 * index));
	return o;
}

Object Object::Variant(const char *name) const
{
	Object o;
	const Member *m = Field(name);
	if (!m || !Valid() || m->type != kVariantReference)
		return o;
	o.file = file;
	o.type = file->TypeAt(file->Pointer(at.At(m->offset)));
	o.at = file->Pointer(at.At(m->offset + 4));
	return o;
}

Object Object::Inline(const char *name) const
{
	Object o;
	const Member *m = Field(name);
	if (!m || !Valid() || m->type != kInline)
		return o;
	o.file = file;
	o.type = file->TypeAt(m->referenceType);
	o.at = at.At(m->offset);
	return o;
}

} // namespace bf_granny
