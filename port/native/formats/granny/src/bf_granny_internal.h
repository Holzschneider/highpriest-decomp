// Internals of the Granny reader, shared by its source files. Not part of
// the contract (that is include/bf_granny.h).
#ifndef BF_GRANNY_INTERNAL_H
#define BF_GRANNY_INTERNAL_H

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace bf_granny {

bool DecompressOodle0(const uint8_t *in, size_t inSize, size_t outSize, uint32_t stop16, uint32_t stop8,
	std::vector<uint8_t> &out, std::string &error);

// A place in the decompressed data: section and byte offset. Section -1 is
// the null reference.
struct Ref {
	int section = -1;
	uint32_t offset = 0;
	bool Null() const { return section < 0; }
	Ref At(uint32_t delta) const { Ref r = *this; r.offset += delta; return r; }
};

// Member types of the self-describing type tree (granny_member_type).
enum MemberType {
	kEnd = 0, kInline = 1, kReference = 2, kReferenceToArray = 3, kArrayOfReferences = 4,
	kVariantReference = 5, kSwitchableType = 6, kReferenceToVariantArray = 7, kString = 8,
	kTransform = 9, kReal32 = 10, kInt8 = 11, kUInt8 = 12, kBinormalInt8 = 13, kNormalUInt8 = 14,
	kInt16 = 15, kUInt16 = 16, kBinormalInt16 = 17, kNormalUInt16 = 18, kInt32 = 19, kUInt32 = 20,
};

const char *MemberTypeName(int type);

struct Member {
	int type = 0;
	std::string name;
	Ref referenceType;       // for inline, reference and array members
	int arrayWidth = 0;      // 0 = a single element
	uint32_t offset = 0;     // within the object, in the file's 32-bit layout
	uint32_t size = 0;       // bytes, all array elements together
};

struct Type {
	Ref at;
	std::vector<Member> members;
	uint32_t size = 0;
	const Member *Find(const char *name) const;
};

struct Section {
	uint32_t compression = 0, fileOffset = 0, packedSize = 0, size = 0, alignment = 0;
	uint32_t stop16 = 0, stop8 = 0, fixupOffset = 0, fixupCount = 0, marshalOffset = 0, marshalCount = 0;
	bool decoded = false;
	std::string error;
	std::vector<uint8_t> data;
};

// A container file: header, sections, pointer fix-ups and the type tree.
class File {
public:
	bool Open(const uint8_t *data, size_t size, std::string &error);

	uint32_t version = 0, totalSize = 0, crc = 0, typeTag = 0;
	Ref rootType, rootObject;
	std::vector<Section> sections;
	size_t fixupTotal = 0;

	// Raw reads; out-of-range reads yield zero and set m_bad.
	const uint8_t *Bytes(Ref r, uint32_t count) const;
	uint32_t U32(Ref r) const;
	int32_t I32(Ref r) const { return (int32_t)U32(r); }
	float F32(Ref r) const;
	// The pointer stored at r, through the fix-up table.
	Ref Pointer(Ref r) const;
	// A NUL-terminated string the pointer at r points to ("" for null).
	const char *String(Ref r) const;
	const char *StringAt(Ref r) const;

	// The type definition at r, parsed once and cached.
	const Type *TypeAt(Ref r) const;

	bool Bad() const { return m_bad; }

private:
	std::unordered_map<uint64_t, Ref> m_fixups;   // key: section << 32 | offset
	mutable std::unordered_map<uint64_t, Type *> m_types;
	mutable std::vector<std::unique_ptr<Type>> m_typeStore;
	mutable bool m_bad = false;
	static uint64_t Key(Ref r) { return ((uint64_t)(uint32_t)r.section << 32) | r.offset; }
	uint32_t MemberSize(const Member &m, int depth) const;
	const Type *ParseType(Ref r, int depth) const;
};

// An object with its type: what the reader walks.
struct Object {
	const File *file = nullptr;
	const Type *type = nullptr;
	Ref at;
	bool Valid() const { return file && type && !at.Null(); }

	const Member *Field(const char *name) const { return type ? type->Find(name) : nullptr; }
	Ref FieldRef(const char *name) const;
	int32_t Int(const char *name, int32_t fallback = 0) const;
	float Real(const char *name, float fallback = 0) const;
	const char *Str(const char *name) const;
	// Reference member: the object it points to.
	Object Ref1(const char *name) const;
	// Reference-to-array member: the count and the first element.
	int ArrayCount(const char *name) const;
	Object ArrayElement(const char *name, int index) const;
	// Array-of-references member.
	Object ReferencedElement(const char *name, int index) const;
	// Variant reference member: the object with the type stored next to it.
	Object Variant(const char *name) const;
	// Inline member (a struct embedded in this one).
	Object Inline(const char *name) const;
};

} // namespace bf_granny

#endif
