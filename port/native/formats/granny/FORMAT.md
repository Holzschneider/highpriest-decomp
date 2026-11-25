# Granny 2 files as the game uses them

What the reader in this directory knows about `.gr2` files, established from the 21 files of the kRO 2021 archives and from public descriptions (listed at the end). Offsets are in bytes; "s0:0x1234" means offset 0x1234 in decompressed section 0. Example values are from `data\model\3dmob\empelium90_0.gr2` unless stated.

**Where it stops:** everything below is decoded for all 21 files: containers, objects, the 24 embedded textures (M12.3b), and the animation curves, which the reader evaluates, poses and skins on the CPU (M12.3b). What is not known is listed where it applies: the meaning of a texture's 4-byte head word, and the higher curve degrees, which no file uses. No byte-for-byte reference exists for any of it (the RAD library is not run here); the evidence for each part is given with it.

## The 21 files

All live in `data.grf` (none in `rdata.grf`). All 21 entries carry archive flags 3 (file + "mixed" DES, flag 2): the client's GRF reader decodes them (`src/base/GPak.cpp`, `CDec.cpp`); the test program has its own reader (`test/grf.cpp`) with the same scheme: one DES round with an all-zero key on the first 20 blocks and every n-th block after, a byte shuffle on every 8th of the others, then zlib.

| File | Holds | Meshes | Bones | Textures | Animation |
|---|---|---|---|---|---|
| `3dmob/empelium90_0` | Emperium | 1 (159 vertices, 138 triangles) | 18 | 1 | 9.93 s idle |
| `3dmob/guildflag90_1` | guild flag | 2 (banner, 16×16 emblem plane) | 41 | 2 | 5.67 s |
| `3dmob/treasurebox_2` | treasure box | 1 | 22 | 1 | 3.0 s |
| `3dmob/kguardian90_7` | knight guardian | 5 | 35 | 5 | 2.0 s |
| `3dmob/aguardian90_8` | archer guardian | 6 (2 rigid shoulder pieces) | 43 | 4 | 3.33 s |
| `3dmob/sguardian90_9` | soldier guardian | 4 | 31 | 4 | 2.0 s |
| `3dmob_bone/2_{damage,dead}` | treasure box poses | 0 | 22 | 0 | 1.33 s, 3.33 s |
| `3dmob_bone/7_{attack,damage,dead,move}` | knight poses | 0 | 35 | 0 | 1.5–3.67 s |
| `3dmob_bone/8_{attack,damage,dead,move}` | archer poses | 2 rigid | 43 | 0 | 1.5–1.83 s |
| `3dmob_bone/9_{attack,damage,dead,move}` | soldier poses | 0 | 31 | 0 | |
| `3dmob_bone/1_attack` | not a flag: an unfinished 3D novice (`0220R2NFF-attackII01.max`) | 0 | 50 | 7 | 1.13 s |

Every file was written by "Granny Standard Exporter, SDK version 2.1.0.5" from 3D Studio MAX, 39.37 units per metre, up = +Z. The client never loads bone files for type 1 (`C3dGrannyBoneRes::GetAnimation` skips `nType == 1`), which fits `1_attack` being something else.

## Container

All integers little-endian.

### Magic (0x00, 32 bytes)

| Offset | Size | Value | Meaning |
|---|---|---|---|
| 0x00 | 16 | `b8 67 b0 ca f8 6d b1 0f 84 72 8c 7e 5e 19 00 1e` | little-endian, 32-bit pointers |
| 0x10 | 4 | 0x160 | header size (magic + header + section table) |
| 0x14 | 4 | 0 | header format |
| 0x18 | 8 | 0 | reserved |

### Header (0x20, 56 bytes; `granny_grn_file_header`)

| Offset | Field | Empelium |
|---|---|---|
| 0x20 | version | 6 (all 21) |
| 0x24 | total size | 0xCA44 = file size |
| 0x28 | CRC | 0x31c3d35b (not checked by the reader) |
| 0x2C | section table offset, from 0x20 | 0x38 → table at 0x58 |
| 0x30 | section count | 6 (all 21) |
| 0x34 | root type: section, offset | 0, 0xa638 |
| 0x3C | root object: section, offset | 0, 0 |
| 0x44 | type tag | 0x8000000f (all 21) |
| 0x48 | extra tags [4] | 0 |

### Section table (0x58, 6 × 44 bytes)

| +0 | +4 | +8 | +12 | +16 | +20 | +24 | +28 | +32 | +36 | +40 |
|---|---|---|---|---|---|---|---|---|---|---|
| compression | file offset | stored size | decompressed size | alignment | stop16 | stop8 | fix-up table offset | fix-up count | marshalling offset | marshalling count |

Empelium:

```
s0 comp=1 off=0x196c  stored=0x5c9c size=0xc9cc align=4  stops 0xbf14/0xbf14 fixups 0x160 ×513
s1 comp=1 off=0x7608  stored=0     size=0      align=32
s2 comp=1 off=0x7608  stored=0     size=0      align=4
s3 comp=1 off=0x7618  stored=0xd2c size=0x18e0 align=32 marshalling 0x7608 ×1
s4 comp=1 off=0x8344  stored=0x1e4 size=0x678  align=4
s5 comp=0 off=0x8528  stored=0x451c size=0x451c align=4
```

Compression: 0 = stored, 1 = Oodle0. No other value occurs. The six sections have fixed roles, which match the section numbers the client passes to `GrannyFreeFileSection` (`src/resource/Model.cpp`: rigid vertices 1, rigid indices 2, deformable indices 4, textures 5):

| Section | Contents | Evidence |
|---|---|---|
| 0 | type tree, all objects, strings | root type and root object point here; 99 % of fix-ups |
| 1 | rigid-mesh vertices | archer: 2 meshes × 40 vertices × 32 bytes = 2560 = s1 size |
| 2 | rigid-mesh indices | archer: 2 × 28 triangles × 3 × 4 = 672 = s2 size |
| 3 | skinned-mesh vertices | empelium: 159 × 40 = 6360, padded to 32 → 6368 |
| 4 | skinned-mesh indices | empelium: 414 × 4 = 1656 |
| 5 | texture pixels, stored uncompressed | empelium: one 17692-byte image |

### Pointer fix-ups

Pointers inside sections are stored as zero; a table of 12-byte records `{offset in this section, target section, target offset}` says what each points to. In format 6 the tables are stored uncompressed, right after the header: section 0's starts at 0x160, and 0x160 + 513 × 12 = 0x196c is exactly where section 0's data begins. A pointer with no record is null. The reader keeps the table as a map instead of patching (the client is 64-bit; the file's pointer slots are 4 bytes).

First records of empelium: `(0, s0, 0xbbb8)` the string table pointer of the root, `(4, s0, 0x58)` ArtToolInfo, `(8, s0, 0x98)` ExporterInfo.

### Marshalling records

16 bytes `{count, offset, type section, type offset}`, e.g. empelium 0x7608: `159, 0, s0, 0x1a78`: 159 objects at s3:0 of the vertex type. They tell a big-endian reader which bytes to swap; on a little-endian machine nothing needs doing and the reader ignores them.

## Oodle0

43 sections of the 21 files are Oodle0-compressed (1,318,516 bytes decompressed). The reader decodes every one of them to exactly its stated size, and the result is coherent: the type trees name every member of the Granny 2.1 structures, all 17,383 fix-ups land inside their target sections, all strings are terminated, vertex normals have unit length, bone weights sum to 255, every mesh renders as the expected object. A single wrong bit early in a section would garble all of that, so the decoder is taken to be exact; there is no reference output to compare bytes against (no `granny2.dll` on this machine, and it is never to be run).

The scheme, as implemented in `src/bf_granny_oodle0.cpp`:

- **Blocks.** A section's output is produced in three consecutive blocks, ending at `stop16`, `stop8` and the section size (empty blocks are skipped). The stored data starts with three 12-byte block headers (36 bytes); the bit stream follows and runs on across the blocks.
- **Block header.** Word 0: bits 0–8 the literal escape range (256 everywhere), bits 9–31 the largest match distance (262136 everywhere). Word 1: bits 0–8 the number of distinct literals, bits 9–31 the number of distinct high distance parts. Word 2: four bytes, the number of distinct length symbols for previous-length groups 0–15 (most significant byte), 16–31, 32–47, 48–64 (least significant byte).
- **Bits** come from 32-bit little-endian words, least significant bit first.
- **Range decoder.** 31-bit low/high/code, code initialised from the first 31 bits (first bit most significant). Decoding a frequency `[start, start+count)` of a total narrows the range with 64-bit products, then shifts out agreeing top bits (8, then 4, then 1 at a time, the code taking that many stream bits), then expands around the midpoint while low has bit 29 set and high does not (flipping bit 29 of the code). A "uniform" value in `[0, n)` is decoded the same way with count 1.
- **Adaptive models.** Slot 0 is an escape with initial weight 3. Decoding the escape introduces the next slot with weight 2, whose value then follows as a uniform value in a range given by the context. When the alphabet is full the escape's weight drops to 0. Each decoded symbol adds 1 to its weight. Cumulative weights are kept in 16 bins whose width (a power of two) is chosen so the open-ended last bin is smallest; the rounding of the decoder depends on these totals, so they are part of the format. When the total reaches 16384 the model is rescaled: weights halve, symbols at weight ≤ 1 are dropped (the last symbol moves into the gap), the heaviest symbol is moved to the start of the last occupied bin, and the escape gets weight 2 again if it reached 0 while the alphabet is not full.
- **LZ77.** One model for literals, 65 models for the length symbol (one per previous length symbol, sized from the header byte of its group), one model for the low two bits of the distance (alphabet min(max distance, 4)), one for the high part. Length symbol 0 is a literal byte; 1–60 copy symbol + 1 bytes; 61–64 copy 128, 192, 256, 512 bytes. The distance is `low + 1 + 4 × high`; `high` escapes into the range `min(max distance, bytes so far in this block) / 4 + 1`. Copies may overlap, and may reach into earlier blocks.

The reader rejects headers whose alphabets exceed what their values can hold (more than 256 literals, more high distance parts than the section has room for), counts it as a damaged file, and never reads past the input (missing bits are zeros, and a block that runs more than 8 bytes past the end fails).

## Type tree

Each type is an array of 32-byte member records, ended by a record of type 0:

| +0 | +4 | +8 | +12 | +16 | +28 |
|---|---|---|---|---|---|
| member type | name (pointer) | referenced type (pointer) | array width (0 = one) | 3 × reserved | traversal id |

Member types and their size in the 32-bit layout: 1 inline (the referenced type's size), 2 reference (4), 3 reference to array (count, pointer: 8), 4 array of references (count, pointer to a pointer table: 8), 5 variant reference (type pointer, object pointer: 8), 7 reference to variant array (type pointer, count, pointer: 12), 8 string (4), 9 transform (68: flags, position[3], orientation[4], scale/shear[3][3]), 10 real32, 11–14 int8/uint8/binormal-int8/normal-uint8 (1), 15–18 the 16-bit kinds (2), 19–20 int32/uint32 (4); scalars times the array width. Type 6 does not occur.

The root type (empelium s0:0xa638) has these members, in this order, identical in all 21 files:

```
_GrannyFileStringTable  reference            (StringOffsets[], Characters[])
ArtToolInfo             reference            FromArtToolName, revisions, UnitsPerMeter, Origin, Right/Up/BackVector
ExporterInfo            reference
FromFileName            string
Textures                array-of-references  FromFileName, TextureType, Width, Height, Encoding, SubFormat,
                                             Layout{BytesPerPixel, ShiftForComponent[4], BitsForComponent[4]},
                                             Images[]{MIPLevels[]{Stride, Pixels[] (uint8)}}, ExtendedData
Materials               array-of-references  Name, Maps[]{Usage, Map → material}, Texture, ExtendedData
Skeletons               array-of-references  Name, Bones[]{Name, ParentIndex, Transform, InverseWorldTransform real32[16],
                                             LightInfo, CameraInfo, ExtendedData}
VertexDatas             array-of-references  Vertices (variant array), VertexAnnotationSets[]
TriTopologies           array-of-references  Groups[]{MaterialIndex, TriFirst, TriCount}, Indices[] (int32), Indices16[],
                                             VertexToVertexMap, VertexToTriangleMap, SideToNeighborMap,
                                             BonesForTriangle, TriangleToBoneIndices, TriAnnotationSets
Meshes                  array-of-references  Name, PrimaryVertexData, MorphTargets[], PrimaryTopology,
                                             MaterialBindings[]{Material}, BoneBindings[]{BoneName, OBBMin, OBBMax,
                                             TriangleIndices[]}, ExtendedData
Models                  array-of-references  Name, Skeleton, InitialPlacement, MeshBindings[]{Mesh}
TrackGroups             array-of-references  Name, ScalarTracks[], TransformTracks[]{Name, Position/Orientation/
                                             ScaleShearCurve{Degree, Knots[], Controls[]}}, TextTracks[],
                                             InitialPlacement, AccumulationFlags, LoopTranslation, PeriodicLoop,
                                             ExtendedData
Animations              array-of-references  Name, Duration, TimeStep, TrackGroups (array of references)
```

Note two names that differ from RAD's 2.1 header in `src/granny/`: the bone's transform is `Transform` (header: `LocalTransform`) and its matrix `InverseWorldTransform` (header: `InverseWorld4x4`). The reader looks members up by name, accepting both.

### Vertices

The vertex type is stored with each vertex array. Two layouts occur:

- skinned, 40 bytes: `Position real32[3], BoneWeights normal-uint8[4], BoneIndices uint8[4], Normal real32[3], TextureCoordinates0 real32[2]` (17 of the 27 meshes);
- rigid, 32 bytes: `Position real32[3], Normal real32[3], TextureCoordinates0 real32[2]` (the other 10: the archer's two shoulder pieces, in `aguardian90_8` and each `8_*` file; one bone binding each, no material binding, so the client draws them untextured).

Bone indices refer to the mesh's `BoneBindings`, matched to the skeleton by name; positions are the rest pose in model space (3ds Max units, Z up). Indices are 32-bit in every file (`Indices16` is always empty); one triangle group per mesh.

## Textures

Every texture (17 in the six model files, 7 in `1_attack`) has `Encoding` 3, which public notes on these files and the Granny encoding names give as RAD's Bink texture compression (0 user, 1 raw, 2 S3TC, 3 Bink), one image, one MIP level, `Stride` 0, and its bytes in section 5. The archives hold no separate copy of these pictures (searched by name). The texture file names (`D:\ChalesWORK_2003\...\empelium.tif`) are what the client matches materials to. `bf_granny_texture_pixels` decodes them (`src/bf_granny_texture.cpp`); all 24 decode.

**Plain pictures.** A picture of at most 256 pixels is stored as RGBA bytes: the guild flag's 16×16 emblem placeholder (1024 bytes), which the client replaces with the guild's emblem (`C3dGrannyGameActor::RenderGuildMark`).

**Wavelet pictures.** Everything larger is a wavelet image coder:

- A 4-byte head word (`00 01 02 00` for every 256×256, `10 83 00 00` for the knight's 64×128; its meaning is not established and the decoder does not need it), then three or four **planes**: luma Y, R−G and B−G, and alpha when the layout has 8 alpha bits (the 3-byte layouts of `1_attack` have three planes, ~13.2 KB; the 4-byte ones four, ~17.6 KB). Colour: G = Y − (RG + BG) / 4 (integer division towards zero), R = RG + G, B = BG + G, each clamped to 0–255. Rows run from the top; the client uses the mesh's v as stored.
- A plane covers the picture rounded up to 16 pixels each way, as 16-bit samples, and is `{symbol bytes: u32, raw bytes: u32, symbols, raw bits}`. The symbol stream goes through the same range decoder and adaptive models as Oodle0 (section "Oodle0"); the raw stream (32-bit little-endian words, least significant bit first) holds parameters, signs, and the values of newly seen symbols.
- The plane is a 4-level decomposition. The lowest band (1/16 size each way) is either constant (flag bit, 16-bit value) or a 16-bit first sample, then steps coded with one model (escape range from a 16-bit count) and a sign bit each: the first row predicts from the left, the first column from above, the rest from the mean of left and above (halved towards zero).
- Then, per level, three detail bands (right of, below, and diagonal to the band above them): a 16-bit scale; a constant band (flag bit, 16-bit value × scale) or a 16-bit largest magnitude and values coded in alternating runs. A run pair is a count of coded values (a 64-symbol model, new symbols as 6 raw bits; 60–63 mean 128, 256, 512, 1024) and a count of zeros (a 256-symbol model, new symbols as 8 raw bits; 0 = none, 252–255 mean 514, 1026, 2050, 3074, others n + 2). A coded value's model is chosen by the bit length (at most 15) of a quarter of the sum of its neighbours' magnitudes (west, north-west, north, north-east; at a row's end twice west instead of north-east), magnitudes × scale, a raw sign bit for non-zero ones.
- Inverse transform, coarsest level first: rows, then columns, with the 9/7 synthesis in 16.16 fixed point (bands mirrored at their ends, 16-bit results), or a two-tap synthesis ((2l ± h) / 2) where an area is under 12 samples wide or 10 high. Samples are 16-bit throughout and wrap.

Evidence: every texture decodes to a clean picture (looked at: the Emperium's crystals, the guardians' armour, the flag's banner and cord, the novice's clothes); a wrong bit would turn the rest of a plane into noise, and the test checks a mean neighbour difference of 1–13 per colour byte against a noise threshold of 25. On the models, the decoded pictures land on the right faces (the treasure box's lock, the flag's fringe, the knight's tabard). The decoder was written from the description of an open-source decoder (see "Sources"), whose author reports matching a reference decoder on these files.

## Animations

Each file has one animation with one track group whose transform tracks name skeleton bones (all names resolve in every file, and every bone of every model has a track in each of its animations). Each track has three curves (position 3 floats, orientation 4 floats as quaternion x y z w, scale/shear 9 floats) in the Granny 2.1 form `{Degree, Knots[], Controls[]}`:

- no knots: the bone keeps its rest value (most scale/shear curves);
- one knot at 0, degree 0: a constant, equal to the rest value where checked (`Bip01 HeadNub` position 2.551 0 0 = its rest position);
- degree 2 with n knots and n control points: a quadratic B-spline; knots are times in seconds, rising, the last equal to the animation's duration (knight `7_move`, `Bip01 Tail2` orientation: 65 knots at 1/32 s steps from 0 to 2.0; Emperium `Bip01 Head`: 0, 4.97, 5.59, … 9.93, non-uniform, fitted by the exporter).

Degrees 1 and 3 do not occur (835 curves of degree 0, 462 of degree 2). `TimeStep` is 1/30 s and `AccumulationFlags` 1 in every file.

### Evaluation (`bf_granny_evaluate_curve`)

For t, take i = the first knot after t (the last knot if none); the value is the de Boor blend of controls i−2, i−1, i over knots i−2 … i+1, every index clamped to the stored arrays. The curve then starts at the first control and ends on the last. Orientations are blended component-wise and normalised.

How this was established from the data (`bifrost-granny-test` and a probe that was not kept): on the 111 non-uniform curves of all 21 files, sampled at 600 Hz, the squared second derivative summed over the animation is 8.8 × 10⁴ in total for this rule, 2.8 × 10⁶ for joining the controls linearly as keyframes at the knot times, and 8.7 × 10⁹ for the same rule shifted by one knot (controls i−1 … i+1), lower in every file. The curves are continuous at every knot (values 10⁻⁴ s either side differ by at most 0.02, which is the slope, not a step) and start and end on their end controls (within 0.0014). Of 25,817 consecutive pairs of orientation controls none lie in opposite hemispheres, so blending needs no sign correction. The rule matches the one in an open-source Granny importer (see "Sources"); the data, not that importer, decided it.

### Pose (`bf_granny_sample_local`, `bf_granny_build_world`, `bf_granny_deform`)

- A transform's matrix is scale/shear, then rotation, then translation, for row vectors (v' = v × M, translation in the last row, as the client reads `GrannyGetWorldPoseComposite4x4`); quaternion x y z w with the usual right-handed rotation matrix. Bones without the scale/shear flag (4) use the identity.
- World = local × world(parent). The rest pose rebuilt this way reproduces the stored inverse world matrices to 8 × 10⁻⁶ in all six models, which settles the order, handedness and convention.
- **Placement.** The stored inverse world matrices include the model's `InitialPlacement` (identity for four models; a 7.25-unit shift for the knight, the same in its `7_*` files; a 90° turn about X for the treasure box; a shift of 35 for `1_attack`). `GrannyBuildWorldPose` is called by the client without an offset, so its rest-pose composites are the inverse of the placement, not the identity: the treasure box comes out lying on its back. The client allows for it: `C3dGrannyGameActor::RenderNormal` turns models whose name contains "90" by π/2 about X and the others (the treasure box) by π, exactly the treasure box's extra quarter turn. `bf_granny_build_world` therefore applies no placement unless given one as `offset`; the test passes it to draw models as authored.
- Skinning: a vertex is the weighted sum of its (up to four) bones' composites applied to its rest position and normal; rigid meshes follow their single bone.

Checks on the posed models: every animation of every model binds all bones by name; the guardians' walk cycles (`7_move`, `8_move`, `9_move`) close, every bone within 0.9 % of the model's height between the first and the last frame; bone lengths stay within 5 % of the height through every animation except for helper bones that are animated free by design (Biped's `Footsteps`, the root `Bip01`) and the archer's shoulder pad bone `Bip01 L UpperArm01` (5.2 % in `8_attack`, 65 % in `8_dead`), whose stored position controls in `8_dead` move it from (3.0, 0.0, 2.8) to (18.8, −2.2, 7.5) between 1.8 s and 2.5 s: the pad flies off as he dies. The pictures (`<model>+<animation>.posed.png`) were looked at: walking, the knight's sword swing, the archer drawing his bow and falling, the treasure box bursting open, the flag swaying, the Emperium turning.

## What the client needs

From `src/resource/Model.cpp`, `src/GameActor3d.cpp`, `src/Pc.cpp`, `src/Session.cpp`:

- **Which files.** 69 job names end in `.gr2` (`Session.cpp`): 55 × `TREASUREBOX_2`, 4 × `Kguardian90_7`, 4 × `Aguardian90_8`, 2 × `Empelium90_0`, `Sguardian90_9`, `Guildflag90_1`, and two that do not exist in the archives (`Hugeling90_6`, `dragon_5`). They load from `model\3dmob\`; the digit after `_` is the bone type, whose animations load from `model\3dmob_bone\<type>_<stand|move|attack|dead|damage>.gr2` (stand from the model file itself). `CGrannyPc` (`Pc.cpp`) loads a player model that is not in the archives.
- **Per file** (`GrannyReadEntireFileFromMemory`, `GrannyGetFileInfo`): `TextureCount`/`Textures`, `Models[0]`, `Skeletons[0]`, `AnimationCount`/`Animations[0]`.
- **Textures**: `FromFileName`, `TextureType == 0`, `ImageCount == 1`, `GrannyTextureHasAlpha`, `GrannyCopyTextureImage` to RGBA8888, which the client then swaps to BGRA: `bf_granny_texture_pixels` hands out BGRA directly.
- **Meshes** of `Models[0]->MeshBindings`: `GrannyGetMeshIndexCount`, `GrannyCopyMeshIndices` (16-bit), `GrannyGetMeshVertexCount`, `GrannyMeshIsRigid`, `GrannyCopyMeshVertices` to `granny_pnt332_vertex` (position, normal, uv) for rigid meshes, a deformer (`GrannyNewMeshDeformer`, `GrannyDeformVertices`, position and normal, four weights) for skinned ones, `GrannyGetMeshTriangleGroups` (first group's material), `MaterialBindings` → `GrannyGetMaterialTextureByType(Diffuse)`, the mesh name (`hair_pivot` for the player model), `GrannyNewMeshBinding` (maps the mesh's bone bindings to a skeleton, also to another file's skeleton in `BindMeshToSkeleton`).
- **Animation**: `GrannyInstantiateModel`, `GrannyPlayControlledAnimation`, `GrannySetControlLoopCount`, `GrannySetControlActive`, `GrannyGetControlDurationLeft`, `GrannySetModelClock`, `GrannySampleModelAnimations`, `GrannyBuildWorldPose`, `GrannyGetWorldPoseComposite4x4(Array)`, `GrannyFreeCompletedModelControls`: curve evaluation, local-to-world, composite = inverse world × world.

The reader now supplies all of this: `bf_granny_bind_animation` (the play-control binding by bone names, across files), `bf_granny_sample_local` (`GrannySampleModelAnimations`), `bf_granny_build_world` (`GrannyBuildWorldPose` and the composites), `bf_granny_deform` (`GrannyDeformVertices` to `granny_pnt332_vertex`). What stays the client's (M12.3a): the clock, looping and loop counts, the controls' durations, and blending between animations, which Granny's controls did and these files never need at once.

## Sources

Read to understand the format; no code was copied. The implementation here is written for this port in C++.

- Ragnarok Research Lab, "GR2": <https://ragnarokresearchlab.github.io/file-formats/gr2> (segment roles, Oodle0 = LZ77 + adaptive arithmetic coding, citing F. Giesen).
- rdw-archive, RagnarokFileFormats `GR2.MD`: <https://github.com/rdw-archive/RagnarokFileFormats/blob/master/GR2.MD> (header and section fields).
- opengr2 wiki, "File Format documentation": <https://github.com/arves100/opengr2/wiki/File-Format-documentation> (type-tree member record).
- granny-ro-js: README and the doc comments in `dist/oodle0.d.ts` (block header fields, constants) <https://cdn.jsdelivr.net/npm/granny-ro-js@1.5.0/>. Its README says the leaked Granny SDK source was consulted there; its implementation was not read for this work.
- Rasetsuu/blendergranny (MIT, described by its author as clean-room): `README.md`, `docs/OODLE0_RESEARCH.md`, `docs/PROVENANCE.md`, and `io_scene_gr2/gr2/decompress/oodle0.py`, which was read to learn the Oodle0 range coder, model and rescaling rules. A bit-exact decoder has only one possible behaviour, so `src/bf_granny_oodle0.cpp` behaves as that file does; its code, structure and names are this port's own. <https://github.com/Rasetsuu/blendergranny>
- For the textures: the description and the decoder `src/gr2/bink.rs` (Apache-2.0) in a public pull request of an open-source Ragnarok client, read to learn the wavelet coder's layout, band coding, run lengths, filters and colour transform; `src/bf_granny_texture.cpp` is written for this port, and it reuses this reader's own range decoder and models. The project's name and address are given in the run's report, not here, because it may be one of the owner's own projects.
- For curve evaluation: blendergranny's `io_scene_gr2/gr2/animation.py` (`_evaluate_curve`), whose knot window was the first hypothesis tested against the data (see "Animations").
- The client's own source in this repository: `src/granny/granny.h` (RAD's 2.1 header: structure and API names; deleted from `macos-port` in M11.10, still on `main`), `src/resource/Model.cpp`, `src/GameActor3d.cpp`, `src/Session.cpp`, `src/Pc.cpp`, `src/base/GPak.cpp`, `src/base/CDec.cpp`.
- FIPS 46 (DES tables) for the test program's archive reader.
