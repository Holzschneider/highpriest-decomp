// The client's runtime (bf_granny.h, "The client's runtime", M11.10): the
// call sequences of src/resource/Model.cpp and src/GameActor3d.cpp (the
// patterns, not the client's code), on the game's files. Moved from the
// test of the Granny adapter (port/compat/granny), which M11.10 deleted;
// the checks are the same.

#include "bf_granny.h"
#include "grf.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <algorithm>
#include <string>
#include <vector>

namespace {

int g_failed = 0;

void Check(bool ok, const char *what, const std::string &detail = "")
{
	printf("%s %s%s%s\n", ok ? "ok  " : "FAIL", what, detail.empty() ? "" : ": ", detail.c_str());
	if (!ok)
		++g_failed;
}

std::string Fmt(const char *f, ...) __attribute__((format(printf, 1, 2)));
std::string Fmt(const char *f, ...)
{
	char buf[512];
	va_list a;
	va_start(a, f);
	vsnprintf(buf, sizeof buf, f, a);
	va_end(a);
	return buf;
}

// C3dGrannyModelRes's per-mesh state (GrannyMesh in Model.h).
struct Mesh {
	const bf_granny_mesh *mesh;
	int index;
	int *binding;
	bool deformer;
	std::vector<uint16_t> idx;
	std::vector<bf_granny_pnt332> rigid;
};

struct Model {
	bf_granny_file *file = nullptr;
	const bf_granny_info *info = nullptr;
	bf_granny_instance *instance = nullptr;
	bf_granny_world_pose *world = nullptr;
	std::vector<bf_granny_transform> local;
	std::vector<Mesh> meshes;
	int textures_ok = 0;
};

// C3dGrannyModelRes::Load, CreateTexture, CreateRGBATexture, CreateModel, CreateMesh.
bool Load(Model &m, std::vector<uint8_t> &bytes)
{
	m.file = bf_granny_open(bytes.data(), bytes.size(), nullptr, 0);
	if (!m.file)
		return false;
	m.info = bf_granny_get_info(m.file);
	for (int t = 0; t < m.info->texture_count; ++t) {
		const bf_granny_texture *tex = bf_granny_get_texture(m.file, t);
		if (tex->texture_type != 0 || tex->image_count != 1)
			continue;
		std::vector<uint8_t> px((size_t)tex->width * tex->height * 4, 0);
		int alpha = 0;
		bf_granny_texture_pixels(m.file, t, px.data(), &alpha);
		// a picture, not a blank: more than a handful of distinct colours
		std::vector<uint32_t> seen;
		for (size_t i = 0; i + 3 < px.size() && seen.size() < 64; i += 4 * 7) {
			uint32_t c = px[i] | px[i + 1] << 8 | px[i + 2] << 16;
			bool found = false;
			for (uint32_t s : seen)
				found = found || s == c;
			if (!found)
				seen.push_back(c);
		}
		if (seen.size() >= 16)
			++m.textures_ok;
	}
	const bf_granny_model *model = bf_granny_get_model(m.file, 0);
	m.instance = bf_granny_new_instance(m.file, 0);
	const bf_granny_skeleton *sk = bf_granny_instance_skeleton(m.instance);
	if (!model || !sk)
		return false;
	m.world = bf_granny_new_world_pose(sk->bone_count);
	for (int i = 0; i < model->mesh_count; ++i) {
		Mesh x;
		x.index = model->meshes[i];
		x.mesh = bf_granny_get_mesh(m.file, x.index);
		x.binding = bf_granny_bind_mesh(x.mesh, sk);
		x.idx.resize((size_t)x.mesh->index_count);
		if (!x.idx.empty())
			bf_granny_copy_indices16(x.mesh, x.idx.data());
		x.deformer = !x.mesh->rigid;
		if (x.mesh->rigid) {
			x.rigid.resize((size_t)x.mesh->vertex_count);
			bf_granny_copy_vertices(x.mesh, x.rigid.data());
		}
		m.meshes.push_back(x);
	}
	m.local.resize((size_t)sk->bone_count);
	return true;
}

// C3dGrannyModelRes::UpdateAni.
void UpdateAni(Model &m, bf_granny_instance *inst, float clock)
{
	const bf_granny_skeleton *sk = bf_granny_instance_skeleton(inst);
	bf_granny_set_clock(inst, clock);
	bf_granny_sample_instance(inst, m.local.data(), (int)m.local.size());
	bf_granny_build_world_pose(sk, m.local.data(), (int)m.local.size(), m.world);
}

// C3dGrannyModelRes::RenderModel for every mesh: the posed positions, as the
// client gets them (skinned meshes deformed, rigid ones times their bone's
// composite).
std::vector<float> Render(Model &m)
{
	std::vector<float> out;
	float *composite = bf_granny_world_pose_composite(m.world, 0);
	for (Mesh &x : m.meshes) {
		int n = x.mesh->vertex_count;
		// the client's buffer: GetMeshIndexCount (triangles) * 3 vertices
		std::vector<bf_granny_pnt332> buf((size_t)(x.mesh->index_count / 3) * 3);
		if (x.mesh->rigid) {
			float *t = bf_granny_world_pose_composite(m.world, x.binding[0]);
			for (auto &v : x.rigid)
				for (int j = 0; j < 3; ++j)
					out.push_back(v.position[0] * t[j] + v.position[1] * t[4 + j] + v.position[2] * t[8 + j] + t[12 + j]);
		} else if (x.deformer && (int)buf.size() >= n) {
			bf_granny_deform(m.file, x.index, x.binding, composite, buf.data());
			for (int v = 0; v < n; ++v)
				for (int j = 0; j < 3; ++j)
					out.push_back(buf[(size_t)v].position[j]);
		}
	}
	return out;
}

float MaxDiff(const std::vector<float> &a, const std::vector<float> &b)
{
	float d = a.size() == b.size() ? 0 : 1e9f;
	for (size_t i = 0; i < a.size() && i < b.size(); ++i)
		d = fmaxf(d, fabsf(a[i] - b[i]));
	return d;
}

bool Finite(const std::vector<float> &a)
{
	for (float f : a)
		if (!isfinite(f))
			return false;
	return !a.empty();
}

void Free(Model &m)
{
	for (Mesh &x : m.meshes)
		bf_granny_free_mesh_binding(x.binding);
	bf_granny_free_instance(m.instance);
	bf_granny_free_world_pose(m.world);
	bf_granny_close(m.file);
}

bool Read(const std::vector<Grf *> &archives, const std::string &name, std::vector<uint8_t> &out)
{
	for (Grf *g : archives)
		if (g->Read(name, out))
			return true;
	return false;
}

// An animation file's one animation, started on inst, looping for ever.
bf_granny_control *PlayLooping(bf_granny_instance *inst, const bf_granny_file *anim)
{
	bf_granny_control *c = bf_granny_play(inst, 0.0f, anim, 0);
	bf_granny_set_loop_count(c, 0);
	return c;
}

}  // namespace

// Returns the number of failed checks; archives may be empty (the checks
// without game data only).
int RuntimeChecks(const std::vector<Grf *> &archives)
{
	printf("\n-- the client's runtime\n");
	// ---- without data
	bf_granny_free_instance(nullptr);
	bf_granny_free_control(nullptr);
	bf_granny_free_world_pose(nullptr);
	bf_granny_free_mesh_binding(nullptr);
	Check(bf_granny_time_left(nullptr) == 0 && bf_granny_new_instance(nullptr, 0) == nullptr
			&& bf_granny_play(nullptr, 0, nullptr, 0) == nullptr && bf_granny_sample_instance(nullptr, nullptr, 0) == 0
			&& bf_granny_bind_mesh(nullptr, nullptr) == nullptr,
		"NULL handles are ignored (the actor's destructor frees what it may never have made)");
	bf_granny_world_pose *p = bf_granny_new_world_pose(2);
	float *c0 = bf_granny_world_pose_composite(p, 0), *c1 = bf_granny_world_pose_composite(p, 1);
	Check(c0 && c1 == c0 + 16 && c0[0] == 1 && c0[5] == 1 && c0[15] == 1 && c0[1] == 0
			&& bf_granny_world_pose_composite(p, 2) == nullptr && bf_granny_world_pose_composite(p, -1) == nullptr,
		"a new world pose is identity; bones outside it give NULL");
	bf_granny_free_world_pose(p);
	if (archives.empty()) {
		printf("skipped  the runtime's file checks: no archive\n");
		return g_failed;
	}
	const std::string base = "data\\model\\3dmob\\";
	const std::string bone = "data\\model\\3dmob_bone\\";

	// every file reads; every mesh fits the client's buffers (16-bit
	// indices, GetMeshIndexCount * 3 vertices for the deformed copy)
	std::vector<std::string> names;
	for (Grf *g : archives)
		for (const std::string &n : g->Names(".gr2"))
			if (std::find(names.begin(), names.end(), n) == names.end())
				names.push_back(n);
	int read = 0, fits = 0, meshes = 0;
	std::string worst;
	for (const std::string &n : names) {
		std::vector<uint8_t> bytes;
		if (!Read(archives, n, bytes))
			continue;
		Model m;
		if (!Load(m, bytes) && m.file == nullptr)
			continue;
		++read;
		for (Mesh &x : m.meshes) {
			++meshes;
			int nv = x.mesh->vertex_count, ni = x.mesh->index_count;
			if (nv <= (ni / 3) * 3 && nv <= 65536)
				++fits;
			else
				worst = Fmt("%s %s: %d vertices, %d indices", n.c_str(), x.mesh->name, nv, ni);
		}
		Free(m);
	}
	Check(read == (int)names.size() && read > 0, "every .gr2 loads as the client loads it",
		Fmt("%d of %d", read, (int)names.size()));
	Check(fits == meshes, "every model mesh fits the client's buffers",
		Fmt("%d of %d%s%s", fits, meshes, worst.empty() ? "" : "; ", worst.c_str()));

	// the Emperium: stand animation from its own file (C3dGrannyBoneRes::
	// GetAnimation, nType 0: "model\\3dmob\\" + the job name, loaded a second time)
	{
		std::vector<uint8_t> bytes, again;
		Model m;
		bool ok = Read(archives, base + "empelium90_0.gr2", bytes) && Load(m, bytes);
		Check(ok, "Emperium loads as C3dGrannyModelRes::Load does");
		if (ok) {
			Check(m.info->texture_count == 1 && m.textures_ok == 1, "Emperium texture decodes to a picture",
				Fmt("%d of %d", m.textures_ok, m.info->texture_count));
			Check(m.meshes.size() == 1 && m.meshes[0].deformer, "Emperium: one skinned mesh");
			UpdateAni(m, m.instance, 0);
			std::vector<float> rest = Render(m);
			Check(Finite(rest), "rest pose (no control) renders", Fmt("%d coordinates", (int)rest.size()));

			Read(archives, base + "empelium90_0.gr2", again);
			bf_granny_file *af = bf_granny_open(again.data(), again.size(), nullptr, 0);
			const bf_granny_info *ai = bf_granny_get_info(af);
			Check(ai && ai->animation_count == 1, "the stand animation file has one animation");
			bf_granny_instance *inst = bf_granny_new_instance(m.file, 0);   // the actor's own instance
			bf_granny_control *ctl = PlayLooping(inst, af);
			Check(ctl != nullptr, "bf_granny_play binds the stand animation");
			Check(bf_granny_time_left(ctl) > 1e20f, "loop count 0 never ends");
			UpdateAni(m, inst, 1.0f);
			std::vector<float> a = Render(m);
			UpdateAni(m, inst, 6.0f);
			std::vector<float> b = Render(m);
			Check(Finite(a) && Finite(b) && MaxDiff(a, b) > 0.5f, "the Emperium turns: two times differ",
				Fmt("max %.2f units", MaxDiff(a, b)));
			float dur = bf_granny_get_animation(af, 0)->duration;
			UpdateAni(m, inst, 1.0f + dur);
			std::vector<float> c = Render(m);
			Check(MaxDiff(a, c) < 0.01f, "looping: t and t + duration give the same pose", Fmt("%.4f", MaxDiff(a, c)));

			bf_granny_set_active(ctl, 0);
			UpdateAni(m, inst, 1.0f);
			std::vector<float> d = Render(m);
			Check(MaxDiff(rest, d) < 1e-4f, "an inactive control leaves the rest pose", Fmt("%.5f", MaxDiff(rest, d)));
			bf_granny_set_active(ctl, 1);

			// loop count 1 (an attack): ends after one pass and holds
			bf_granny_control *once = bf_granny_play(inst, 0.0f, af, 0);
			bf_granny_set_loop_count(once, 1);
			UpdateAni(m, inst, dur * 0.5f);
			Check(fabsf(bf_granny_time_left(once) - dur * 0.5f) < 1e-3f, "loop count 1: time left halfway",
				Fmt("%.3f of %.3f", bf_granny_time_left(once), dur));
			UpdateAni(m, inst, dur + 2.0f);
			std::vector<float> e = Render(m);
			UpdateAni(m, inst, dur + 3.0f);
			std::vector<float> f = Render(m);
			Check(bf_granny_time_left(once) <= 0 && MaxDiff(e, f) < 1e-5f,
				"loop count 1: time left <= 0 after the pass, the end pose held");
			bf_granny_free_control(once);
			// an instance freed before its control: the control stays valid
			bf_granny_free_instance(inst);
			Check(bf_granny_time_left(ctl) > 1e20f, "a control outlives its instance until freed");
			bf_granny_free_control(ctl);
			bf_granny_close(af);
			Free(m);
		}
	}

	// the guild flag: two meshes (banner and emblem plane), two textures;
	// its own animation as the bone file WinMain preloads (type 5)
	{
		std::vector<uint8_t> bytes, again;
		Model m;
		bool ok = Read(archives, base + "guildflag90_1.gr2", bytes) && Load(m, bytes);
		Check(ok, "guild flag loads");
		if (ok) {
			Check(m.meshes.size() == 2, "guild flag: two meshes", Fmt("%d", (int)m.meshes.size()));
			const bf_granny_mesh *banner = m.meshes[0].mesh;
			const bf_granny_tri_group *g = banner->groups;
			const bf_granny_texture *tex = nullptr;
			if (g && banner->material_count > g->material) {
				const bf_granny_material *mat = bf_granny_get_material(m.file, banner->materials[g->material]);
				tex = mat ? bf_granny_get_texture(m.file, mat->diffuse_texture) : nullptr;
			}
			Check(tex != nullptr, "the banner's first group has a diffuse texture", tex ? tex->file_name : "none");
			UpdateAni(m, m.instance, 0);
			std::vector<float> rest = Render(m);
			Check(Finite(rest), "flag rest pose renders");
			Read(archives, base + "guildflag90_1.gr2", again);
			bf_granny_file *af = bf_granny_open(again.data(), again.size(), nullptr, 0);
			bf_granny_instance *inst = bf_granny_new_instance(m.file, 0);
			bf_granny_control *ctl = PlayLooping(inst, af);
			UpdateAni(m, inst, 0.5f);
			std::vector<float> a = Render(m);
			UpdateAni(m, inst, 2.5f);
			std::vector<float> b = Render(m);
			Check(MaxDiff(a, b) > 0.5f, "the flag's own animation sways it", Fmt("max %.2f units", MaxDiff(a, b)));
			bf_granny_free_control(ctl);
			bf_granny_free_instance(inst);
			bf_granny_close(af);
			Free(m);
		}
	}

	// a guardian with a 3dmob_bone animation from another file (7_move)
	{
		std::vector<uint8_t> bytes, abytes;
		Model m;
		bool ok = Read(archives, base + "kguardian90_7.gr2", bytes) && Load(m, bytes);
		Check(ok, "knight guardian loads");
		if (ok) {
			Check(m.textures_ok == m.info->texture_count, "knight guardian textures decode",
				Fmt("%d of %d", m.textures_ok, m.info->texture_count));
			ok = Read(archives, bone + "7_move.gr2", abytes);
			Check(ok, "7_move.gr2 in the archive");
			if (ok) {
				bf_granny_file *af = bf_granny_open(abytes.data(), abytes.size(), nullptr, 0);
				bf_granny_instance *inst = bf_granny_new_instance(m.file, 0);
				bf_granny_control *ctl = PlayLooping(inst, af);
				UpdateAni(m, inst, 0.2f);
				std::vector<float> a = Render(m);
				UpdateAni(m, inst, 0.9f);
				std::vector<float> b = Render(m);
				Check(Finite(a) && MaxDiff(a, b) > 0.5f, "the knight walks (7_move): two times differ",
					Fmt("max %.2f units", MaxDiff(a, b)));
				bf_granny_free_control(ctl);
				bf_granny_free_instance(inst);
				bf_granny_close(af);
			}
			Free(m);
		}
	}
	return g_failed;
}
