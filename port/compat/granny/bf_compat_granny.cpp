// The Granny adapter (step M12.3a): the Granny* names of RAD's granny2.dll
// that the client calls, on port/native/formats/granny (bf_granny.h).
// README.md here has the table of names; the callers are
// src/resource/Model.cpp (C3dGrannyModelRes, C3dGrannyBoneRes),
// src/GameActor3d.cpp and src/Pc.cpp.
//
// Stage A only: M11 deletes this file and moves the two resource classes
// onto bf_granny.h.
//
// Objects. The client reads some of Granny's structures directly
// (granny_file_info's arrays, granny_texture's size and name, a model's
// skeleton and mesh bindings, a mesh's name and material bindings, a
// skeleton's bone count), so the adapter builds them, as granny.h declares
// them, over the reader's arrays when a file is read. Each such structure is
// the first member of an adapter object that also says which file and which
// index it stands for, so a pointer the client hands back leads to the
// reader's data. The types granny.h leaves opaque (model instance, control,
// local and world pose, mesh binding, deformer) are defined here.
//
// Runtime. Granny's controls are reduced to what the client uses: one
// animation per control, a start time, a loop count (0: forever), active or
// not. Sampling a model instance takes its most recently started active
// control; the client never runs two at once that it wants blended (it
// frees all controls before starting another, and the attack's control has
// ended when the stand control starts). An instance without an active
// control is in its rest pose, as in Granny.
//
// Threads. The resource manager may load on a loader thread, so the table of
// vertex arrays (GrannyDeformVertices finds its mesh by them) has a lock.
// Everything else is used by one thread at a time, as granny2.dll required.

#include "../../../src/granny/granny.h"

#include "bf_granny.h"
#include "bf_trace.h"

#include <mach/mach_time.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <mutex>
#include <unordered_map>
#include <vector>

static_assert(sizeof(bf_granny_pnt332) == sizeof(granny_pnt332_vertex), "granny_pnt332_vertex layout");
static_assert(sizeof(bf_granny_transform) == sizeof(granny_transform), "granny_transform layout");

namespace {

struct FileData;

// Adapter objects: the Granny structure first, then where it comes from.
struct FileObj      { granny_file g;      FileData *data; };
struct TextureObj   { granny_texture g;   FileObj *file; int index; uint8_t *bgra; int has_alpha; int decoded; };
struct MaterialObj  { granny_material g;  FileObj *file; int index; };
struct SkeletonObj  { granny_skeleton g;  FileObj *file; int index; };
struct MeshObj      { granny_mesh g;      FileObj *file; int index; };
struct ModelObj     { granny_model g;     FileObj *file; int index; };
struct AnimationObj { granny_animation g; FileObj *file; int index; };

struct MeshExtra {
	std::vector<granny_material_binding> materials;
	std::vector<granny_bone_binding> bones;
	std::vector<granny_tri_material_group> groups;
	granny_vertex_data vertex_data;
	granny_tri_topology topology;
};

// Everything built for one file. The vectors are sized once, before any
// pointer into them is handed out.
struct FileData {
	bf_granny_file *bf = nullptr;
	granny_file_info info;
	granny_art_tool_info art_tool;
	granny_exporter_info exporter;
	std::vector<TextureObj> textures;
	std::vector<granny_texture *> texture_ptrs;
	std::vector<MaterialObj> materials;
	std::vector<granny_material *> material_ptrs;
	std::vector<SkeletonObj> skeletons;
	std::vector<granny_skeleton *> skeleton_ptrs;
	std::vector<std::vector<granny_bone>> bones;
	std::vector<MeshObj> meshes;
	std::vector<granny_mesh *> mesh_ptrs;
	std::vector<MeshExtra> mesh_extra;
	std::vector<ModelObj> models;
	std::vector<granny_model *> model_ptrs;
	std::vector<std::vector<granny_model_mesh_binding>> model_meshes;
	std::vector<AnimationObj> animations;
	std::vector<granny_animation *> animation_ptrs;
};

FileObj *AsFile(granny_file *f) { return reinterpret_cast<FileObj *>(f); }
TextureObj *AsTexture(granny_texture *t) { return reinterpret_cast<TextureObj *>(t); }
MaterialObj *AsMaterial(granny_material *m) { return reinterpret_cast<MaterialObj *>(m); }
SkeletonObj *AsSkeleton(granny_skeleton *s) { return reinterpret_cast<SkeletonObj *>(s); }
MeshObj *AsMesh(granny_mesh *m) { return reinterpret_cast<MeshObj *>(m); }
ModelObj *AsModel(granny_model *m) { return reinterpret_cast<ModelObj *>(m); }
AnimationObj *AsAnimation(granny_animation *a) { return reinterpret_cast<AnimationObj *>(a); }

const bf_granny_mesh *BfMesh(granny_mesh *m)
{
	MeshObj *o = AsMesh(m);
	return o ? bf_granny_get_mesh(o->file->data->bf, o->index) : nullptr;
}

const bf_granny_skeleton *BfSkeleton(granny_skeleton *s)
{
	SkeletonObj *o = AsSkeleton(s);
	return o ? bf_granny_get_skeleton(o->file->data->bf, o->index) : nullptr;
}

// GrannyDeformVertices is given the source vertices, not the mesh: the
// vertex arrays of every open file, to find the mesh again.
std::mutex g_vertices_lock;
std::unordered_map<const void *, MeshObj *> g_vertices;

// Vertex layouts: granny_pnt332_vertex, and the reader's own vertex
// (bf_granny_vertex), which is what GrannyGetMeshVertices hands out.
granny_data_type_definition g_pnt332[] = {
	{ GrannyReal32Member, "Position", nullptr, 3, { 0, 0, 0 }, 0 },
	{ GrannyReal32Member, "Normal", nullptr, 3, { 0, 0, 0 }, 0 },
	{ GrannyReal32Member, "TextureCoordinates0", nullptr, 2, { 0, 0, 0 }, 0 },
	{ GrannyEndMember, nullptr, nullptr, 0, { 0, 0, 0 }, 0 },
};
granny_data_type_definition g_source_vertex[] = {
	{ GrannyReal32Member, "Position", nullptr, 3, { 0, 0, 0 }, 0 },
	{ GrannyReal32Member, "Normal", nullptr, 3, { 0, 0, 0 }, 0 },
	{ GrannyReal32Member, "TextureCoordinates0", nullptr, 2, { 0, 0, 0 }, 0 },
	{ GrannyUInt8Member, "BoneIndices", nullptr, 4, { 0, 0, 0 }, 0 },
	{ GrannyNormalUInt8Member, "BoneWeights", nullptr, 4, { 0, 0, 0 }, 0 },
	{ GrannyEndMember, nullptr, nullptr, 0, { 0, 0, 0 }, 0 },
};

// Bytes R, G, B, A in memory.
granny_pixel_layout g_rgba8888 = { 4, { 0, 8, 16, 24 }, { 8, 8, 8, 8 } };

void CopyTransform(granny_transform &out, const bf_granny_transform &in)
{
	memcpy(&out, &in, sizeof out);
}

FileObj *Build(bf_granny_file *bf)
{
	const bf_granny_info *in = bf_granny_get_info(bf);
	FileObj *obj = new FileObj;
	memset(&obj->g, 0, sizeof obj->g);
	FileData *d = obj->data = new FileData;
	d->bf = bf;
	obj->g.SectionCount = in->section_count;

	memset(&d->art_tool, 0, sizeof d->art_tool);
	d->art_tool.FromArtToolName = (char *)in->art_tool;
	d->art_tool.UnitsPerMeter = in->units_per_meter;
	memcpy(d->art_tool.Origin, in->origin, sizeof in->origin);
	memcpy(d->art_tool.RightVector, in->right, sizeof in->right);
	memcpy(d->art_tool.UpVector, in->up, sizeof in->up);
	memcpy(d->art_tool.BackVector, in->back, sizeof in->back);
	memset(&d->exporter, 0, sizeof d->exporter);
	d->exporter.ExporterName = (char *)in->exporter;
	d->exporter.ExporterMajorRevision = in->exporter_version[0];
	d->exporter.ExporterMinorRevision = in->exporter_version[1];
	d->exporter.ExporterCustomization = in->exporter_version[2];
	d->exporter.ExporterBuildNumber = in->exporter_version[3];

	d->textures.resize((size_t)in->texture_count);
	d->texture_ptrs.resize((size_t)in->texture_count);
	for (int i = 0; i < in->texture_count; ++i) {
		const bf_granny_texture *t = bf_granny_get_texture(bf, i);
		TextureObj &o = d->textures[(size_t)i];
		memset(&o, 0, sizeof o);
		o.file = obj;
		o.index = i;
		o.g.FromFileName = (char *)t->file_name;
		o.g.TextureType = t->texture_type;
		o.g.Width = t->width;
		o.g.Height = t->height;
		o.g.Encoding = t->encoding;
		o.g.SubFormat = t->sub_format;
		o.g.Layout.BytesPerPixel = t->bytes_per_pixel;
		memcpy(o.g.Layout.ShiftForComponent, t->shift_for_component, sizeof t->shift_for_component);
		memcpy(o.g.Layout.BitsForComponent, t->bits_for_component, sizeof t->bits_for_component);
		o.g.ImageCount = t->image_count;   // the images themselves: GrannyCopyTextureImage
		d->texture_ptrs[(size_t)i] = &o.g;
	}

	d->materials.resize((size_t)in->material_count);
	d->material_ptrs.resize((size_t)in->material_count);
	for (int i = 0; i < in->material_count; ++i) {
		const bf_granny_material *m = bf_granny_get_material(bf, i);
		MaterialObj &o = d->materials[(size_t)i];
		memset(&o, 0, sizeof o);
		o.file = obj;
		o.index = i;
		o.g.Name = (char *)m->name;
		o.g.Texture = m->texture >= 0 && m->texture < in->texture_count ? &d->textures[(size_t)m->texture].g : nullptr;
		d->material_ptrs[(size_t)i] = &o.g;
	}

	d->skeletons.resize((size_t)in->skeleton_count);
	d->skeleton_ptrs.resize((size_t)in->skeleton_count);
	d->bones.resize((size_t)in->skeleton_count);
	for (int i = 0; i < in->skeleton_count; ++i) {
		const bf_granny_skeleton *s = bf_granny_get_skeleton(bf, i);
		std::vector<granny_bone> &bones = d->bones[(size_t)i];
		bones.resize((size_t)s->bone_count);
		for (int b = 0; b < s->bone_count; ++b) {
			granny_bone &g = bones[(size_t)b];
			memset(&g, 0, sizeof g);
			g.Name = (char *)s->bones[b].name;
			g.ParentIndex = s->bones[b].parent;
			CopyTransform(g.LocalTransform, s->bones[b].local);
			memcpy(g.InverseWorld4x4, s->bones[b].inverse_world, sizeof g.InverseWorld4x4);
		}
		SkeletonObj &o = d->skeletons[(size_t)i];
		memset(&o, 0, sizeof o);
		o.file = obj;
		o.index = i;
		o.g.Name = (char *)s->name;
		o.g.BoneCount = s->bone_count;
		o.g.Bones = bones.empty() ? nullptr : bones.data();
		d->skeleton_ptrs[(size_t)i] = &o.g;
	}

	d->meshes.resize((size_t)in->mesh_count);
	d->mesh_ptrs.resize((size_t)in->mesh_count);
	d->mesh_extra.resize((size_t)in->mesh_count);
	for (int i = 0; i < in->mesh_count; ++i) {
		const bf_granny_mesh *m = bf_granny_get_mesh(bf, i);
		MeshExtra &x = d->mesh_extra[(size_t)i];
		x.materials.resize((size_t)m->material_count);
		for (int k = 0; k < m->material_count; ++k) {
			int mat = m->materials[k];
			x.materials[(size_t)k].Material =
				mat >= 0 && mat < in->material_count ? &d->materials[(size_t)mat].g : nullptr;
		}
		x.bones.resize((size_t)m->bone_binding_count);
		for (int k = 0; k < m->bone_binding_count; ++k) {
			granny_bone_binding &b = x.bones[(size_t)k];
			memset(&b, 0, sizeof b);
			b.BoneName = (char *)m->bone_bindings[k].bone_name;
			memcpy(b.OBBMin, m->bone_bindings[k].obb_min, sizeof b.OBBMin);
			memcpy(b.OBBMax, m->bone_bindings[k].obb_max, sizeof b.OBBMax);
		}
		x.groups.resize((size_t)m->group_count);
		for (int k = 0; k < m->group_count; ++k) {
			x.groups[(size_t)k].MaterialIndex = m->groups[k].material;
			x.groups[(size_t)k].TriFirst = m->groups[k].first_triangle;
			x.groups[(size_t)k].TriCount = m->groups[k].triangle_count;
		}
		memset(&x.vertex_data, 0, sizeof x.vertex_data);
		x.vertex_data.VertexType = g_source_vertex;
		x.vertex_data.VertexCount = m->vertex_count;
		x.vertex_data.Vertices = (granny_uint8 *)m->vertices;
		memset(&x.topology, 0, sizeof x.topology);
		x.topology.GroupCount = m->group_count;
		x.topology.Groups = x.groups.empty() ? nullptr : x.groups.data();
		x.topology.IndexCount = m->index_count;
		x.topology.Indices = (granny_int32 *)m->indices;

		MeshObj &o = d->meshes[(size_t)i];
		memset(&o, 0, sizeof o);
		o.file = obj;
		o.index = i;
		o.g.Name = (char *)m->name;
		o.g.PrimaryVertexData = &x.vertex_data;
		o.g.PrimaryTopology = &x.topology;
		o.g.MaterialBindingCount = m->material_count;
		o.g.MaterialBindings = x.materials.empty() ? nullptr : x.materials.data();
		o.g.BoneBindingCount = m->bone_binding_count;
		o.g.BoneBindings = x.bones.empty() ? nullptr : x.bones.data();
		d->mesh_ptrs[(size_t)i] = &o.g;
		if (m->vertices) {
			std::lock_guard<std::mutex> lock(g_vertices_lock);
			g_vertices[m->vertices] = &o;
		}
	}

	d->models.resize((size_t)in->model_count);
	d->model_ptrs.resize((size_t)in->model_count);
	d->model_meshes.resize((size_t)in->model_count);
	for (int i = 0; i < in->model_count; ++i) {
		const bf_granny_model *m = bf_granny_get_model(bf, i);
		std::vector<granny_model_mesh_binding> &mb = d->model_meshes[(size_t)i];
		for (int k = 0; k < m->mesh_count; ++k) {
			int mesh = m->meshes[k];
			if (mesh >= 0 && mesh < in->mesh_count) {
				granny_model_mesh_binding b = { &d->meshes[(size_t)mesh].g };
				mb.push_back(b);
			}
		}
		ModelObj &o = d->models[(size_t)i];
		memset(&o, 0, sizeof o);
		o.file = obj;
		o.index = i;
		o.g.Name = (char *)m->name;
		o.g.Skeleton = m->skeleton >= 0 && m->skeleton < in->skeleton_count ? &d->skeletons[(size_t)m->skeleton].g : nullptr;
		CopyTransform(o.g.InitialPlacement, m->initial_placement);
		o.g.MeshBindingCount = (granny_int32)mb.size();
		o.g.MeshBindings = mb.empty() ? nullptr : mb.data();
		d->model_ptrs[(size_t)i] = &o.g;
	}

	d->animations.resize((size_t)in->animation_count);
	d->animation_ptrs.resize((size_t)in->animation_count);
	for (int i = 0; i < in->animation_count; ++i) {
		const bf_granny_animation *a = bf_granny_get_animation(bf, i);
		AnimationObj &o = d->animations[(size_t)i];
		memset(&o, 0, sizeof o);
		o.file = obj;
		o.index = i;
		o.g.Name = (char *)a->name;
		o.g.Duration = a->duration;
		o.g.TimeStep = a->time_step;
		// Track groups are the reader's (bf_granny_bind_animation); the
		// client never reads them.
		o.g.TrackGroupCount = 0;
		o.g.TrackGroups = nullptr;
		d->animation_ptrs[(size_t)i] = &o.g;
	}

	granny_file_info &fi = d->info;
	memset(&fi, 0, sizeof fi);
	fi.ArtToolInfo = &d->art_tool;
	fi.ExporterInfo = &d->exporter;
	fi.FromFileName = (char *)in->from_file_name;
	fi.TextureCount = in->texture_count;
	fi.Textures = d->texture_ptrs.empty() ? nullptr : d->texture_ptrs.data();
	fi.MaterialCount = in->material_count;
	fi.Materials = d->material_ptrs.empty() ? nullptr : d->material_ptrs.data();
	fi.SkeletonCount = in->skeleton_count;
	fi.Skeletons = d->skeleton_ptrs.empty() ? nullptr : d->skeleton_ptrs.data();
	fi.MeshCount = in->mesh_count;
	fi.Meshes = d->mesh_ptrs.empty() ? nullptr : d->mesh_ptrs.data();
	fi.ModelCount = in->model_count;
	fi.Models = d->model_ptrs.empty() ? nullptr : d->model_ptrs.data();
	fi.AnimationCount = in->animation_count;
	fi.Animations = d->animation_ptrs.empty() ? nullptr : d->animation_ptrs.data();
	return obj;
}

bool Decode(TextureObj *t)
{
	if (!t->decoded) {
		t->decoded = 1;
		size_t n = (size_t)t->g.Width * (size_t)t->g.Height * 4;
		t->bgra = n ? (uint8_t *)malloc(n) : nullptr;
		if (t->bgra && !bf_granny_texture_pixels(t->file->data->bf, t->index, t->bgra, &t->has_alpha)) {
			free(t->bgra);
			t->bgra = nullptr;
		}
		if (!t->bgra) {
			bf_count("granny.texture_failed", 1);
			BF_TRACE(BFT_RES, "granny: texture %d (%s) not decoded", t->index, t->g.FromFileName ? t->g.FromFileName : "?");
		}
	}
	return t->bgra != nullptr;
}

// A control: one animation bound to one instance's skeleton.
const float kForever = 1.0e30f;

}  // namespace

// The types granny.h leaves opaque.
struct granny_local_pose {
	std::vector<bf_granny_transform> local;
};

struct granny_world_pose {
	int bone_count;
	std::vector<float> world, composite;
};

struct granny_mesh_binding {
	std::vector<granny_int32> to_bone;
};

struct granny_mesh_deformer {
	int type;
};

struct granny_model_instance {
	ModelObj *model;
	float clock;
	std::vector<granny_control *> controls;   // in the order they were started
	bf_granny_binding *rest;                  // the rest pose, made when first needed
};

struct granny_control {
	granny_model_instance *instance;          // NULL once the instance is freed
	bf_granny_binding *binding;
	float start, duration, clock;
	int loops;                                // 0: forever
	bool active;
};

extern "C" {

granny_data_type_definition *GrannyPNT332VertexType = g_pnt332;
granny_pixel_layout *GrannyRGBA8888PixelFormat = &g_rgba8888;

char GrannyVersionsMatch_(granny_int32x MajorVersion, granny_int32x MinorVersion, granny_int32x Customization,
	granny_int32x BuildNumber)
{
	// The client was built against 2.1.0.5; the reader reads what that
	// exporter wrote.
	return MajorVersion == 2 && MinorVersion == 1;
}

// ---- files

granny_file *GrannyReadEntireFileFromMemory(granny_int32x MemorySize, void *Memory)
{
	if (!Memory || MemorySize <= 0)
		return nullptr;
	char error[256] = "";
	bf_granny_file *bf = bf_granny_open(Memory, (size_t)MemorySize, error, sizeof error);
	if (!bf) {
		bf_count("granny.file_refused", 1);
		BF_TRACE(BFT_RES, "granny: file refused (%d bytes): %s", (int)MemorySize, error);
		return nullptr;
	}
	bf_count("granny.files", 1);
	return &Build(bf)->g;
}

granny_file_info *GrannyGetFileInfo(granny_file *File)
{
	return File ? &AsFile(File)->data->info : nullptr;
}

void GrannyFreeFileSection(granny_file *File, granny_int32x SectionIndex)
{
	// The reader keeps its sections until the file is closed; the client
	// frees the vertex, index and texture sections after loading, which
	// only saved memory in granny2.dll.
}

void GrannyFreeFile(granny_file *File)
{
	if (!File)
		return;
	FileObj *obj = AsFile(File);
	FileData *d = obj->data;
	{
		std::lock_guard<std::mutex> lock(g_vertices_lock);
		for (MeshObj &m : d->meshes) {
			const bf_granny_mesh *bm = bf_granny_get_mesh(d->bf, m.index);
			if (bm && bm->vertices)
				g_vertices.erase(bm->vertices);
		}
	}
	for (TextureObj &t : d->textures)
		free(t.bgra);
	bf_granny_close(d->bf);
	delete d;
	delete obj;
}

// ---- textures and materials

int GrannyTextureHasAlpha(granny_texture *Texture)
{
	TextureObj *t = AsTexture(Texture);
	return t && Decode(t) ? t->has_alpha : 0;
}

void GrannyCopyTextureImage(granny_texture *Texture, granny_int32x ImageIndex, granny_int32x MIPIndex,
	granny_pixel_layout *Layout, granny_int32x DestWidth, granny_int32x DestHeight, granny_int32x DestStride,
	void *Pixels)
{
	TextureObj *t = AsTexture(Texture);
	if (!t || !Layout || !Pixels || DestWidth <= 0 || DestHeight <= 0)
		return;
	int bpp = Layout->BytesPerPixel;
	if (bpp < 1 || bpp > 4)
		return;
	bool ok = ImageIndex == 0 && MIPIndex == 0 && Decode(t);
	if (!ok)
		BF_TRACE(BFT_RES, "granny: texture %d image %d MIP %d not available", t->index, (int)ImageIndex, (int)MIPIndex);
	for (int y = 0; y < DestHeight; ++y) {
		uint8_t *row = (uint8_t *)Pixels + (size_t)y * (size_t)DestStride;
		for (int x = 0; x < DestWidth; ++x) {
			uint8_t c[4] = { 0, 0, 0, 0 };   // R, G, B, A
			if (ok) {
				// nearest pixel when the destination is another size
				int sx = (int)((long long)x * t->g.Width / DestWidth);
				int sy = (int)((long long)y * t->g.Height / DestHeight);
				const uint8_t *p = t->bgra + ((size_t)sy * (size_t)t->g.Width + (size_t)sx) * 4;
				c[0] = p[2];
				c[1] = p[1];
				c[2] = p[0];
				c[3] = p[3];
			}
			uint32_t word = 0;
			for (int k = 0; k < 4; ++k) {
				int bits = Layout->BitsForComponent[k];
				if (bits <= 0)
					continue;
				uint32_t v = bits >= 8 ? (uint32_t)c[k] << (bits - 8) : (uint32_t)c[k] >> (8 - bits);
				word |= v << Layout->ShiftForComponent[k];
			}
			for (int k = 0; k < bpp; ++k)
				row[x * bpp + k] = (uint8_t)(word >> (8 * k));
		}
	}
}

granny_texture *GrannyGetMaterialTextureByType(granny_material *Material, int Type)
{
	MaterialObj *o = AsMaterial(Material);
	if (!o)
		return nullptr;
	FileData *d = o->file->data;
	const bf_granny_material *m = bf_granny_get_material(d->bf, o->index);
	// Granny's material texture types: 2 = diffuse colour; the RO files
	// have no other map the client asks for.
	int index = Type == 2 ? m->diffuse_texture : m->texture;
	if (index < 0 || index >= (int)d->textures.size())
		return nullptr;
	return &d->textures[(size_t)index].g;
}

// ---- meshes

granny_int32x GrannyGetMeshVertexCount(granny_mesh *Mesh)
{
	const bf_granny_mesh *m = BfMesh(Mesh);
	return m ? m->vertex_count : 0;
}

granny_data_type_definition *GrannyGetMeshVertexType(granny_mesh *Mesh)
{
	return Mesh ? g_source_vertex : nullptr;
}

void *GrannyGetMeshVertices(granny_mesh *Mesh)
{
	const bf_granny_mesh *m = BfMesh(Mesh);
	return m ? (void *)m->vertices : nullptr;
}

bool GrannyMeshIsRigid(granny_mesh *Mesh)
{
	const bf_granny_mesh *m = BfMesh(Mesh);
	return m && m->rigid;
}

void GrannyCopyMeshVertices(granny_mesh *Mesh, granny_data_type_definition *VertexType, void *DestVertices)
{
	const bf_granny_mesh *m = BfMesh(Mesh);
	if (!m || !DestVertices)
		return;
	if (VertexType != g_pnt332) {
		BF_TRACE(BFT_RES, "granny: GrannyCopyMeshVertices to a layout other than PNT332 (not done)");
		return;
	}
	granny_pnt332_vertex *out = (granny_pnt332_vertex *)DestVertices;
	for (int v = 0; v < m->vertex_count; ++v) {
		memcpy(out[v].Position, m->vertices[v].position, sizeof out[v].Position);
		memcpy(out[v].Normal, m->vertices[v].normal, sizeof out[v].Normal);
		memcpy(out[v].UV, m->vertices[v].uv, sizeof out[v].UV);
	}
}

granny_int32x GrannyGetMeshIndexCount(granny_mesh *Mesh)
{
	const bf_granny_mesh *m = BfMesh(Mesh);
	return m ? m->index_count : 0;
}

granny_int32x GrannyGetMeshBytesPerIndex(granny_mesh *Mesh)
{
	return 4;   // the reader widens every index to 32 bits
}

void *GrannyGetMeshIndices(granny_mesh *Mesh)
{
	const bf_granny_mesh *m = BfMesh(Mesh);
	return m ? (void *)m->indices : nullptr;
}

void GrannyCopyMeshIndices(granny_mesh *Mesh, granny_int32x BytesPerIndex, void *DestIndices)
{
	const bf_granny_mesh *m = BfMesh(Mesh);
	if (!m || !DestIndices)
		return;
	for (int i = 0; i < m->index_count; ++i) {
		uint32_t v = m->indices[i];
		if (BytesPerIndex == 2) {
			if (v > 0xffff)
				bf_count("granny.index_truncated", 1);
			((uint16_t *)DestIndices)[i] = (uint16_t)v;
		} else if (BytesPerIndex == 4) {
			((uint32_t *)DestIndices)[i] = v;
		} else if (BytesPerIndex == 1) {
			((uint8_t *)DestIndices)[i] = (uint8_t)v;
		}
	}
}

granny_tri_material_group *GrannyGetMeshTriangleGroups(granny_mesh *Mesh)
{
	MeshObj *o = AsMesh(Mesh);
	if (!o)
		return nullptr;
	MeshExtra &x = o->file->data->mesh_extra[(size_t)o->index];
	return x.groups.empty() ? nullptr : x.groups.data();
}

// A mesh's bone bindings matched by name to a skeleton's bones, which may
// be another file's (C3dGrannyModelRes::BindMeshToSkeleton).
granny_mesh_binding *GrannyNewMeshBinding(granny_mesh *Mesh, granny_skeleton *FromSkeleton, granny_skeleton *ToSkeleton)
{
	const bf_granny_mesh *m = BfMesh(Mesh);
	const bf_granny_skeleton *to = BfSkeleton(ToSkeleton);
	if (!m || !to)
		return nullptr;
	granny_mesh_binding *b = new granny_mesh_binding;
	b->to_bone.resize((size_t)m->bone_binding_count);
	for (int k = 0; k < m->bone_binding_count; ++k) {
		const char *name = m->bone_bindings[k].bone_name;
		int found = -1;
		for (int i = 0; name && i < to->bone_count && found < 0; ++i)
			if (to->bones[i].name && strcmp(to->bones[i].name, name) == 0)
				found = i;
		for (int i = 0; name && i < to->bone_count && found < 0; ++i)
			if (to->bones[i].name && strcasecmp(to->bones[i].name, name) == 0)
				found = i;
		if (found < 0) {
			bf_count("granny.bone_unbound", 1);
			BF_TRACE(BFT_RES, "granny: mesh %s: bone %s not in skeleton %s", m->name ? m->name : "?",
				name ? name : "?", to->name ? to->name : "?");
			found = 0;
		}
		b->to_bone[(size_t)k] = found;
	}
	return b;
}

void GrannyFreeMeshBinding(granny_mesh_binding *Binding)
{
	delete Binding;
}

granny_int32x *GrannyGetMeshBindingToBoneIndices(granny_mesh_binding *Binding)
{
	return Binding && !Binding->to_bone.empty() ? Binding->to_bone.data() : nullptr;
}

granny_mesh_deformer *GrannyNewMeshDeformer(granny_data_type_definition *InputVertexLayout,
	granny_data_type_definition *OutputVertexLayout, int DeformationType)
{
	if (OutputVertexLayout != g_pnt332) {
		BF_TRACE(BFT_RES, "granny: deformer to a layout other than PNT332 (not done)");
		return nullptr;
	}
	granny_mesh_deformer *d = new granny_mesh_deformer;
	d->type = DeformationType;
	return d;
}

void GrannyFreeMeshDeformer(granny_mesh_deformer *Deformer)
{
	delete Deformer;
}

void GrannyDeformVertices(granny_mesh_deformer *Deformer, granny_int32x *MatrixIndices, granny_real32 *MatrixBuffer4x4,
	granny_int32x VertexCount, void *SourceVertices, granny_pnt332_vertex *DestVertices)
{
	if (!Deformer || !MatrixBuffer4x4 || !SourceVertices || !DestVertices)
		return;
	MeshObj *mesh = nullptr;
	{
		std::lock_guard<std::mutex> lock(g_vertices_lock);
		auto it = g_vertices.find(SourceVertices);
		if (it != g_vertices.end())
			mesh = it->second;
	}
	if (!mesh) {
		bf_count("granny.deform_unknown_vertices", 1);
		return;
	}
	const bf_granny_mesh *m = bf_granny_get_mesh(mesh->file->data->bf, mesh->index);
	if (VertexCount != m->vertex_count) {
		bf_count("granny.deform_count_mismatch", 1);
		return;
	}
	bf_granny_deform(mesh->file->data->bf, mesh->index, MatrixIndices, MatrixBuffer4x4, (bf_granny_pnt332 *)DestVertices);
}

// ---- models, poses, controls

granny_model_instance *GrannyInstantiateModel(granny_model *Model)
{
	if (!Model)
		return nullptr;
	granny_model_instance *i = new granny_model_instance;
	i->model = AsModel(Model);
	i->clock = 0;
	i->rest = nullptr;
	return i;
}

void GrannyFreeModelInstance(granny_model_instance *ModelInstance)
{
	if (!ModelInstance)
		return;
	// Controls still on the instance stay valid until GrannyFreeControl
	// (the client frees its controls itself); they no longer move it.
	for (granny_control *c : ModelInstance->controls)
		c->instance = nullptr;
	bf_granny_free_binding(ModelInstance->rest);
	delete ModelInstance;
}

granny_skeleton *GrannyGetSourceSkeleton(granny_model_instance *Model)
{
	return Model ? Model->model->g.Skeleton : nullptr;
}

void GrannySetModelClock(granny_model_instance *ModelInstance, granny_real32 NewClock)
{
	if (!ModelInstance)
		return;
	ModelInstance->clock = NewClock;
	for (granny_control *c : ModelInstance->controls)
		c->clock = NewClock;
}

void GrannyFreeCompletedModelControls(granny_model_instance *ModelInstance)
{
	// Granny frees only controls marked for completion (GrannyCompleteControlAt
	// and its kind), which the client never does: its finished attack
	// control is still asked for its duration afterwards
	// (C3dGrannyGameActor::ProcessAni).
}

granny_control *GrannyPlayControlledAnimation(granny_real32 StartTime, granny_animation *Animation,
	granny_model_instance *Model)
{
	AnimationObj *a = AsAnimation(Animation);
	if (!a || !Model)
		return nullptr;
	ModelObj *m = Model->model;
	bf_granny_binding *b = bf_granny_bind_animation(m->file->data->bf, m->index, a->file->data->bf, a->index);
	if (!b) {
		bf_count("granny.animation_unbound", 1);
		BF_TRACE(BFT_RES, "granny: animation %s does not bind to model %s", a->g.Name ? a->g.Name : "?",
			m->g.Name ? m->g.Name : "?");
		return nullptr;
	}
	int bones = 0, matched = 0;
	float duration = 0;
	bf_granny_binding_info(b, &bones, &matched, &duration);
	if (matched == 0) {
		bf_count("granny.animation_no_tracks", 1);
		BF_TRACE(BFT_RES, "granny: animation %s moves no bone of model %s", a->g.Name ? a->g.Name : "?",
			m->g.Name ? m->g.Name : "?");
	}
	granny_control *c = new granny_control;
	c->instance = Model;
	c->binding = b;
	c->start = StartTime;
	c->duration = duration;
	c->clock = Model->clock;
	c->loops = 1;               // Granny's default
	c->active = true;
	Model->controls.push_back(c);
	return c;
}

void GrannySetControlLoopCount(granny_control *Control, granny_int32x LoopCount)
{
	if (Control)
		Control->loops = LoopCount < 0 ? 0 : LoopCount;
}

void GrannySetControlActive(granny_control *Control, int Active)
{
	if (Control)
		Control->active = Active != 0;
}

granny_real32 GrannyGetControlDurationLeft(granny_control *Control)
{
	if (!Control)
		return 0;
	if (Control->loops == 0)
		return kForever;
	return Control->loops * Control->duration - (Control->clock - Control->start);
}

void GrannyFreeControl(granny_control *Control)
{
	if (!Control)
		return;
	if (Control->instance) {
		std::vector<granny_control *> &list = Control->instance->controls;
		for (size_t i = 0; i < list.size(); ++i)
			if (list[i] == Control) {
				list.erase(list.begin() + (long)i);
				break;
			}
	}
	bf_granny_free_binding(Control->binding);
	delete Control;
}

granny_local_pose *GrannyNewLocalPose(granny_int32x BoneCount)
{
	granny_local_pose *p = new granny_local_pose;
	p->local.resize(BoneCount > 0 ? (size_t)BoneCount : 0);
	return p;
}

void GrannyFreeLocalPose(granny_local_pose *LocalPose)
{
	delete LocalPose;
}

granny_world_pose *GrannyNewWorldPose(granny_int32x BoneCount)
{
	granny_world_pose *p = new granny_world_pose;
	p->bone_count = BoneCount > 0 ? BoneCount : 0;
	p->world.assign((size_t)p->bone_count * 16, 0.0f);
	p->composite.assign((size_t)p->bone_count * 16, 0.0f);
	for (int b = 0; b < p->bone_count; ++b)
		for (int k = 0; k < 4; ++k)
			p->world[(size_t)b * 16 + (size_t)k * 5] = p->composite[(size_t)b * 16 + (size_t)k * 5] = 1.0f;
	return p;
}

void GrannyFreeWorldPose(granny_world_pose *WorldPose)
{
	delete WorldPose;
}

// The pose of the most recently started active control at the instance's
// clock, looped by its loop count and held at its end once the loops are
// over; the rest pose without one.
void GrannySampleModelAnimations(granny_model_instance *ModelInstance, granny_int32x FirstBone, granny_int32x BoneCount,
	granny_local_pose *Result)
{
	if (!ModelInstance || !Result)
		return;
	granny_control *use = nullptr;
	for (size_t i = ModelInstance->controls.size(); i-- > 0;)
		if (ModelInstance->controls[i]->active) {
			use = ModelInstance->controls[i];
			break;
		}
	const bf_granny_binding *b;
	float t = 0;
	if (use) {
		b = use->binding;
		t = use->clock - use->start;
		if (t < 0)
			t = 0;
		if (use->duration > 0) {
			if (use->loops > 0 && t >= use->loops * use->duration)
				t = use->duration;
			else
				t = fmodf(t, use->duration);
		}
	} else {
		if (!ModelInstance->rest) {
			ModelObj *m = ModelInstance->model;
			ModelInstance->rest = bf_granny_bind_animation(m->file->data->bf, m->index, nullptr, 0);
		}
		b = ModelInstance->rest;
	}
	int bones = 0;
	if (!b || !bf_granny_binding_info(b, &bones, nullptr, nullptr))
		return;
	if ((int)Result->local.size() < bones)
		Result->local.resize((size_t)bones);
	bf_granny_sample_local(b, t, Result->local.data());
}

void GrannyBuildWorldPose(granny_skeleton *Skeleton, granny_int32x FirstBone, granny_int32x BoneCount,
	granny_local_pose *LocalPose, void *Offset4x4, granny_world_pose *Result)
{
	const bf_granny_skeleton *s = BfSkeleton(Skeleton);
	if (!s || !LocalPose || !Result)
		return;
	if ((int)LocalPose->local.size() < s->bone_count)
		return;
	if (Result->bone_count < s->bone_count) {
		Result->bone_count = s->bone_count;
		Result->world.resize((size_t)s->bone_count * 16);
		Result->composite.resize((size_t)s->bone_count * 16);
	}
	bf_granny_build_world(s, LocalPose->local.data(), (const float *)Offset4x4, Result->world.data(),
		Result->composite.data());
}

granny_matrix_4x4 *GrannyGetWorldPoseComposite4x4Array(granny_world_pose *WorldPose)
{
	if (!WorldPose || WorldPose->composite.empty())
		return nullptr;
	return (granny_matrix_4x4 *)WorldPose->composite.data();
}

void *GrannyGetWorldPoseComposite4x4(granny_world_pose *WorldPose, granny_int32x BoneIndex)
{
	if (!WorldPose || BoneIndex < 0 || BoneIndex >= WorldPose->bone_count)
		return nullptr;
	return WorldPose->composite.data() + (size_t)BoneIndex * 16;
}

// ---- clock

granny_system_clock GrannyGetSystemSeconds()
{
	granny_system_clock c;
	memset(&c, 0, sizeof c);
	uint64_t now = mach_absolute_time();
	c.Data[0] = (granny_uint32)now;
	c.Data[1] = (granny_uint32)(now >> 32);
	return c;
}

granny_real32 GrannyGetSecondsElapsed(granny_system_clock *StartClock, granny_system_clock *EndClock)
{
	if (!StartClock || !EndClock)
		return 0;
	static mach_timebase_info_data_t base;
	if (base.denom == 0)
		mach_timebase_info(&base);
	uint64_t a = (uint64_t)StartClock->Data[0] | ((uint64_t)StartClock->Data[1] << 32);
	uint64_t b = (uint64_t)EndClock->Data[0] | ((uint64_t)EndClock->Data[1] << 32);
	double ticks = (double)(int64_t)(b - a);
	return (granny_real32)(ticks * base.numer / base.denom / 1.0e9);
}

}  // extern "C"
