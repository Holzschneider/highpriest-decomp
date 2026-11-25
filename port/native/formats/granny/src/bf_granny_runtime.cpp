// The client's runtime (M11.10): model instances, controls, poses, mesh
// bindings and the copies into the client's buffers, as the client used
// granny2.dll (bf_granny.h, "The client's runtime"). Moved here from the
// port's Granny adapter (port/compat/granny, M12.3a), which M11.10 deleted;
// the rules are unchanged.

#include "bf_granny.h"

#include "bf_trace.h"

#include <math.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>

#include <vector>

namespace {

const float kForever = 1.0e30f;

}  // namespace

struct bf_granny_instance {
	const bf_granny_file *file;
	int model;
	const bf_granny_skeleton *skeleton;
	float clock;
	std::vector<bf_granny_control *> controls;   // in the order they were started
	bf_granny_binding *rest;                     // the rest pose, made when first needed
};

struct bf_granny_control {
	bf_granny_instance *instance;                // NULL once the instance is freed
	bf_granny_binding *binding;
	float start, duration, clock;
	int loops;                                   // 0: for ever
	bool active;
};

struct bf_granny_world_pose {
	int bone_count;
	std::vector<float> world, composite;
};

// ---- instances

extern "C" bf_granny_instance *bf_granny_new_instance(const bf_granny_file *file, int model)
{
	const bf_granny_model *m = bf_granny_get_model(file, model);
	const bf_granny_skeleton *sk = m ? bf_granny_get_skeleton(file, m->skeleton) : nullptr;
	if (!sk)
		return nullptr;
	bf_granny_instance *i = new bf_granny_instance;
	i->file = file;
	i->model = model;
	i->skeleton = sk;
	i->clock = 0;
	i->rest = nullptr;
	return i;
}

extern "C" void bf_granny_free_instance(bf_granny_instance *instance)
{
	if (!instance)
		return;
	// Controls still on the instance stay valid until bf_granny_free_control
	// (the client frees its controls itself); they no longer move it.
	for (bf_granny_control *c : instance->controls)
		c->instance = nullptr;
	bf_granny_free_binding(instance->rest);
	delete instance;
}

extern "C" const bf_granny_skeleton *bf_granny_instance_skeleton(const bf_granny_instance *instance)
{
	return instance ? instance->skeleton : nullptr;
}

extern "C" void bf_granny_set_clock(bf_granny_instance *instance, float clock)
{
	if (!instance)
		return;
	instance->clock = clock;
	for (bf_granny_control *c : instance->controls)
		c->clock = clock;
}

// ---- controls

extern "C" bf_granny_control *bf_granny_play(bf_granny_instance *instance, float start, const bf_granny_file *anim_file,
	int animation)
{
	const bf_granny_animation *a = bf_granny_get_animation(anim_file, animation);
	if (!instance || !a)
		return nullptr;
	const bf_granny_model *m = bf_granny_get_model(instance->file, instance->model);
	const char *model_name = m && m->name ? m->name : "?";
	bf_granny_binding *b = bf_granny_bind_animation(instance->file, instance->model, anim_file, animation);
	if (!b) {
		bf_count("granny.animation_unbound", 1);
		BF_TRACE(BFT_RES, "granny: animation %s does not bind to model %s", a->name ? a->name : "?", model_name);
		return nullptr;
	}
	int bones = 0, matched = 0;
	float duration = 0;
	bf_granny_binding_info(b, &bones, &matched, &duration);
	if (matched == 0) {
		bf_count("granny.animation_no_tracks", 1);
		BF_TRACE(BFT_RES, "granny: animation %s moves no bone of model %s", a->name ? a->name : "?", model_name);
	}
	bf_granny_control *c = new bf_granny_control;
	c->instance = instance;
	c->binding = b;
	c->start = start;
	c->duration = duration;
	c->clock = instance->clock;
	c->loops = 1;               // Granny's default
	c->active = true;
	instance->controls.push_back(c);
	return c;
}

extern "C" void bf_granny_set_loop_count(bf_granny_control *control, int loops)
{
	if (control)
		control->loops = loops < 0 ? 0 : loops;
}

extern "C" void bf_granny_set_active(bf_granny_control *control, int active)
{
	if (control)
		control->active = active != 0;
}

extern "C" float bf_granny_time_left(const bf_granny_control *control)
{
	if (!control)
		return 0;
	if (control->loops == 0)
		return kForever;
	return control->loops * control->duration - (control->clock - control->start);
}

extern "C" void bf_granny_free_control(bf_granny_control *control)
{
	if (!control)
		return;
	if (control->instance) {
		std::vector<bf_granny_control *> &list = control->instance->controls;
		for (size_t i = 0; i < list.size(); ++i)
			if (list[i] == control) {
				list.erase(list.begin() + (long)i);
				break;
			}
	}
	bf_granny_free_binding(control->binding);
	delete control;
}

// The pose of the most recently started active control at the instance's
// clock, looped by its loop count and held at its end once the loops are
// over; the rest pose without one.
extern "C" int bf_granny_sample_instance(bf_granny_instance *instance, bf_granny_transform *local, int count)
{
	if (!instance || !local)
		return 0;
	bf_granny_control *use = nullptr;
	for (size_t i = instance->controls.size(); i-- > 0;)
		if (instance->controls[i]->active) {
			use = instance->controls[i];
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
		if (!instance->rest)
			instance->rest = bf_granny_bind_animation(instance->file, instance->model, nullptr, 0);
		b = instance->rest;
	}
	int bones = 0;
	if (!b || !bf_granny_binding_info(b, &bones, nullptr, nullptr))
		return 0;
	if (count >= bones) {
		bf_granny_sample_local(b, t, local);
	} else if (count > 0) {
		std::vector<bf_granny_transform> all((size_t)bones);
		bf_granny_sample_local(b, t, all.data());
		memcpy(local, all.data(), sizeof(bf_granny_transform) * (size_t)count);
	}
	return bones;
}

// ---- world poses

extern "C" bf_granny_world_pose *bf_granny_new_world_pose(int bone_count)
{
	bf_granny_world_pose *p = new bf_granny_world_pose;
	p->bone_count = bone_count > 0 ? bone_count : 0;
	p->world.assign((size_t)p->bone_count * 16, 0.0f);
	p->composite.assign((size_t)p->bone_count * 16, 0.0f);
	for (int b = 0; b < p->bone_count; ++b)
		for (int k = 0; k < 4; ++k)
			p->world[(size_t)b * 16 + (size_t)k * 5] = p->composite[(size_t)b * 16 + (size_t)k * 5] = 1.0f;
	return p;
}

extern "C" void bf_granny_free_world_pose(bf_granny_world_pose *pose)
{
	delete pose;
}

extern "C" int bf_granny_build_world_pose(const bf_granny_skeleton *skeleton, const bf_granny_transform *local, int count,
	bf_granny_world_pose *pose)
{
	if (!skeleton || !local || !pose || count < skeleton->bone_count)
		return 0;
	if (pose->bone_count < skeleton->bone_count) {
		pose->bone_count = skeleton->bone_count;
		pose->world.resize((size_t)skeleton->bone_count * 16);
		pose->composite.resize((size_t)skeleton->bone_count * 16);
	}
	return bf_granny_build_world(skeleton, local, nullptr, pose->world.data(), pose->composite.data());
}

extern "C" float *bf_granny_world_pose_composite(bf_granny_world_pose *pose, int bone)
{
	if (!pose || bone < 0 || bone >= pose->bone_count)
		return nullptr;
	return pose->composite.data() + (size_t)bone * 16;
}

// ---- meshes

extern "C" int *bf_granny_bind_mesh(const bf_granny_mesh *mesh, const bf_granny_skeleton *skeleton)
{
	if (!mesh || !skeleton || mesh->bone_binding_count <= 0)
		return nullptr;
	int *bones = (int *)malloc(sizeof(int) * (size_t)mesh->bone_binding_count);
	if (!bones)
		return nullptr;
	for (int k = 0; k < mesh->bone_binding_count; ++k) {
		const char *name = mesh->bone_bindings[k].bone_name;
		int found = -1;
		for (int i = 0; name && i < skeleton->bone_count && found < 0; ++i)
			if (skeleton->bones[i].name && strcmp(skeleton->bones[i].name, name) == 0)
				found = i;
		for (int i = 0; name && i < skeleton->bone_count && found < 0; ++i)
			if (skeleton->bones[i].name && strcasecmp(skeleton->bones[i].name, name) == 0)
				found = i;
		if (found < 0) {
			bf_count("granny.bone_unbound", 1);
			BF_TRACE(BFT_RES, "granny: mesh %s: bone %s not in skeleton %s", mesh->name ? mesh->name : "?",
				name ? name : "?", skeleton->name ? skeleton->name : "?");
			found = 0;
		}
		bones[k] = found;
	}
	return bones;
}

extern "C" void bf_granny_free_mesh_binding(int *bones)
{
	free(bones);
}

extern "C" int bf_granny_copy_vertices(const bf_granny_mesh *mesh, bf_granny_pnt332 *out)
{
	if (!mesh || !out)
		return 0;
	for (int v = 0; v < mesh->vertex_count; ++v) {
		memcpy(out[v].position, mesh->vertices[v].position, sizeof out[v].position);
		memcpy(out[v].normal, mesh->vertices[v].normal, sizeof out[v].normal);
		memcpy(out[v].uv, mesh->vertices[v].uv, sizeof out[v].uv);
	}
	return mesh->vertex_count;
}

extern "C" int bf_granny_copy_indices16(const bf_granny_mesh *mesh, uint16_t *out)
{
	if (!mesh || !out)
		return 0;
	for (int i = 0; i < mesh->index_count; ++i) {
		uint32_t v = mesh->indices[i];
		if (v > 0xffff)
			bf_count("granny.index_truncated", 1);
		out[i] = (uint16_t)v;
	}
	return mesh->index_count;
}
