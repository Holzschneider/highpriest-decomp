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

Matrices are row-major for row vectors, as the client reads Granny's. The world pose applies no model placement unless given one, as Granny did when the client called it; the client's own draw matrix allows for that (FORMAT.md, "Animations").

What the format is, how Oodle0 and the texture coder work, how the curve convention was established, which public sources were read, and what the client calls is in [FORMAT.md](FORMAT.md).

## Test

```
cmake -S port/native/formats/granny -B build/native/granny -G Ninja
cmake --build build/native/granny
build/native/granny/bifrost-granny-test            # all files; a name filter may follow
```

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
- Clocks, looping, loop counts and blending between animations (Granny's controls) are the caller's.

## For port/README.md

> **Granny reader** (`port/native/formats/granny`): `bifrost-granny-test` reads every `.gr2` from `data.grf`, prints its contents, decodes its textures and draws each model textured and posed through its animations to `~/Games/Bifrost/test/granny/`. See the module's FORMAT.md for the format and the evidence.
