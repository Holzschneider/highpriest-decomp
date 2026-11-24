# formats/granny: the Granny 2 model reader

Reads the game's `.gr2` files (guild flag, Emperium, guardians, treasure box and their animation files) without `granny2.dll`, which exists for x86 Windows only. Step M12.3; drawing the models is M12.3a.

## What it offers

`include/bf_granny.h` is the contract. `bf_granny_open()` takes a file's bytes and returns a handle from which the caller reads plain arrays:

- the container: header, six sections (Oodle0-decompressed), pointer fix-ups, the root type's members;
- textures (name, size, encoding, stored bytes) and materials (diffuse and opacity texture);
- skeletons: bones with parent, rest transform and inverse world matrix;
- meshes: vertices converted to position, normal, uv, four bone indices and weights; 32-bit indices; triangle groups; materials; bone bindings resolved to the model's skeleton;
- models: skeleton and meshes;
- animations: track groups with one transform track per bone, each with position, orientation and scale/shear curves as stored (degree, knots, controls).

What the format is, how Oodle0 works, which public sources were read, and what the client calls is in [FORMAT.md](FORMAT.md).

## Test

```
cmake -S port/native/formats/granny -B build/native/granny -G Ninja
cmake --build build/native/granny
build/native/granny/bifrost-granny-test            # all files; a name filter may follow
```

Without game data it checks a small file built in memory (fix-ups, type tree, variant arrays, out-of-range access) and six broken variants. With `data.grf` in `BF_CLIENT_DIR` (default `~/Games/Bifrost`) it reads all 21 `.gr2` files straight from the archive, prints sections, root members, textures, materials, skeleton, meshes and animation tracks, checks what the data allows (groups cover all triangles, bone bindings resolve, weights sum to 255, normals are unit length, curves are well formed, every track names a bone, knots lie within the duration), feeds 40 damaged copies of each file to the reader, and draws each mesh flat-shaded in its rest pose to `BF_CLIENT_DIR/test/granny/` (`<file>.mesh<n>-<name>.png`, and `<file>.model.png` for several meshes together). Those pictures show game artwork and stay out of git. It takes under a second.

## Not done, untested

- **Texture pixels.** Every texture is Bink-encoded (encoding 3); the reader returns the stored bytes only. A decoder is needed before the models can be drawn textured.
- **Curve evaluation**, posing, world matrices and skinning: the data is all there; the arithmetic is M12.3a's.
- Only what the 21 RO files use is implemented: format 6, little-endian, 32-bit pointers, Oodle0 or stored sections. Oodle1, BitKnit, big-endian files and format 7 are refused with a reason.
- No byte-for-byte reference exists for the Oodle0 output (the RAD library is not run); correctness rests on 43 sections decoding to their exact sizes with a coherent object tree and correct-looking geometry (see FORMAT.md).

## For port/README.md

> **Granny reader** (`port/native/formats/granny`): `bifrost-granny-test` reads every `.gr2` from `data.grf`, prints its contents and draws each mesh to `~/Games/Bifrost/test/granny/`. Textures (Bink-encoded) and animation evaluation are not done yet; see the module's FORMAT.md.
