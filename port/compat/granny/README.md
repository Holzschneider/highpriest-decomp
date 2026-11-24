# granny: the Granny adapter (stage A)

This adapter defines the 44 `Granny*` functions and the 2 data names of RAD's `granny2.dll` that the client calls, on top of the reader in [`port/native/formats/granny`](../../native/formats/granny/README.md) (step M12.3a). The callers are `C3dGrannyModelRes` and `C3dGrannyBoneRes` in `src/resource/Model.cpp`, which are built as the 2008 client had them, and the actor classes in `src/GameActor3d.cpp` (`C3dGrannyGameActor`, `CGrannyCorpse3d`) and `src/Pc.cpp` (`CGrannyPc`, never constructed). The adapter gets its declarations from the client's `src/granny/granny.h`, so a definition whose signature differs from RAD's does not compile. M11 deletes this directory and moves the two resource classes onto `bf_granny.h`.

Why an adapter and not a native path in the client: the client's Granny code is confined to the two resource classes, which use Granny as a library of plain calls plus a few structures they read (`granny_file_info`'s arrays, a texture's size and name, a model's skeleton and mesh bindings, a mesh's name and material bindings). The reader already supplies every one of those operations, so the adapter is translation only, and `src/` changes by four guards instead of a rewrite of both classes.

## Build and test

```
cmake --preset debug && cmake --build --preset debug --target bf_compat_granny bifrost-compat-granny-test
build/cmake/debug/compat/granny/bifrost-compat-granny-test
```

The test calls the names in the client's order: `C3dGrannyModelRes::Load` (file, textures through `GrannyTextureHasAlpha` and `GrannyCopyTextureImage`, model instance, world pose, mesh bindings, 16-bit indices, rigid vertices or a deformer, local pose), `C3dGrannyBoneRes::GetAnimation` (the stand animation read from the model's file a second time, or a `3dmob_bone` file), `ActAnimation`, `UpdateAni` and `RenderModel`. Without game data it checks the version, the clock and the NULL handles the actor's destructor passes, and prints `skipped`. With `data.grf` in `BF_CLIENT_DIR` it reads all 21 `.gr2` files, checks that every mesh fits the client's buffers (`GetMeshIndexCount * 3` vertices for the deformed copy, 16-bit indices), that the Emperium's texture decodes to a picture, that its stand animation moves it, loops after its duration and leaves the rest pose when the control is inactive, that a loop count of 1 ends after one pass, that the guild flag's own animation sways it, and that the knight guardian walks with `7_move` from another file. ctest label `headless`; it runs in under a second.

## Objects

The structures the client reads are built when a file is read, from the reader's arrays, as `granny.h` declares them. Each is the first member of an adapter object that also names its file and index, so a pointer the client passes back leads to the reader's data. The types `granny.h` leaves opaque are defined in the adapter: `granny_model_instance` (model, clock, controls), `granny_control` (one bound animation, start, loop count, active), `granny_local_pose`, `granny_world_pose` (world and composite matrices), `granny_mesh_binding` (bone indices), `granny_mesh_deformer`.

## The names

| Function | Maps to |
|---|---|
| `GrannyVersionsMatch_` | true for 2.1.x, what the reader reads |
| `GrannyReadEntireFileFromMemory` | `bf_granny_open`, then the structures above; NULL (and `granny.file_refused`) for a file the reader refuses |
| `GrannyGetFileInfo` | the built `granny_file_info` |
| `GrannyFreeFileSection` | nothing: the reader keeps its sections until the file is closed |
| `GrannyFreeFile` | `bf_granny_close` and the structures |
| `GrannyTextureHasAlpha` | `bf_granny_texture_pixels`' `has_alpha` (decoded once per texture) |
| `GrannyCopyTextureImage` | `bf_granny_texture_pixels`, packed into the requested layout (shifts and bits per component; image 0, MIP 0) |
| `GrannyGetMaterialTextureByType` | the material's diffuse texture (type 2), else its own texture |
| `GrannyGetMeshVertexCount`, `GrannyGetMeshIndexCount`, `GrannyMeshIsRigid` | the reader's mesh |
| `GrannyGetMeshVertexType`, `GrannyGetMeshVertices` | the reader's vertex (`bf_granny_vertex`) and its layout |
| `GrannyCopyMeshVertices` | to `granny_pnt332_vertex` (the only layout the client asks for) |
| `GrannyGetMeshBytesPerIndex`, `GrannyGetMeshIndices` | 4, the reader's 32-bit indices |
| `GrannyCopyMeshIndices` | 1, 2 or 4 bytes per index |
| `GrannyGetMeshTriangleGroups` | the reader's groups as `granny_tri_material_group` |
| `GrannyNewMeshBinding`, `GrannyGetMeshBindingToBoneIndices`, `GrannyFreeMeshBinding` | the mesh's bone bindings matched by name to the target skeleton (exact, then ignoring case; an unmatched bone is counted as `granny.bone_unbound` and bound to bone 0) |
| `GrannyNewMeshDeformer`, `GrannyFreeMeshDeformer` | a record; PNT332 output only |
| `GrannyDeformVertices` | `bf_granny_deform`, the mesh found from its vertex array |
| `GrannyInstantiateModel`, `GrannyFreeModelInstance`, `GrannyGetSourceSkeleton` | an instance of the model; freeing it detaches its controls (the client frees them itself) |
| `GrannyPlayControlledAnimation` | `bf_granny_bind_animation` (the animation may come from another file); loop count 1, active |
| `GrannySetControlLoopCount`, `GrannySetControlActive`, `GrannyFreeControl` | the control |
| `GrannyGetControlDurationLeft` | `loops * duration - (clock - start)`; 10^30 for loop count 0 |
| `GrannySetModelClock` | the clock of the instance and its controls |
| `GrannySampleModelAnimations` | `bf_granny_sample_local` with the most recently started active control, its time looped by its loop count and held at the end once the loops are over; the rest pose without one |
| `GrannyBuildWorldPose` | `bf_granny_build_world` (offset as given: the client passes none) |
| `GrannyGetWorldPoseComposite4x4Array`, `GrannyGetWorldPoseComposite4x4` | the composite matrices |
| `GrannyFreeCompletedModelControls` | nothing: Granny frees only controls marked for completion, which the client never does (its finished attack control is asked for its duration afterwards) |
| `GrannyNewLocalPose`, `GrannyFreeLocalPose`, `GrannyNewWorldPose`, `GrannyFreeWorldPose` | the poses |
| `GrannyGetSystemSeconds`, `GrannyGetSecondsElapsed` | `mach_absolute_time` |
| `GrannyPNT332VertexType`, `GrannyRGBA8888PixelFormat` (data) | the layouts the adapter knows |

Counters (in the trace at quit when a category is on): `granny.files` read; failures `granny.file_refused`, `granny.texture_failed`, `granny.bone_unbound`, `granny.animation_unbound`, `granny.animation_no_tracks`, `granny.deform_unknown_vertices`, `granny.deform_count_mismatch`, `granny.index_truncated`.

## In the client

- `CGameMode::NewNpc` makes a `C3dGrannyGameActor` for a `.gr2` job name whose model loads (under `BF_PORT` it asks the resource manager first: `Hugeling90_6.gr2` and `dragon_5.gr2` are not in the archives, and the actor uses its model without a check), else a `CNpc` with the placeholder sprite. `MakeGrannyCorpse3d` makes the corpse again.
- `C3dGrannyModelRes::Load` no longer shows a box under `BF_PORT`: a missing file is reported by the resource manager (`res.missing`), a refused one as `res.unusable`, and the half-made resource is not used.
- Animation is the 2008 client's: the bone type is the digit after `_` in the file name; type 0 (Emperium) plays its own file's animation, 2 and 7 to 9 (treasure box, guardians) their `3dmob_bone` files. **The guild flag stays in its rest pose**, as in the 2008 client: its type is 1, and `C3dGrannyBoneRes::GetAnimation` loads nothing for type 1 (reference executable 0x436490, `if (param_1 != 1) AddBone(...)`); `WinMain` preloads the flag's animation as type 5, which no model uses. Making it sway would be a one-line departure under `BF_PORT` in `GetAnimation` (load type 1's stand animation from the model file); the adapter's test shows the animation works.

## Evidence in the client

`port/harness/scripts/granny.py` (M12): its own account at the Prontera fountain, the five guild flags drawn as banners on their stands, no `res.missing` for a `.gr2`; the Dream Metal (1846, the Emperium's model; rAthena refuses `@monster 1288`) and the three guardians spawned by a GM bot, drawn and animated, then their corpses; the Emperium in prtg_cas01 during a War of Emperium the bot starts and ends (`--param woe=0` leaves that out); `granny.files` counted and no adapter failure. Manifest `port/harness/manifests/m12-granny.sha256`.

## Open

- Blending between controls is not done (the client never needs it: it frees all controls before starting another).
- The guild emblem on the flag's second mesh comes from the client (`GetEmblemTexture`); the Prontera flags belong to no guild on the test server, so their emblem plane is empty. A guild-owned castle flag is untested.

## For port/README.md

> `port/compat/granny`: the 44 `Granny*` functions and 2 data names of granny2.dll on `port/native/formats/granny` (step M12.3a), so that `src/resource/Model.cpp`'s Granny resource classes build as in 2008. `bifrost-compat-granny-test` replays the client's load, animate and skin sequence on the game's 21 `.gr2` files (`skipped` without `data.grf`). In the client: `port/harness/scripts/granny.py` (guild flags, Emperium, guardians, corpses).
