// Test of the Granny adapter (M12.3a): the client's call sequences through
// the Granny* names, as src/resource/Model.cpp and src/GameActor3d.cpp make
// them (the patterns, not the client's code). One line per check; exit 1 on
// a failure. Without data.grf in BF_CLIENT_DIR (default ~/Games/<name>) the
// file checks print "skipped".

#include "../../../src/granny/granny.h"

#include "grf.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <string>
#include <vector>

static int g_failed = 0, g_passed = 0;

static void check(bool ok, const char *what, const std::string &detail = "")
{
	printf("%s  %s%s%s\n", ok ? "ok  " : "FAIL", what, detail.empty() ? "" : ": ", detail.c_str());
	if (ok)
		++g_passed;
	else
		++g_failed;
}

static std::string fmt(const char *f, ...)
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
	granny_mesh *mesh;
	granny_mesh_binding *binding;
	granny_mesh_deformer *deformer;
	std::vector<unsigned short> idx;
	std::vector<granny_pnt332_vertex> rigid;
};

struct Model {
	granny_file *file = nullptr;
	granny_file_info *info = nullptr;
	granny_model *model = nullptr;
	granny_model_instance *instance = nullptr;
	granny_world_pose *world = nullptr;
	granny_local_pose *local = nullptr;
	std::vector<Mesh> meshes;
	int textures_ok = 0;
};

// C3dGrannyModelRes::Load, CreateTexture, CreateRGBATexture, CreateModel, CreateMesh.
static bool Load(Model &m, std::vector<uint8_t> &bytes, const char *name)
{
	m.file = GrannyReadEntireFileFromMemory((granny_int32x)bytes.size(), bytes.data());
	if (!m.file)
		return false;
	m.info = GrannyGetFileInfo(m.file);
	if (!m.info)
		return false;
	for (int t = 0; t < m.info->TextureCount; ++t) {
		granny_texture *tex = m.info->Textures[t];
		if (tex->TextureType != 0 || tex->ImageCount != 1)
			continue;
		GrannyTextureHasAlpha(tex);
		std::vector<uint8_t> px((size_t)tex->Width * tex->Height * 4, 0);
		GrannyCopyTextureImage(tex, 0, 0, GrannyRGBA8888PixelFormat, tex->Width, tex->Height, tex->Width * 4, px.data());
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
	m.model = m.info->Models[0];
	m.instance = GrannyInstantiateModel(m.model);
	m.world = GrannyNewWorldPose(m.model->Skeleton->BoneCount);
	for (int i = 0; i < m.model->MeshBindingCount; ++i) {
		Mesh x;
		x.mesh = m.model->MeshBindings[i].Mesh;
		granny_skeleton *sk = GrannyGetSourceSkeleton(m.instance);
		x.binding = GrannyNewMeshBinding(x.mesh, sk, sk);
		x.idx.resize((size_t)GrannyGetMeshIndexCount(x.mesh));
		if (!x.idx.empty())
			GrannyCopyMeshIndices(x.mesh, 2, x.idx.data());
		x.deformer = nullptr;
		if (GrannyMeshIsRigid(x.mesh)) {
			x.rigid.resize((size_t)GrannyGetMeshVertexCount(x.mesh));
			GrannyCopyMeshVertices(x.mesh, GrannyPNT332VertexType, x.rigid.data());
		} else {
			x.deformer = GrannyNewMeshDeformer(GrannyGetMeshVertexType(x.mesh), GrannyPNT332VertexType, 2);
		}
		m.meshes.push_back(x);
	}
	m.local = GrannyNewLocalPose(m.model->Skeleton->BoneCount);
	for (int s = 1; s <= 5; ++s)
		GrannyFreeFileSection(m.file, s);
	return true;
}

// C3dGrannyModelRes::UpdateAni.
static void UpdateAni(Model &m, granny_model_instance *inst, float clock)
{
	granny_skeleton *sk = GrannyGetSourceSkeleton(inst);
	GrannySetModelClock(inst, clock);
	GrannySampleModelAnimations(inst, 0, sk->BoneCount, m.local);
	GrannyBuildWorldPose(sk, 0, sk->BoneCount, m.local, 0, m.world);
	GrannyFreeCompletedModelControls(inst);
}

// C3dGrannyModelRes::RenderModel for every mesh: the posed positions, as the
// client gets them (skinned meshes deformed, rigid ones times their bone's
// composite).
static std::vector<float> Render(Model &m)
{
	std::vector<float> out;
	granny_matrix_4x4 *composite = GrannyGetWorldPoseComposite4x4Array(m.world);
	for (Mesh &x : m.meshes) {
		int *bones = GrannyGetMeshBindingToBoneIndices(x.binding);
		int n = GrannyGetMeshVertexCount(x.mesh);
		// the client's buffer: GetMeshIndexCount (triangles) * 3 vertices
		std::vector<granny_pnt332_vertex> buf((size_t)(GrannyGetMeshIndexCount(x.mesh) / 3) * 3);
		if (GrannyMeshIsRigid(x.mesh)) {
			float *t = (float *)GrannyGetWorldPoseComposite4x4(m.world, bones[0]);
			for (auto &v : x.rigid)
				for (int j = 0; j < 3; ++j)
					out.push_back(v.Position[0] * t[j] + v.Position[1] * t[4 + j] + v.Position[2] * t[8 + j] + t[12 + j]);
		} else if (x.deformer && (int)buf.size() >= n) {
			GrannyDeformVertices(x.deformer, bones, (float *)composite, n, GrannyGetMeshVertices(x.mesh), buf.data());
			for (int v = 0; v < n; ++v)
				for (int j = 0; j < 3; ++j)
					out.push_back(buf[(size_t)v].Position[j]);
		}
	}
	return out;
}

static float MaxDiff(const std::vector<float> &a, const std::vector<float> &b)
{
	float d = a.size() == b.size() ? 0 : 1e9f;
	for (size_t i = 0; i < a.size() && i < b.size(); ++i)
		d = fmaxf(d, fabsf(a[i] - b[i]));
	return d;
}

static bool Finite(const std::vector<float> &a)
{
	for (float f : a)
		if (!isfinite(f))
			return false;
	return !a.empty();
}

static void Free(Model &m)
{
	for (Mesh &x : m.meshes) {
		GrannyFreeMeshBinding(x.binding);
		GrannyFreeMeshDeformer(x.deformer);
	}
	GrannyFreeModelInstance(m.instance);
	GrannyFreeWorldPose(m.world);
	GrannyFreeLocalPose(m.local);
	GrannyFreeFile(m.file);
}

int main(int argc, char **argv)
{
	// ---- without data
	check(GrannyVersionsMatch, "GrannyVersionsMatch (2.1.0.5)");
	granny_system_clock c0 = GrannyGetSystemSeconds();
	usleep(50000);
	granny_system_clock c1 = GrannyGetSystemSeconds();
	float dt = GrannyGetSecondsElapsed(&c0, &c1);
	check(dt > 0.04f && dt < 0.5f, "GrannyGetSecondsElapsed over a 50 ms sleep", fmt("%.4f s", dt));
	char junk[64] = "not a granny file";
	check(GrannyReadEntireFileFromMemory(sizeof junk, junk) == nullptr, "a broken file is refused (NULL)");
	GrannyFreeModelInstance(nullptr);
	GrannyFreeControl(nullptr);
	GrannyFreeFile(nullptr);
	check(GrannyGetControlDurationLeft(nullptr) == 0, "NULL handles are ignored (the actor's destructor frees what it may never have made)");

	// ---- the game's files
	const char *dir = getenv("BF_CLIENT_DIR");
	std::string game = dir ? dir : std::string(getenv("HOME")) + "/Games/" BF_APP_NAME;
	Grf grf;
	if (!grf.Open(game + "/data.grf")) {
		printf("skipped  no data.grf in %s: the file checks\n", game.c_str());
		printf("%d passed, %d failed\n", g_passed, g_failed);
		return g_failed ? 1 : 0;
	}
	const std::string base = "data\\model\\3dmob\\";
	const std::string bone = "data\\model\\3dmob_bone\\";

	// every file reads; every mesh fits the client's buffers (16-bit
	// indices, GetMeshIndexCount * 3 vertices for the deformed copy)
	std::vector<std::string> names = grf.Names(".gr2");
	int read = 0, fits = 0, meshes = 0;
	std::string worst;
	for (const std::string &n : names) {
		std::vector<uint8_t> bytes;
		if (!grf.Read(n, bytes))
			continue;
		Model m;
		if (!Load(m, bytes, n.c_str()) && m.file == nullptr)
			continue;
		++read;
		for (Mesh &x : m.meshes) {
			++meshes;
			int nv = GrannyGetMeshVertexCount(x.mesh), ni = GrannyGetMeshIndexCount(x.mesh);
			if (nv <= (ni / 3) * 3 && nv <= 65536)
				++fits;
			else
				worst = fmt("%s %s: %d vertices, %d indices", n.c_str(), x.mesh->Name, nv, ni);
		}
		Free(m);
	}
	check(read == (int)names.size() && read > 0, "every .gr2 reads through GrannyReadEntireFileFromMemory", fmt("%d of %d", read, (int)names.size()));
	check(fits == meshes, "every model mesh fits the client's buffers", fmt("%d of %d%s%s", fits, meshes, worst.empty() ? "" : "; ", worst.c_str()));

	// the Emperium: stand animation from its own file (C3dGrannyBoneRes::
	// GetAnimation, nType 0: "model\\3dmob\\" + the job name, loaded a second time)
	{
		std::vector<uint8_t> bytes, again;
		Model m;
		bool ok = grf.Read(base + "empelium90_0.gr2", bytes) && Load(m, bytes, "empelium90_0");
		check(ok, "Emperium loads as C3dGrannyModelRes::Load does");
		if (ok) {
			check(m.info->TextureCount == 1 && m.textures_ok == 1, "Emperium texture decodes to a picture",
				fmt("%d of %d", m.textures_ok, m.info->TextureCount));
			check(m.meshes.size() == 1 && m.meshes[0].deformer != nullptr, "Emperium: one skinned mesh with a deformer");
			UpdateAni(m, m.instance, 0);
			std::vector<float> rest = Render(m);
			check(Finite(rest), "rest pose (no control) renders", fmt("%d coordinates", (int)rest.size()));

			grf.Read(base + "empelium90_0.gr2", again);
			granny_file *af = GrannyReadEntireFileFromMemory((granny_int32x)again.size(), again.data());
			granny_file_info *ai = GrannyGetFileInfo(af);
			check(ai && ai->AnimationCount == 1, "the stand animation file has one animation");
			granny_model_instance *inst = GrannyInstantiateModel(m.model);   // the actor's own instance
			granny_control *ctl = GrannyPlayControlledAnimation(0.0f, ai->Animations[0], inst);
			GrannySetControlLoopCount(ctl, 0);
			check(ctl != nullptr, "GrannyPlayControlledAnimation binds the stand animation");
			check(GrannyGetControlDurationLeft(ctl) > 1e20f, "loop count 0 never ends");
			UpdateAni(m, inst, 1.0f);
			std::vector<float> a = Render(m);
			UpdateAni(m, inst, 6.0f);
			std::vector<float> b = Render(m);
			check(Finite(a) && Finite(b) && MaxDiff(a, b) > 0.5f, "the Emperium turns: two times differ",
				fmt("max %.2f units", MaxDiff(a, b)));
			float dur = ai->Animations[0]->Duration;
			UpdateAni(m, inst, 1.0f + dur);
			std::vector<float> c = Render(m);
			check(MaxDiff(a, c) < 0.01f, "looping: t and t + duration give the same pose", fmt("%.4f", MaxDiff(a, c)));

			GrannySetControlActive(ctl, false);
			UpdateAni(m, inst, 1.0f);
			std::vector<float> d = Render(m);
			check(MaxDiff(rest, d) < 1e-4f, "an inactive control leaves the rest pose", fmt("%.5f", MaxDiff(rest, d)));
			GrannySetControlActive(ctl, true);

			// loop count 1 (an attack): ends after one pass and holds
			granny_control *once = GrannyPlayControlledAnimation(0.0f, ai->Animations[0], inst);
			GrannySetControlLoopCount(once, 1);
			UpdateAni(m, inst, dur * 0.5f);
			check(fabsf(GrannyGetControlDurationLeft(once) - dur * 0.5f) < 1e-3f, "loop count 1: duration left halfway",
				fmt("%.3f of %.3f", GrannyGetControlDurationLeft(once), dur));
			UpdateAni(m, inst, dur + 2.0f);
			check(GrannyGetControlDurationLeft(once) <= 0, "loop count 1: duration left <= 0 after the pass");
			GrannyFreeControl(once);
			GrannyFreeControl(ctl);
			GrannyFreeModelInstance(inst);
			GrannyFreeFile(af);
			Free(m);
		}
	}

	// the guild flag: two meshes (banner and emblem plane), two textures;
	// its own animation as the bone file WinMain preloads (type 5)
	{
		std::vector<uint8_t> bytes, again;
		Model m;
		bool ok = grf.Read(base + "guildflag90_1.gr2", bytes) && Load(m, bytes, "guildflag90_1");
		check(ok, "guild flag loads");
		if (ok) {
			check(m.meshes.size() == 2, "guild flag: two meshes", fmt("%d", (int)m.meshes.size()));
			granny_tri_material_group *g = GrannyGetMeshTriangleGroups(m.meshes[0].mesh);
			granny_texture *tex = nullptr;
			if (g && m.meshes[0].mesh->MaterialBindingCount > g->MaterialIndex)
				tex = GrannyGetMaterialTextureByType(m.meshes[0].mesh->MaterialBindings[g->MaterialIndex].Material, 2);
			check(tex != nullptr, "the banner's first group has a diffuse texture", tex ? tex->FromFileName : "none");
			UpdateAni(m, m.instance, 0);
			std::vector<float> rest = Render(m);
			check(Finite(rest), "flag rest pose renders");
			grf.Read(base + "guildflag90_1.gr2", again);
			granny_file *af = GrannyReadEntireFileFromMemory((granny_int32x)again.size(), again.data());
			granny_model_instance *inst = GrannyInstantiateModel(m.model);
			granny_control *ctl = GrannyPlayControlledAnimation(0.0f, GrannyGetFileInfo(af)->Animations[0], inst);
			GrannySetControlLoopCount(ctl, 0);
			UpdateAni(m, inst, 0.5f);
			std::vector<float> a = Render(m);
			UpdateAni(m, inst, 2.5f);
			std::vector<float> b = Render(m);
			check(MaxDiff(a, b) > 0.5f, "the flag's own animation sways it", fmt("max %.2f units", MaxDiff(a, b)));
			GrannyFreeControl(ctl);
			GrannyFreeModelInstance(inst);
			GrannyFreeFile(af);
			Free(m);
		}
	}

	// a guardian with a 3dmob_bone animation from another file (7_move)
	{
		std::vector<uint8_t> bytes, abytes;
		Model m;
		bool ok = grf.Read(base + "kguardian90_7.gr2", bytes) && Load(m, bytes, "kguardian90_7");
		check(ok, "knight guardian loads");
		if (ok) {
			check(m.textures_ok == m.info->TextureCount, "knight guardian textures decode",
				fmt("%d of %d", m.textures_ok, m.info->TextureCount));
			ok = grf.Read(bone + "7_move.gr2", abytes);
			check(ok, "7_move.gr2 in the archive");
			if (ok) {
				granny_file *af = GrannyReadEntireFileFromMemory((granny_int32x)abytes.size(), abytes.data());
				granny_model_instance *inst = GrannyInstantiateModel(m.model);
				granny_control *ctl = GrannyPlayControlledAnimation(0.0f, GrannyGetFileInfo(af)->Animations[0], inst);
				GrannySetControlLoopCount(ctl, 0);
				UpdateAni(m, inst, 0.2f);
				std::vector<float> a = Render(m);
				UpdateAni(m, inst, 0.9f);
				std::vector<float> b = Render(m);
				check(Finite(a) && MaxDiff(a, b) > 0.5f, "the knight walks (7_move): two times differ",
					fmt("max %.2f units", MaxDiff(a, b)));
				GrannyFreeControl(ctl);
				GrannyFreeModelInstance(inst);
				GrannyFreeFile(af);
			}
			Free(m);
		}
	}

	printf("%d passed, %d failed\n", g_passed, g_failed);
	return g_failed ? 1 : 0;
}
