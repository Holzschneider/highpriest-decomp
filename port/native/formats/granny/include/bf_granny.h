/*
 * bf_granny: a reader for the game's Granny 2 models (.gr2, format 6, as
 * written by the Granny 2.1.0.5 exporter): guild flags, the Emperium, the
 * guardians, the treasure box and their animation files.
 *
 * It replaces the parts of granny2.dll the client loads files with: the
 * container (sections, Oodle0 decompression, pointer fix-ups, the type
 * tree), the object tree below granny_file_info, and the runtime the client
 * calls: texture decoding, curve evaluation, posing and CPU skinning. What
 * it hands out is plain data: meshes with their vertices in the rest pose,
 * skeletons, materials, textures (stored and decoded), animation curves,
 * and matrices. The reader's posing functions leave clocks, looping and
 * blending to the caller; "The client's runtime" at the end (M11.10) adds
 * model instances and controls as the client uses them (no blending).
 *
 * Everything returned points into the file object and stays valid until
 * bf_granny_close(). Strings are the bytes stored in the file (ASCII in the
 * RO files; Windows paths from the artists' machines), NUL-terminated.
 *
 * Threads: a file object is used by one thread at a time; separate file
 * objects may be used on separate threads. Nothing here touches global state
 * other than the trace.
 *
 * The format as far as it is established, with evidence, is in FORMAT.md
 * next to this module.
 */
#ifndef BF_GRANNY_H
#define BF_GRANNY_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct bf_granny_file bf_granny_file;

/*
 * Reads a .gr2 file from memory. The bytes are not kept: the caller may free
 * them on return. Returns NULL on failure, with a trace line (BFT_ERR) and,
 * if error is not NULL, the reason in error (at most error_size bytes).
 * Counted in the trace: granny.files read, granny.file_refused.
 */
bf_granny_file *bf_granny_open(const void *data, size_t size, char *error, size_t error_size);
void bf_granny_close(bf_granny_file *file);

/* --- The container ------------------------------------------------------ */

enum {
	BF_GRANNY_NO_COMPRESSION = 0,
	BF_GRANNY_OODLE0 = 1,           /* the only compression the RO files use */
	BF_GRANNY_OODLE1 = 2            /* not supported: open fails */
};

typedef struct bf_granny_section {
	int compression;
	uint32_t file_offset;         /* where the stored bytes start in the file */
	uint32_t packed_size;         /* stored bytes */
	uint32_t size;                /* decompressed bytes */
	uint32_t alignment;
	uint32_t stop16, stop8;       /* Oodle0 block boundaries (decompressed offsets) */
	uint32_t fixup_count;         /* pointer fix-ups whose pointer lies in this section */
	uint32_t marshal_count;       /* byte-order marshalling records (unused on little-endian) */
	const uint8_t *data;          /* the decompressed bytes, size of them */
} bf_granny_section;

typedef struct bf_granny_info {
	uint32_t version;             /* 6 */
	uint32_t total_size;
	uint32_t crc;                 /* as stored; not checked */
	uint32_t type_tag;            /* 0x8000000f in every RO file */
	int section_count;
	int fixup_count;
	uint32_t root_type_section, root_type_offset;
	uint32_t root_object_section, root_object_offset;

	const char *from_file_name;   /* the .max scene the file was exported from */
	const char *art_tool;         /* "3D Studio MAX" */
	float units_per_meter;
	float origin[3], right[3], up[3], back[3];  /* the art tool's axes; up is +Z */
	const char *exporter;         /* "Granny Standard Exporter, SDK version 2.1.0.5" */
	int exporter_version[4];

	int texture_count, material_count, skeleton_count, mesh_count, model_count;
	int track_group_count, animation_count;
} bf_granny_info;

const bf_granny_info *bf_granny_get_info(const bf_granny_file *file);
const bf_granny_section *bf_granny_get_section(const bf_granny_file *file, int index);

/*
 * The members of the root object's type (granny_file_info as this file
 * describes it): name, member type name ("reference", "array-of-references",
 * ...), and offset in the 32-bit layout. Returns 0 past the last one.
 */
int bf_granny_root_member(const bf_granny_file *file, int index, const char **name, const char **type, uint32_t *offset);

/* --- Textures and materials -------------------------------------------- */

enum {
	BF_GRANNY_ENCODING_USER = 0,
	BF_GRANNY_ENCODING_RAW = 1,
	BF_GRANNY_ENCODING_S3TC = 2,
	BF_GRANNY_ENCODING_BINK = 3     /* every texture in the RO files; not decoded here */
};

typedef struct bf_granny_texture {
	const char *file_name;        /* FromFileName, e.g. "D:\\...\\empelium.tif" */
	int texture_type;             /* 0 = colour map */
	int width, height;
	int encoding;                 /* BF_GRANNY_ENCODING_* */
	int sub_format;
	int bytes_per_pixel;
	int shift_for_component[4], bits_for_component[4];
	int image_count, mip_count;   /* of image 0 */
	int stride;                   /* of image 0, MIP 0 (0 for Bink) */
	const uint8_t *pixels;        /* image 0, MIP 0, as stored (encoded for Bink) */
	size_t pixel_size;
} bf_granny_texture;

typedef struct bf_granny_material {
	const char *name;
	int texture;                  /* the material's own texture, or -1 */
	int diffuse_texture;          /* "Diffuse Color" map's texture, else texture; or -1 */
	int opacity_texture;          /* "Opacity" map's texture, or -1 */
	int map_count;
} bf_granny_material;

const bf_granny_texture *bf_granny_get_texture(const bf_granny_file *file, int index);
const bf_granny_material *bf_granny_get_material(const bf_granny_file *file, int index);

/* --- Skeletons ------------------------------------------------------------ */

typedef struct bf_granny_transform {
	uint32_t flags;               /* 1 position, 2 orientation, 4 scale/shear present */
	float position[3];
	float orientation[4];         /* quaternion x, y, z, w */
	float scale_shear[9];         /* row-major 3x3 */
} bf_granny_transform;

typedef struct bf_granny_bone {
	const char *name;
	int parent;                   /* -1 for a root */
	bf_granny_transform local;    /* relative to the parent, rest pose */
	float inverse_world[16];      /* inverse of the rest-pose world matrix, row-major, row vectors */
} bf_granny_bone;

typedef struct bf_granny_skeleton {
	const char *name;
	int bone_count;
	const bf_granny_bone *bones;  /* parents come before their children */
} bf_granny_skeleton;

const bf_granny_skeleton *bf_granny_get_skeleton(const bf_granny_file *file, int index);

/* --- Meshes and models ---------------------------------------------------- */

/* One vertex, converted from the file's layout (granny_pnt332_vertex plus skinning). */
typedef struct bf_granny_vertex {
	float position[3];            /* rest pose, model space */
	float normal[3];
	float uv[2];
	uint8_t bone_index[4];        /* into the mesh's bone bindings */
	uint8_t bone_weight[4];       /* 0..255; they sum to 255 */
} bf_granny_vertex;

typedef struct bf_granny_tri_group {
	int material;                 /* into the mesh's materials[] */
	int first_triangle;
	int triangle_count;
} bf_granny_tri_group;

typedef struct bf_granny_bone_binding {
	const char *bone_name;        /* matched to a skeleton bone by name */
	int bone;                     /* that bone in the model's skeleton, or -1 */
	float obb_min[3], obb_max[3];
} bf_granny_bone_binding;

typedef struct bf_granny_mesh {
	const char *name;
	const char *vertex_layout;    /* the stored layout, e.g. "Position:real32[3] BoneWeights:normal-uint8[4] ..." */
	int rigid;                    /* 1 if the layout has no bone weights (one bone moves it) */
	int vertex_count;
	const bf_granny_vertex *vertices;
	int index_count;              /* three per triangle */
	const uint32_t *indices;
	int group_count;
	const bf_granny_tri_group *groups;
	int material_count;
	const int *materials;         /* into the file's materials, -1 if unresolved */
	int bone_binding_count;
	const bf_granny_bone_binding *bone_bindings;
} bf_granny_mesh;

typedef struct bf_granny_model {
	const char *name;
	int skeleton;                 /* into the file's skeletons, or -1 */
	bf_granny_transform initial_placement;
	int mesh_count;
	const int *meshes;            /* into the file's meshes */
} bf_granny_model;

const bf_granny_mesh *bf_granny_get_mesh(const bf_granny_file *file, int index);
const bf_granny_model *bf_granny_get_model(const bf_granny_file *file, int index);

/* --- Animations ----------------------------------------------------------- */

/*
 * A curve as stored (Granny 2.1 B-spline): degree, knot times and control
 * values, dimension floats per control (3 position, 4 orientation
 * quaternion, 9 scale/shear). A curve with no knots is absent: the bone
 * keeps its rest value. See FORMAT.md for what is known of evaluation.
 */
typedef struct bf_granny_curve {
	int degree;
	int dimension;
	int knot_count;
	const float *knots;
	int control_count;            /* floats, not points */
	const float *controls;
} bf_granny_curve;

typedef struct bf_granny_transform_track {
	const char *name;             /* the bone it drives */
	bf_granny_curve position, orientation, scale_shear;
} bf_granny_transform_track;

typedef struct bf_granny_track_group {
	const char *name;
	int transform_track_count;
	const bf_granny_transform_track *transform_tracks;
	int scalar_track_count;       /* counted, not decoded (none in the RO files) */
	int text_track_count;         /* counted, not decoded (none in the RO files) */
	bf_granny_transform initial_placement;
	int accumulation_flags;
	float loop_translation[3];
} bf_granny_track_group;

typedef struct bf_granny_animation {
	const char *name;
	float duration;               /* seconds */
	float time_step;              /* seconds between source frames */
	int track_group_count;
	const int *track_groups;      /* into the file's track groups */
} bf_granny_animation;

const bf_granny_track_group *bf_granny_get_track_group(const bf_granny_file *file, int index);
const bf_granny_animation *bf_granny_get_animation(const bf_granny_file *file, int index);

/* --- Texture pixels (M12.3b) ---------------------------------------------- */

/*
 * Decodes texture index (image 0, MIP 0) into bgra: width * height pixels of
 * B, G, R, A bytes, rows from the top (the same bytes the client builds from
 * GrannyCopyTextureImage's RGBA8888 by swapping red and blue). bgra must hold
 * width * height * 4 bytes. *has_alpha (may be NULL) is set as
 * GrannyTextureHasAlpha would: 1 when the layout has alpha bits and the
 * picture carries alpha. Decodes on every call: the caller keeps the result.
 * Returns 1, or 0 with a trace line (unknown encoding, damaged data).
 */
int bf_granny_texture_pixels(const bf_granny_file *file, int index, uint8_t *bgra, int *has_alpha);

/* --- Posing and skinning (M12.3b) ----------------------------------------- */

/*
 * Matrices are 4x4 floats, row-major, for row vectors (v' = v * M, the
 * translation in elements 12..14), as GrannyGetWorldPoseComposite4x4 hands
 * them to the client.
 */

/* Evaluates a curve at t seconds into dimension floats (orientation results
 * are normalised). Returns 0, leaving out alone, when the curve is absent. */
int bf_granny_evaluate_curve(const bf_granny_curve *curve, float t, float *out);

/* The matrix of a transform: scale/shear, then rotation, then translation. */
void bf_granny_transform_matrix(const bf_granny_transform *transform, float *matrix);

/*
 * An animation bound to a model's skeleton: each bone gets the transform
 * track of the same name, if the animation has one (what
 * GrannyPlayControlledAnimation does). The animation may come from another
 * file (the 3dmob_bone files); both files must outlive the binding. With
 * anim_file NULL every bone keeps its rest transform.
 */
typedef struct bf_granny_binding bf_granny_binding;
bf_granny_binding *bf_granny_bind_animation(const bf_granny_file *model_file, int model,
	const bf_granny_file *anim_file, int animation);
void bf_granny_free_binding(bf_granny_binding *binding);
int bf_granny_binding_info(const bf_granny_binding *binding, int *bone_count, int *matched_tracks, float *duration);

/*
 * The local pose at t seconds (clamped to 0..duration; looping is the
 * caller's): bone_count transforms, a bone without a track or a curve
 * keeping its rest value (GrannySampleModelAnimations). Returns bone_count.
 */
int bf_granny_sample_local(const bf_granny_binding *binding, float t, bf_granny_transform *local);

/*
 * World matrices from a local pose (GrannyBuildWorldPose): world[b] =
 * local[b] * world[parent], and offset (may be NULL: identity) after the
 * roots. composite (may be NULL) gets inverse_world[b] * world[b], what
 * skinning multiplies rest-pose vertices by
 * (GrannyGetWorldPoseComposite4x4Array). 16 floats per bone each. Returns
 * bone_count.
 *
 * The stored inverse_world matrices include the model's initial_placement.
 * Without it as offset, as the client calls Granny, the rest pose's
 * composite is the inverse of the placement, not identity: the treasure box
 * lies rotated by -90 degrees about X and the knight guardian is shifted by
 * 7.25 units, which the client's own draw matrices account for (see
 * FORMAT.md, "Animations").
 */
int bf_granny_build_world(const bf_granny_skeleton *skeleton, const bf_granny_transform *local, const float *offset,
	float *world, float *composite);

/* bf_granny_sample_local + bf_granny_build_world without offset. */
int bf_granny_sample(const bf_granny_binding *binding, float t, float *world, float *composite);

/* granny_pnt332_vertex. */
typedef struct bf_granny_pnt332 {
	float position[3];
	float normal[3];
	float uv[2];
} bf_granny_pnt332;

/*
 * Skins a mesh on the CPU (GrannyDeformVertices, position and normal): each
 * vertex is the weighted sum of its bones' composites applied to its rest
 * position; normals are not renormalised. bones maps the mesh's bone
 * bindings to skeleton bones (GrannyGetMeshBindingToBoneIndices); NULL uses
 * the bindings resolved at open against the model's own skeleton. Rigid
 * meshes follow their one bone. out holds vertex_count vertices. Returns
 * vertex_count.
 */
int bf_granny_deform(const bf_granny_file *file, int mesh, const int *bones, const float *composite,
	bf_granny_pnt332 *out);

/* --- The client's runtime (M11.10) ---------------------------------------- */

/*
 * What the client did with granny2.dll beyond reading files: model
 * instances with a clock, controls (one animation each, a start time, a loop
 * count, active or not), local and world poses, mesh bindings to another
 * model's skeleton, and the vertex and index copies its buffers take. The
 * rules are those the port's Granny adapter of M12.3a found and the client's
 * scenes were checked with (guild flags, Emperium, guardians, corpses):
 *
 * - A control starts with loop count 1 and active. Loop count 0 plays for
 *   ever. Its time is the instance's clock less its start; within its loops
 *   the pose is that time modulo the animation's duration, after the last
 *   loop the end pose is held.
 * - Sampling an instance takes its most recently started active control; no
 *   blending (the client frees all controls before starting another, and an
 *   attack's control has ended when the stand control starts). Without an
 *   active control the instance is in its rest pose.
 * - Controls are freed by the caller only (granny2.dll's
 *   GrannyFreeCompletedModelControls frees only controls marked for
 *   completion, which the client never does: it asks a finished attack
 *   control for its time left afterwards). Freeing an instance detaches its
 *   controls, which stay valid until freed and no longer move anything.
 *
 * Counters (in the trace at quit): granny.files and granny.file_refused
 * (bf_granny_open), granny.bone_unbound, granny.animation_unbound,
 * granny.animation_no_tracks, granny.index_truncated.
 *
 * Instances, controls and poses are used by one thread at a time.
 */
typedef struct bf_granny_instance bf_granny_instance;
typedef struct bf_granny_control bf_granny_control;
typedef struct bf_granny_world_pose bf_granny_world_pose;

/* An instance of a model of file (GrannyInstantiateModel), clock 0, no
 * controls. NULL when the model or its skeleton is not there. The file must
 * outlive it. */
bf_granny_instance *bf_granny_new_instance(const bf_granny_file *file, int model);
void bf_granny_free_instance(bf_granny_instance *instance);
/* The model's skeleton (GrannyGetSourceSkeleton). */
const bf_granny_skeleton *bf_granny_instance_skeleton(const bf_granny_instance *instance);
/* The clock of the instance and of every control on it (GrannySetModelClock). */
void bf_granny_set_clock(bf_granny_instance *instance, float clock);

/*
 * Starts animation of anim_file (any file: the 3dmob_bone files) on the
 * instance at start seconds of its clock (GrannyPlayControlledAnimation):
 * loop count 1, active. NULL, with a trace line and granny.animation_unbound,
 * when it does not bind; an animation that moves none of the model's bones
 * is counted as granny.animation_no_tracks and still played. anim_file must
 * outlive the control.
 */
bf_granny_control *bf_granny_play(bf_granny_instance *instance, float start, const bf_granny_file *anim_file,
	int animation);
void bf_granny_set_loop_count(bf_granny_control *control, int loops);   /* 0: for ever; negative as 0 */
void bf_granny_set_active(bf_granny_control *control, int active);
/* loops * duration - (clock - start); 1e30 for loop count 0; 0 for NULL
 * (GrannyGetControlDurationLeft). */
float bf_granny_time_left(const bf_granny_control *control);
/* Removes the control from its instance and frees it; NULL is ignored. */
void bf_granny_free_control(bf_granny_control *control);

/*
 * The instance's local pose at its clock (GrannySampleModelAnimations), by
 * the rules above, into local, which holds count transforms (the first count
 * bones are written if the skeleton has more). Returns the skeleton's bone
 * count, 0 for a NULL argument.
 */
int bf_granny_sample_instance(bf_granny_instance *instance, bf_granny_transform *local, int count);

/* A world pose of bone_count bones, identity until built (GrannyNewWorldPose). */
bf_granny_world_pose *bf_granny_new_world_pose(int bone_count);
void bf_granny_free_world_pose(bf_granny_world_pose *pose);
/*
 * bf_granny_build_world without offset into the pose (GrannyBuildWorldPose),
 * which grows to the skeleton's bone count if it is smaller. local holds
 * count transforms; nothing is done (0) when that is fewer than the
 * skeleton's bones. Returns the bone count.
 */
int bf_granny_build_world_pose(const bf_granny_skeleton *skeleton, const bf_granny_transform *local, int count,
	bf_granny_world_pose *pose);
/* The composite matrix of bone (16 floats, the following bones' after it:
 * bone 0 gives the whole array, GrannyGetWorldPoseComposite4x4Array); NULL
 * when bone is not in the pose. */
float *bf_granny_world_pose_composite(bf_granny_world_pose *pose, int bone);

/*
 * The mesh's bone bindings matched by name to the bones of skeleton, which
 * may be another file's (GrannyNewMeshBinding, GrannyGetMeshBindingToBoneIndices):
 * the exact name, else the name ignoring case; a bone not found is bound to
 * bone 0, traced and counted as granny.bone_unbound. Returns
 * mesh->bone_binding_count indices for bf_granny_deform's bones, to be freed
 * with bf_granny_free_mesh_binding; NULL when the mesh has no bindings or an
 * argument is NULL.
 */
int *bf_granny_bind_mesh(const bf_granny_mesh *mesh, const bf_granny_skeleton *skeleton);
void bf_granny_free_mesh_binding(int *bones);

/* The rest-pose vertices as granny_pnt332_vertex (GrannyCopyMeshVertices):
 * out holds vertex_count. Returns vertex_count. */
int bf_granny_copy_vertices(const bf_granny_mesh *mesh, bf_granny_pnt332 *out);
/* The indices as 16 bits (GrannyCopyMeshIndices with 2 bytes): out holds
 * index_count; an index above 65535 is cut and counted as
 * granny.index_truncated. Returns index_count. */
int bf_granny_copy_indices16(const bf_granny_mesh *mesh, uint16_t *out);

#ifdef __cplusplus
}
#endif

#endif
