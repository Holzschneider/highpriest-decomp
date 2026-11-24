// The C API: walks the object tree below granny_file_info once, at open,
// into owned arrays, and hands out pointers into them.

#include "bf_granny.h"
#include "bf_granny_internal.h"

#include "bf_trace.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include <deque>

using namespace bf_granny;

struct bf_granny_file {
	File file;
	bf_granny_info info;
	std::vector<bf_granny_section> sections;
	std::deque<std::string> strings;   // stable addresses

	std::vector<bf_granny_texture> textures;
	std::vector<bf_granny_material> materials;
	std::vector<bf_granny_skeleton> skeletons;
	std::vector<std::vector<bf_granny_bone>> bones;
	std::vector<bf_granny_mesh> meshes;
	std::vector<std::vector<bf_granny_vertex>> vertices;
	std::vector<std::vector<uint32_t>> indices;
	std::vector<std::vector<bf_granny_tri_group>> groups;
	std::vector<std::vector<int>> meshMaterials;
	std::vector<std::vector<bf_granny_bone_binding>> bindings;
	std::vector<bf_granny_model> models;
	std::vector<std::vector<int>> modelMeshes;
	std::vector<bf_granny_track_group> trackGroups;
	std::vector<std::vector<bf_granny_transform_track>> tracks;
	std::deque<std::vector<float>> curveData;
	std::vector<bf_granny_animation> animations;
	std::vector<std::vector<int>> animationGroups;
	std::vector<const Member *> rootMembers;

	const char *Keep(const char *s) { strings.push_back(s ? s : ""); return strings.back().c_str(); }
};

namespace {

uint64_t KeyOf(Ref r) { return ((uint64_t)(uint32_t)r.section << 32) | r.offset; }

int IndexOf(const std::vector<uint64_t> &keys, Ref r)
{
	if (r.Null())
		return -1;
	uint64_t k = KeyOf(r);
	for (size_t i = 0; i < keys.size(); ++i)
		if (keys[i] == k)
			return (int)i;
	return -1;
}

void ReadTransform(const File &f, Ref r, bf_granny_transform *t)
{
	t->flags = f.U32(r);
	for (int i = 0; i < 3; ++i) t->position[i] = f.F32(r.At(4 + 4 * i));
	for (int i = 0; i < 4; ++i) t->orientation[i] = f.F32(r.At(16 + 4 * i));
	for (int i = 0; i < 9; ++i) t->scale_shear[i] = f.F32(r.At(32 + 4 * i));
}

void ReadFloats(const File &f, Ref r, int count, float *out)
{
	for (int i = 0; i < count; ++i)
		out[i] = f.F32(r.At(4 * i));
}

// One element of a scalar member, as a float (normalised types to 0..1 or -1..1).
float ScalarAt(const File &f, int type, Ref r)
{
	const uint8_t *p;
	switch (type) {
	case kReal32: return f.F32(r);
	case kUInt8: p = f.Bytes(r, 1); return p ? p[0] : 0;
	case kInt8: p = f.Bytes(r, 1); return p ? (int8_t)p[0] : 0;
	case kNormalUInt8: p = f.Bytes(r, 1); return p ? p[0] / 255.0f : 0;
	case kBinormalInt8: p = f.Bytes(r, 1); return p ? (int8_t)p[0] / 127.0f : 0;
	case kUInt16: p = f.Bytes(r, 2); return p ? (uint16_t)(p[0] | (p[1] << 8)) : 0;
	case kInt16: p = f.Bytes(r, 2); return p ? (int16_t)(p[0] | (p[1] << 8)) : 0;
	case kNormalUInt16: p = f.Bytes(r, 2); return p ? (uint16_t)(p[0] | (p[1] << 8)) / 65535.0f : 0;
	case kBinormalInt16: p = f.Bytes(r, 2); return p ? (int16_t)(p[0] | (p[1] << 8)) / 32767.0f : 0;
	case kInt32: case kUInt32: return (float)f.I32(r);
	}
	return 0;
}

uint32_t ElementSize(int type)
{
	switch (type) {
	case kReal32: case kInt32: case kUInt32: return 4;
	case kInt16: case kUInt16: case kNormalUInt16: case kBinormalInt16: return 2;
	case kInt8: case kUInt8: case kNormalUInt8: case kBinormalInt8: return 1;
	}
	return 0;
}

// Reads up to count elements of a named scalar member of a vertex.
int VertexField(const File &f, const Type *vt, Ref vertex, const char *name, float *out, int count)
{
	const Member *m = vt->Find(name);
	if (!m)
		return 0;
	uint32_t es = ElementSize(m->type);
	if (!es)
		return 0;
	int width = m->arrayWidth > 0 ? m->arrayWidth : 1;
	int n = width < count ? width : count;
	for (int i = 0; i < n; ++i)
		out[i] = ScalarAt(f, m->type, vertex.At(m->offset + es * i));
	return n;
}

// Raw bytes of a uint8-like member (bone indices and weights keep their values).
int VertexBytes(const File &f, const Type *vt, Ref vertex, const char *name, uint8_t *out, int count)
{
	const Member *m = vt->Find(name);
	if (!m || ElementSize(m->type) != 1)
		return 0;
	int width = m->arrayWidth > 0 ? m->arrayWidth : 1;
	int n = width < count ? width : count;
	const uint8_t *p = f.Bytes(vertex.At(m->offset), n);
	if (!p)
		return 0;
	memcpy(out, p, n);
	return n;
}

std::string Layout(const Type *t)
{
	std::string s;
	if (!t)
		return s;
	for (const Member &m : t->members) {
		if (!s.empty())
			s += ' ';
		s += m.name + ":" + MemberTypeName(m.type);
		if (m.arrayWidth > 0)
			s += "[" + std::to_string(m.arrayWidth) + "]";
	}
	return s;
}

void ReadCurve(bf_granny_file *g, const Object &curve, int dimension, bf_granny_curve *out)
{
	memset(out, 0, sizeof(*out));
	out->dimension = dimension;
	if (!curve.Valid())
		return;
	out->degree = curve.Int("Degree");
	const File &f = g->file;
	for (int which = 0; which < 2; ++which) {
		const char *name = which ? "Controls" : "Knots";
		int n = curve.ArrayCount(name);
		const Member *m = curve.Field(name);
		if (n <= 0 || !m || n > (1 << 24))
			continue;
		Ref first = f.Pointer(curve.at.At(m->offset + 4));
		g->curveData.push_back(std::vector<float>((size_t)n));
		ReadFloats(f, first, n, g->curveData.back().data());
		if (which) {
			out->control_count = n;
			out->controls = g->curveData.back().data();
		} else {
			out->knot_count = n;
			out->knots = g->curveData.back().data();
		}
	}
}

void Extract(bf_granny_file *g)
{
	const File &f = g->file;
	Object root;
	root.file = &f;
	root.type = f.TypeAt(f.rootType);
	root.at = f.rootObject;
	bf_granny_info &info = g->info;
	if (root.type)
		for (const Member &m : root.type->members)
			g->rootMembers.push_back(&m);

	info.from_file_name = g->Keep(root.Str("FromFileName"));
	Object art = root.Ref1("ArtToolInfo");
	info.art_tool = g->Keep(art.Str("FromArtToolName"));
	info.units_per_meter = art.Real("UnitsPerMeter");
	const char *axes[4] = { "Origin", "RightVector", "UpVector", "BackVector" };
	float *dst[4] = { info.origin, info.right, info.up, info.back };
	for (int i = 0; i < 4; ++i)
		if (art.Field(axes[i]))
			ReadFloats(f, art.FieldRef(axes[i]), 3, dst[i]);
	Object exp = root.Ref1("ExporterInfo");
	info.exporter = g->Keep(exp.Str("ExporterName"));
	info.exporter_version[0] = exp.Int("ExporterMajorRevision");
	info.exporter_version[1] = exp.Int("ExporterMinorRevision");
	info.exporter_version[2] = exp.Int("ExporterCustomization");
	info.exporter_version[3] = exp.Int("ExporterBuildNumber");

	// Textures.
	std::vector<uint64_t> textureKeys;
	for (int i = 0; i < root.ArrayCount("Textures"); ++i) {
		Object t = root.ReferencedElement("Textures", i);
		bf_granny_texture tex;
		memset(&tex, 0, sizeof(tex));
		textureKeys.push_back(KeyOf(t.at));
		tex.file_name = g->Keep(t.Str("FromFileName"));
		tex.texture_type = t.Int("TextureType");
		tex.width = t.Int("Width");
		tex.height = t.Int("Height");
		tex.encoding = t.Int("Encoding");
		tex.sub_format = t.Int("SubFormat");
		Object layout = t.Inline("Layout");
		tex.bytes_per_pixel = layout.Int("BytesPerPixel");
		if (layout.Field("ShiftForComponent"))
			for (int k = 0; k < 4; ++k) {
				tex.shift_for_component[k] = f.I32(layout.FieldRef("ShiftForComponent").At(4 * k));
				tex.bits_for_component[k] = f.I32(layout.FieldRef("BitsForComponent").At(4 * k));
			}
		tex.image_count = t.ArrayCount("Images");
		Object image = t.ArrayElement("Images", 0);
		tex.mip_count = image.ArrayCount("MIPLevels");
		Object mip = image.ArrayElement("MIPLevels", 0);
		tex.stride = mip.Int("Stride");
		int n = mip.ArrayCount("Pixels");
		const Member *pm = mip.Field("Pixels");
		if (n > 0 && pm) {
			Ref px = f.Pointer(mip.at.At(pm->offset + 4));
			tex.pixels = f.Bytes(px, (uint32_t)n);
			tex.pixel_size = tex.pixels ? (size_t)n : 0;
		}
		g->textures.push_back(tex);
	}

	// Materials.
	std::vector<uint64_t> materialKeys;
	std::vector<Object> materialObjects;
	for (int i = 0; i < root.ArrayCount("Materials"); ++i) {
		Object m = root.ReferencedElement("Materials", i);
		materialKeys.push_back(KeyOf(m.at));
		materialObjects.push_back(m);
	}
	for (const Object &m : materialObjects) {
		bf_granny_material mat;
		mat.name = g->Keep(m.Str("Name"));
		mat.texture = IndexOf(textureKeys, m.Ref1("Texture").at);
		mat.diffuse_texture = mat.texture;
		mat.opacity_texture = -1;
		mat.map_count = m.ArrayCount("Maps");
		for (int k = 0; k < mat.map_count; ++k) {
			Object map = m.ArrayElement("Maps", k);
			Object sub = map.Ref1("Map");
			int tex = IndexOf(textureKeys, sub.Ref1("Texture").at);
			if (!strcmp(map.Str("Usage"), "Diffuse Color") && tex >= 0)
				mat.diffuse_texture = tex;
			else if (!strcmp(map.Str("Usage"), "Opacity") && tex >= 0)
				mat.opacity_texture = tex;
		}
		g->materials.push_back(mat);
	}

	// Skeletons.
	std::vector<uint64_t> skeletonKeys;
	for (int i = 0; i < root.ArrayCount("Skeletons"); ++i) {
		Object s = root.ReferencedElement("Skeletons", i);
		skeletonKeys.push_back(KeyOf(s.at));
		bf_granny_skeleton sk;
		sk.name = g->Keep(s.Str("Name"));
		std::vector<bf_granny_bone> bones;
		for (int b = 0; b < s.ArrayCount("Bones"); ++b) {
			Object o = s.ArrayElement("Bones", b);
			bf_granny_bone bone;
			memset(&bone, 0, sizeof(bone));
			bone.name = g->Keep(o.Str("Name"));
			bone.parent = o.Int("ParentIndex", -1);
			const char *tn = o.Field("Transform") ? "Transform" : "LocalTransform";
			if (o.Field(tn))
				ReadTransform(f, o.FieldRef(tn), &bone.local);
			const char *iw = o.Field("InverseWorldTransform") ? "InverseWorldTransform" : "InverseWorld4x4";
			if (o.Field(iw))
				ReadFloats(f, o.FieldRef(iw), 16, bone.inverse_world);
			bones.push_back(bone);
		}
		g->bones.push_back(bones);
		g->skeletons.push_back(sk);
	}

	// Meshes.
	std::vector<uint64_t> meshKeys;
	for (int i = 0; i < root.ArrayCount("Meshes"); ++i) {
		Object m = root.ReferencedElement("Meshes", i);
		meshKeys.push_back(KeyOf(m.at));
		bf_granny_mesh mesh;
		memset(&mesh, 0, sizeof(mesh));
		mesh.name = g->Keep(m.Str("Name"));

		Object vd = m.Ref1("PrimaryVertexData");
		std::vector<bf_granny_vertex> verts;
		const Member *vm = vd.Field("Vertices");
		if (vm && vm->type == kReferenceToVariantArray) {
			const Type *vt = f.TypeAt(f.Pointer(vd.at.At(vm->offset)));
			int count = f.I32(vd.at.At(vm->offset + 4));
			Ref first = f.Pointer(vd.at.At(vm->offset + 8));
			mesh.vertex_layout = g->Keep(Layout(vt).c_str());
			if (vt && vt->size && !first.Null() && count > 0 && count < (1 << 22)) {
				mesh.rigid = vt->Find("BoneWeights") == nullptr;
				verts.resize((size_t)count);
				for (int v = 0; v < count; ++v) {
					bf_granny_vertex &out = verts[v];
					memset(&out, 0, sizeof(out));
					Ref at = first.At(vt->size * v);
					VertexField(f, vt, at, "Position", out.position, 3);
					VertexField(f, vt, at, "Normal", out.normal, 3);
					VertexField(f, vt, at, "TextureCoordinates0", out.uv, 2);
					if (mesh.rigid) {
						out.bone_weight[0] = 255;
					} else {
						VertexBytes(f, vt, at, "BoneIndices", out.bone_index, 4);
						VertexBytes(f, vt, at, "BoneWeights", out.bone_weight, 4);
					}
				}
			}
		} else {
			mesh.vertex_layout = g->Keep("");
		}
		g->vertices.push_back(verts);

		Object topo = m.Ref1("PrimaryTopology");
		std::vector<uint32_t> idx;
		const char *which = topo.ArrayCount("Indices") > 0 ? "Indices" : "Indices16";
		const Member *im = topo.Field(which);
		int icount = topo.ArrayCount(which);
		if (im && icount > 0 && icount < (1 << 24)) {
			const Type *it = f.TypeAt(im->referenceType);
			uint32_t es = it && it->size ? it->size : (strcmp(which, "Indices") ? 2 : 4);
			Ref first = f.Pointer(topo.at.At(im->offset + 4));
			idx.resize((size_t)icount);
			for (int k = 0; k < icount; ++k) {
				Ref r = first.At(es * k);
				if (es == 2) {
					const uint8_t *p = f.Bytes(r, 2);
					idx[k] = p ? (uint32_t)(p[0] | (p[1] << 8)) : 0;
				} else {
					idx[k] = f.U32(r);
				}
				if (idx[k] >= verts.size()) {
					BF_TRACE(BFT_ERR, "granny: mesh %s index %d = %u beyond %zu vertices", mesh.name, k, idx[k], verts.size());
					idx[k] = 0;
				}
			}
		}
		g->indices.push_back(idx);

		std::vector<bf_granny_tri_group> grp;
		for (int k = 0; k < topo.ArrayCount("Groups"); ++k) {
			Object o = topo.ArrayElement("Groups", k);
			bf_granny_tri_group tg = { o.Int("MaterialIndex"), o.Int("TriFirst"), o.Int("TriCount") };
			grp.push_back(tg);
		}
		g->groups.push_back(grp);

		std::vector<int> mats;
		for (int k = 0; k < m.ArrayCount("MaterialBindings"); ++k)
			mats.push_back(IndexOf(materialKeys, m.ArrayElement("MaterialBindings", k).Ref1("Material").at));
		g->meshMaterials.push_back(mats);

		std::vector<bf_granny_bone_binding> bb;
		for (int k = 0; k < m.ArrayCount("BoneBindings"); ++k) {
			Object o = m.ArrayElement("BoneBindings", k);
			bf_granny_bone_binding b;
			memset(&b, 0, sizeof(b));
			b.bone_name = g->Keep(o.Str("BoneName"));
			b.bone = -1;
			if (o.Field("OBBMin")) {
				ReadFloats(f, o.FieldRef("OBBMin"), 3, b.obb_min);
				ReadFloats(f, o.FieldRef("OBBMax"), 3, b.obb_max);
			}
			bb.push_back(b);
		}
		g->bindings.push_back(bb);
		g->meshes.push_back(mesh);
	}

	// Models; bone bindings are resolved against the binding model's skeleton.
	for (int i = 0; i < root.ArrayCount("Models"); ++i) {
		Object o = root.ReferencedElement("Models", i);
		bf_granny_model model;
		memset(&model, 0, sizeof(model));
		model.name = g->Keep(o.Str("Name"));
		model.skeleton = IndexOf(skeletonKeys, o.Ref1("Skeleton").at);
		if (o.Field("InitialPlacement"))
			ReadTransform(f, o.FieldRef("InitialPlacement"), &model.initial_placement);
		std::vector<int> ms;
		for (int k = 0; k < o.ArrayCount("MeshBindings"); ++k) {
			int mi = IndexOf(meshKeys, o.ArrayElement("MeshBindings", k).Ref1("Mesh").at);
			ms.push_back(mi);
			if (mi < 0 || model.skeleton < 0)
				continue;
			const std::vector<bf_granny_bone> &bones = g->bones[model.skeleton];
			for (bf_granny_bone_binding &b : g->bindings[mi]) {
				if (b.bone >= 0)
					continue;
				for (size_t n = 0; n < bones.size(); ++n)
					if (!strcmp(bones[n].name, b.bone_name)) {
						b.bone = (int)n;
						break;
					}
				if (b.bone < 0)
					BF_TRACE(BFT_ERR, "granny: bone %s of mesh %s not in skeleton", b.bone_name, g->meshes[mi].name);
			}
		}
		g->modelMeshes.push_back(ms);
		g->models.push_back(model);
	}

	// Track groups: the file's list, then any an animation names that it lacks.
	std::vector<uint64_t> groupKeys;
	std::vector<Object> groupObjects;
	for (int i = 0; i < root.ArrayCount("TrackGroups"); ++i) {
		Object o = root.ReferencedElement("TrackGroups", i);
		groupKeys.push_back(KeyOf(o.at));
		groupObjects.push_back(o);
	}
	std::vector<Object> animationObjects;
	for (int i = 0; i < root.ArrayCount("Animations"); ++i) {
		Object a = root.ReferencedElement("Animations", i);
		animationObjects.push_back(a);
		std::vector<int> ids;
		for (int k = 0; k < a.ArrayCount("TrackGroups"); ++k) {
			Object o = a.ReferencedElement("TrackGroups", k);
			int id = IndexOf(groupKeys, o.at);
			if (id < 0 && o.Valid()) {
				id = (int)groupKeys.size();
				groupKeys.push_back(KeyOf(o.at));
				groupObjects.push_back(o);
			}
			ids.push_back(id);
		}
		g->animationGroups.push_back(ids);
	}
	for (const Object &o : groupObjects) {
		bf_granny_track_group tg;
		memset(&tg, 0, sizeof(tg));
		tg.name = g->Keep(o.Str("Name"));
		tg.scalar_track_count = o.ArrayCount("ScalarTracks");
		tg.text_track_count = o.ArrayCount("TextTracks");
		tg.accumulation_flags = o.Int("AccumulationFlags");
		if (o.Field("InitialPlacement"))
			ReadTransform(f, o.FieldRef("InitialPlacement"), &tg.initial_placement);
		if (o.Field("LoopTranslation"))
			ReadFloats(f, o.FieldRef("LoopTranslation"), 3, tg.loop_translation);
		std::vector<bf_granny_transform_track> tt;
		for (int k = 0; k < o.ArrayCount("TransformTracks"); ++k) {
			Object t = o.ArrayElement("TransformTracks", k);
			bf_granny_transform_track track;
			track.name = g->Keep(t.Str("Name"));
			ReadCurve(g, t.Inline("PositionCurve"), 3, &track.position);
			ReadCurve(g, t.Inline("OrientationCurve"), 4, &track.orientation);
			ReadCurve(g, t.Inline("ScaleShearCurve"), 9, &track.scale_shear);
			tt.push_back(track);
		}
		g->tracks.push_back(tt);
		g->trackGroups.push_back(tg);
	}
	for (const Object &a : animationObjects) {
		bf_granny_animation an;
		memset(&an, 0, sizeof(an));
		an.name = g->Keep(a.Str("Name"));
		an.duration = a.Real("Duration");
		an.time_step = a.Real("TimeStep");
		g->animations.push_back(an);
	}

	// Pointers into the arrays, now that they no longer grow.
	for (size_t i = 0; i < g->skeletons.size(); ++i) {
		g->skeletons[i].bone_count = (int)g->bones[i].size();
		g->skeletons[i].bones = g->bones[i].data();
	}
	for (size_t i = 0; i < g->meshes.size(); ++i) {
		bf_granny_mesh &m = g->meshes[i];
		m.vertex_count = (int)g->vertices[i].size();
		m.vertices = g->vertices[i].data();
		m.index_count = (int)g->indices[i].size();
		m.indices = g->indices[i].data();
		m.group_count = (int)g->groups[i].size();
		m.groups = g->groups[i].data();
		m.material_count = (int)g->meshMaterials[i].size();
		m.materials = g->meshMaterials[i].data();
		m.bone_binding_count = (int)g->bindings[i].size();
		m.bone_bindings = g->bindings[i].data();
	}
	for (size_t i = 0; i < g->models.size(); ++i) {
		g->models[i].mesh_count = (int)g->modelMeshes[i].size();
		g->models[i].meshes = g->modelMeshes[i].data();
	}
	for (size_t i = 0; i < g->trackGroups.size(); ++i) {
		g->trackGroups[i].transform_track_count = (int)g->tracks[i].size();
		g->trackGroups[i].transform_tracks = g->tracks[i].data();
	}
	for (size_t i = 0; i < g->animations.size(); ++i) {
		g->animations[i].track_group_count = (int)g->animationGroups[i].size();
		g->animations[i].track_groups = g->animationGroups[i].data();
	}

	info.texture_count = (int)g->textures.size();
	info.material_count = (int)g->materials.size();
	info.skeleton_count = (int)g->skeletons.size();
	info.mesh_count = (int)g->meshes.size();
	info.model_count = (int)g->models.size();
	info.track_group_count = (int)g->trackGroups.size();
	info.animation_count = (int)g->animations.size();
}

void SetError(char *error, size_t size, const std::string &text)
{
	if (error && size) {
		strncpy(error, text.c_str(), size - 1);
		error[size - 1] = 0;
	}
}

} // namespace

extern "C" bf_granny_file *bf_granny_open(const void *data, size_t size, char *error, size_t error_size)
{
	SetError(error, error_size, "");
	if (!data) {
		SetError(error, error_size, "no data");
		return nullptr;
	}
	std::unique_ptr<bf_granny_file> g(new bf_granny_file);
	std::string why;
	if (!g->file.Open((const uint8_t *)data, size, why)) {
		BF_TRACE(BFT_ERR, "granny: cannot open: %s", why.c_str());
		SetError(error, error_size, why);
		return nullptr;
	}
	const File &f = g->file;
	bf_granny_info &info = g->info;
	memset(&info, 0, sizeof(info));
	info.version = f.version;
	info.total_size = f.totalSize;
	info.crc = f.crc;
	info.type_tag = f.typeTag;
	info.section_count = (int)f.sections.size();
	info.fixup_count = (int)f.fixupTotal;
	info.root_type_section = (uint32_t)f.rootType.section;
	info.root_type_offset = f.rootType.offset;
	info.root_object_section = (uint32_t)f.rootObject.section;
	info.root_object_offset = f.rootObject.offset;
	for (const Section &s : f.sections) {
		bf_granny_section out;
		out.compression = (int)s.compression;
		out.file_offset = s.fileOffset;
		out.packed_size = s.packedSize;
		out.size = s.size;
		out.alignment = s.alignment;
		out.stop16 = s.stop16;
		out.stop8 = s.stop8;
		out.fixup_count = s.fixupCount;
		out.marshal_count = s.marshalCount;
		out.data = s.data.empty() ? nullptr : s.data.data();
		g->sections.push_back(out);
	}
	if (!f.TypeAt(f.rootType)) {
		BF_TRACE(BFT_ERR, "granny: no root type");
		SetError(error, error_size, "no root type");
		return nullptr;
	}
	Extract(g.get());
	if (f.Bad())
		BF_TRACE(BFT_ERR, "granny: %s: some references pointed outside their section", info.from_file_name);
	return g.release();
}

extern "C" void bf_granny_close(bf_granny_file *file)
{
	delete file;
}

extern "C" const bf_granny_info *bf_granny_get_info(const bf_granny_file *file)
{
	return file ? &file->info : nullptr;
}

template <class T>
static const T *At(const std::vector<T> &v, int index)
{
	return index >= 0 && (size_t)index < v.size() ? &v[index] : nullptr;
}

extern "C" const bf_granny_section *bf_granny_get_section(const bf_granny_file *file, int index)
{
	return file ? At(file->sections, index) : nullptr;
}

extern "C" int bf_granny_root_member(const bf_granny_file *file, int index, const char **name, const char **type,
	uint32_t *offset)
{
	if (!file || index < 0 || (size_t)index >= file->rootMembers.size())
		return 0;
	const Member *m = file->rootMembers[index];
	if (name) *name = m->name.c_str();
	if (type) *type = MemberTypeName(m->type);
	if (offset) *offset = m->offset;
	return 1;
}

extern "C" const bf_granny_texture *bf_granny_get_texture(const bf_granny_file *file, int index)
{
	return file ? At(file->textures, index) : nullptr;
}

extern "C" const bf_granny_material *bf_granny_get_material(const bf_granny_file *file, int index)
{
	return file ? At(file->materials, index) : nullptr;
}

extern "C" const bf_granny_skeleton *bf_granny_get_skeleton(const bf_granny_file *file, int index)
{
	return file ? At(file->skeletons, index) : nullptr;
}

extern "C" const bf_granny_mesh *bf_granny_get_mesh(const bf_granny_file *file, int index)
{
	return file ? At(file->meshes, index) : nullptr;
}

extern "C" const bf_granny_model *bf_granny_get_model(const bf_granny_file *file, int index)
{
	return file ? At(file->models, index) : nullptr;
}

extern "C" const bf_granny_track_group *bf_granny_get_track_group(const bf_granny_file *file, int index)
{
	return file ? At(file->trackGroups, index) : nullptr;
}

extern "C" const bf_granny_animation *bf_granny_get_animation(const bf_granny_file *file, int index)
{
	return file ? At(file->animations, index) : nullptr;
}

extern "C" int bf_granny_texture_pixels(const bf_granny_file *file, int index, uint8_t *bgra, int *has_alpha)
{
	const bf_granny_texture *t = bf_granny_get_texture(file, index);
	if (has_alpha)
		*has_alpha = 0;
	if (!t || !bgra)
		return 0;
	size_t bytes = (size_t)t->width * t->height * 4;
	if (t->encoding == BF_GRANNY_ENCODING_RAW && t->bytes_per_pixel == 4 && t->pixels && t->pixel_size >= bytes) {
		// Raw 8-bit components at the layout's shifts (not used by the RO files).
		for (size_t i = 0; i < bytes / 4; ++i) {
			uint32_t v = t->pixels[4 * i] | (t->pixels[4 * i + 1] << 8) | (t->pixels[4 * i + 2] << 16)
				| ((uint32_t)t->pixels[4 * i + 3] << 24);
			for (int c = 0; c < 4; ++c) {
				int out = c == 0 ? 2 : c == 2 ? 0 : c;   // RGBA component to BGRA byte
				bgra[4 * i + out] = t->bits_for_component[c] ? (uint8_t)(v >> t->shift_for_component[c]) : 255;
			}
		}
		if (has_alpha)
			*has_alpha = t->bits_for_component[3] > 0;
		return 1;
	}
	if (t->encoding != BF_GRANNY_ENCODING_BINK || !t->pixels) {
		BF_TRACE(BFT_ERR, "granny: texture %s: encoding %d is not decoded", t->file_name, t->encoding);
		return 0;
	}
	std::vector<uint8_t> out;
	bool alphaPlane = false;
	std::string error;
	if (!DecodeBinkTexture(t->pixels, t->pixel_size, t->width, t->height, out, alphaPlane, error)) {
		BF_TRACE(BFT_ERR, "granny: texture %s: %s", t->file_name, error.c_str());
		return 0;
	}
	memcpy(bgra, out.data(), bytes);
	if (has_alpha)
		*has_alpha = alphaPlane && t->bits_for_component[3] > 0;
	return 1;
}
