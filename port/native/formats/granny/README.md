# formats/granny: the Granny 2 model reader

Reads the game's `.gr2` files (guild flag, Emperium, guardians, treasure box and their animation files) without `granny2.dll`, which exists for x86 Windows only, and poses them. Steps M12.3 (reader) and M12.3b (textures, curves, posing); drawing the models in the client is M12.3a.

## What it offers

`include/bf_granny.h` is the contract. `bf_granny_open()` takes a file's bytes and returns a handle from which the caller reads plain arrays:

- the container: header, six sections (Oodle0-decompressed), pointer fix-ups, the root type's members;
- textures (name, size, encoding, stored bytes) and materials (diffuse and opacity texture); `bf_granny_texture_pixels` decodes a texture to BGRA (`GrannyCopyTextureImage` plus the client's channel swap);
- skeletons: bones with parent, rest transform and inverse world matrix;
- meshes: vertices converted to position, normal, uv, four bone indices and weights; 32-bit indices; triangle groups; materials; bone bindings resolved to the model's skeleton;
- models: skeleton and meshes;
- animations: track groups with one transform track per bone, each with position, orientation and scale/shear curves (degree, knots, controls);
- posing, the parts of Granny's runtime the client calls: `bf_granny_evaluate_curve`, `bf_granny_bind_animation` (an animation, from any file, bound to a model's bones by name), `bf_granny_sample_local` (`GrannySampleModelAnimations`), `bf_granny_build_world` (`GrannyBuildWorldPose` and the skinning composites), `bf_granny_sample` (both), `bf_granny_deform` (`GrannyDeformVertices` into `granny_pnt332_vertex`).

- the client's runtime (M11.10, moved from the port's Granny adapter when M11 deleted it): model instances with a clock and controls (`bf_granny_new_instance`, `bf_granny_play`, loop counts, active, `bf_granny_time_left`, `bf_granny_sample_instance`: the most recently started active control, looped and held at its end, the rest pose without one, no blending), world poses (`bf_granny_new_world_pose`, `bf_granny_build_world_pose`, `bf_granny_world_pose_composite`), mesh bindings to another model's skeleton by bone name (`bf_granny_bind_mesh`: exact, then ignoring case, else bone 0), and the copies the client's buffers take (`bf_granny_copy_vertices` to PNT332, `bf_granny_copy_indices16`). The client (`src/resource/Model.cpp`, `src/GameActor3d.cpp`, `src/Pc.cpp`) calls these and the reader directly. Counters: `granny.files`, `granny.file_refused`, `granny.bone_unbound`, `granny.animation_unbound`, `granny.animation_no_tracks`, `granny.index_truncated`.

Matrices are row-major for row vectors, as the client reads Granny's. The world pose applies no model placement unless given one, as Granny did when the client called it; the client's own draw matrix allows for that (FORMAT.md, "Animations").

What the format is, how Oodle0 and the texture coder work, how the curve convention was established, which public sources were read, and what the client calls is in [FORMAT.md](FORMAT.md).

## Test

```
cmake -S port/native/formats/granny -B build/native/granny -G Ninja
cmake --build build/native/granny
build/native/granny/bifrost-granny-test            # all files; a name filter may follow
```

It also replays the client's load, animate and skin sequences through the runtime (`test/runtime_checks.cpp`, from the deleted adapter's test): every file loads and every mesh fits the client's buffers (16-bit indices, `GetMeshIndexCount * 3` vertices for the deformed copy); the Emperium's texture decodes, its stand animation (read from its file a second time) moves it, loops after its duration, leaves the rest pose when inactive; a loop count of 1 ends after one pass and holds its end pose; a control outlives its freed instance; the guild flag's own animation sways it; the knight guardian walks with `7_move` from another file. Without data only the NULL handles and the new world pose are checked.

Without game data it checks a small file built in memory (fix-ups, type tree, variant arrays, out-of-range access), six broken variants, a hand-made curve and the transform convention. With `data.grf` in `BF_CLIENT_DIR` (default `~/Games/Bifrost`) it reads all 21 `.gr2` files straight from the archive and:

- prints sections, root members, textures, materials, skeleton, meshes and animation tracks, and checks what the data allows (groups cover all triangles, bone bindings resolve, weights sum to 255, normals are unit length, curves are well formed, every track names a bone, knots lie within the duration), feeding 40 damaged copies of each file to the reader;
- decodes all 24 textures, checks they are pictures rather than noise, feeds damaged texture data to the decoder, and writes them as `<file>.texture<n>.png`;
- checks the curves (continuous at knots, ending on their end controls, quaternion controls in one hemisphere), that the rest pose rebuilds the stored inverse world matrices, that every model's animations (its own and its type's `3dmob_bone` files) bind every bone, that walk cycles close, and how far bones stretch;
- draws each mesh flat-shaded in its rest pose (`<file>.mesh<n>-<name>.png`, `<file>.model.png`) and each model textured and skinned at four times of each of its animations (`<model>+<animation>.posed.png`; `BF_GRANNY_FRAMES=8` for more).

The pictures go to `BF_CLIENT_DIR/test/granny/`; they show game artwork and stay out of git. The run takes about 4 seconds.

## Not done, untested

- Only what the 21 RO files use is implemented: format 6, little-endian, 32-bit pointers, Oodle0 or stored sections, texture encoding 3 (and raw), curve degrees 0 and 2 (1 is evaluated linearly, 3 and up as quadratic; neither occurs). Oodle1, BitKnit, big-endian files and format 7 are refused with a reason.
- No byte-for-byte reference exists for the Oodle0 output, the texture pixels or the poses (the RAD library is not run). The evidence for each, from the files themselves, is in FORMAT.md.
- The meaning of a texture's 4-byte head word is not known; the decoder does not need it.
- Blending between controls is not done (the client never needs it). The clock is the caller's (the client's `bf_clock_us`).

## For port/README.md

> **Granny reader** (`port/native/formats/granny`): `bifrost-granny-test` reads every `.gr2` from `data.grf`, prints its contents, decodes its textures and draws each model textured and posed through its animations to `~/Games/Bifrost/test/granny/`. See the module's FORMAT.md for the format and the evidence.
