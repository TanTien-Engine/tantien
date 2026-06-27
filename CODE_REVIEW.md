# Code Review — tantien-cad (2026-06-28)

Full review of the C++ layer (`src/**`, ~16k LOC) plus the blueprint engine in
`src/script/blueprint/**`. Focus areas: memory/leaks, performance, boundary
conditions, and architecture.

Architecture in one line: a CAD node-graph editor on a self-built scripting VM
(**Vessel**, `.ves` + `ves_*` C API). C++ modules are exposed to scripts through a
`wrap_*.cpp` binding layer (the highest-risk surface), and the bulk of app logic
lives in 500+ `.ves` scripts.

## Status

Three fix commits on `B-cad` (all C++ compile-verified by building the `tantien`
static-lib + `editor` targets):

| Commit | Scope | Files |
| --- | --- | --- |
| `80957dff` | HIGH + safe MED | 15 |
| `b41ca290` | remaining MED | 5 |
| `7eee568e` | performance | 4 |

`✅ Fixed` = landed in one of the above. `⏳ Pending` = analysed, intentionally not
changed yet (see *Pending* at the end for why).

---

## 1. Memory & performance

### Memory (HIGH)
- ✅ **Allocator mismatch on image load** — `wrap_Image.cpp:126`. `gimg_import()` returns `malloc`'d memory (stb_image), but finalize freed it with `delete[]` (UB on the most common image-load path). Fixed: `ImageData` now records the allocator (`pixels_malloc`) and frees with the matching deallocator; `to_rgb565` and `Texture2D.download` follow suit.
- ✅ **`TopoShape` foreign never finalized** — `wrap_OM.cpp`. The class had no `finalize` registered, so every `clip`/`box`/`get_shape` leaked a `shared_ptr<TopoShape>` (BRep/GPU data) for the process lifetime. Fixed: registered allocate+finalize.
- ✅ **GPU readback heap overflow** — `wrap_Render.cpp:1107`. `GetComputeBufferData(data.data(), sizeof(data) * size)` used `sizeof(std::vector)` instead of `sizeof(int)` → wrote ~6–8× past the buffer. Fixed.
- ✅ **`Texture2D.download` buffer leak** — `wrap_Render.cpp`. Overwrote `img->pixels` without freeing the prior buffer. Fixed (allocator-aware free first).

### Memory (Pending)
- ⏳ **Unbounded glyph-style cache** — `GTxt.cpp:144,197`. `GLYPH_HASH` / `NEXT_GLPHY_ID` grow forever, and `line_x` is mixed into the hash but ignored by `glyph_style_eq`, so rotated-gradient text creates a new atlas entry per glyph position. Bounded in practice for normal labels.
- ⏳ **`r16f` ImageData under-allocation** — `wrap_Image.cpp:116`. `get_format_channels(GPF_R16)` returns 1, but R16 is 2 bytes/px, so the numeric constructor under-allocates by half. (The `to_rgb565` OOB *read* on 1-channel images was fixed; the allocation size was not.)

### Performance
- ✅ **`Blueprint.topo_sort` O(N²·E) → O(N+E)** — `blueprint.ves`. Scanned all nodes per edge to resolve `conn.from.node`'s index (hung the editor on large parts). Now uses a per-node scratch `sort_idx` (Vessel `Map` is string-key-only) with an identity check; output is identical. *Needs a runtime load+save sanity check — not runtime-tested here.*
- ✅ **Missing-glyph re-rasterization every frame** — `LoadingList.cpp`. A glyph with no bitmap was re-queued and re-rasterized by freetype every frame, forever. Fixed with a negative cache.
- ⏳ **`editor.ves` save is O(N²)~O(N³)** — `editor.ves:1043,1074,1105` plus `query_node_idx`. Inline full-array scans per connection (no `break`). Infrequent (Ctrl+S), so left for a follow-up.
- ⏳ **`Node.update` per-frame foreign alloc** — `node.ves:80`. Allocates a `Float2` per node per frame just to detect movement; runs every frame (update isn't gated by render-on-demand).
- ⏳ **`RTreeUpdate::RollForward` full re-serialize** per touched poly — `RTreeUpdate.cpp`.

---

## 2. Boundary conditions

- ✅ **`BRepFromByteArray` had zero bounds checks** — `BRepSerialize.cpp`. Counts were read from the (possibly corrupt/truncated/disk) buffer and used to walk a pointer with no length. Fixed: length-aware + every read bounds-checked, returns `nullptr` on bad data; all 4 callers pass `len` and skip null.
- ✅ **`query_with_time` out-of-bounds write** — `wrap_DB.cpp:229`. Wrote `Region[3]` into a `double[DIMENSION]` with `DIMENSION==3`. Fixed: removed the OOB writes (the 4D-time feature was non-functional anyway — `Point` drops the 4th coord); query degrades to 3D.
- ✅ **SceneTree 16-bit index overflow** — `SceneTree.cpp:122`. `unsigned short` indices wrapped past 65535 verts → garbage geometry (same class as renderer commit `14a58cfa`). Fixed: 32-bit indices + `UnsignedInt` index buffer.
- ✅ **`list_to_array` indexed without size check** — `wrap_Graphics.cpp` (add_line/rect/rect_filled/capsule/regional), `wrap_Maths.cpp` (get_line_intersect_line). Short/non-list args → heap OOB read. Fixed (size guards; `GD_ASSERT` is a release no-op).
- ✅ **`exit(1)` on a bad OBJ** — `RTreeBuilder.cpp:46`. A library call killed the whole app. Fixed (log + return); also gave each record a unique id (was all 0) and skip empty-point shapes (poison AABB).
- ✅ **Port-cycle infinite loop** — `execute_list.ves`. `build_list` walked the port chain with no visited guard → hung on a malformed graph. Fixed (Set guard).
- ✅ **Fixed-buffer overflows** — `main.cpp` (`sprintf`/`strcat` → `snprintf`).
- ✅ **Decompiler begin/end unvalidated** — `wrap_VM.cpp`. Clamped into `[0, code size]`.
- ✅ **`recursive_directory_iterator` could throw across the VM C boundary** — `wrap_Filesystem.cpp`. Switched to the `error_code` overload.
- ✅ **`to_shader_stage` returned an uninitialized value** for unknown input — `wrap_Shader.cpp`. Defaulted.
- ✅ **`write_int` type confusion / `BRepToByteArray` size truncation** — `wrap_VM.cpp`, `BRepSerialize.cpp`. Fixed.
- ⏳ **Systemic unchecked `ves_toforeign` / `ves_tostring`** — most `wrap_*` functions deref args without a null/type check; a script can crash any of them. Only the specific OOB-indexing spots were fixed; the broad sweep (a guard macro) is a separate pass.
- ⏳ **`read_module_complete` leak-list mismatch** — `main.cpp:133`. 8 module names are in the don't-free list but aren't returned as static sources, so heap-loaded ones leak once each (latent: those module files likely don't exist).

---

## 3. Architecture (recommendations)

- **Binding-layer registry.** Module registration is hand-written in 4 places (`read_module`, `read_module_complete`, `bind_foreign_class`, `bind_foreign_method`); the leak above is exactly this drift. Replace with a single registry table.
- **Serialization format hardening.** The B-rep byte stream has no magic/version/length/checksum. Add a header and keep `BRepFromByteArray` length-aware (done) so a bad file degrades gracefully instead of crashing.
- **Resolve the 4D-time API.** `RTree.insert/query_with_time` is built on a 3D index. Either make the index 4D or remove the API; today it silently ignores time.
- **Unify the 16-bit index strategy.** Renderer and SceneTree each had `unsigned short` truncation. Standardise on 32-bit indices or a shared "flush/split before overflow" helper.
- **Contain global mutable state** — `ubo_vs` (multi-instance hazard), `GLYPH_*`, `NEXT_ID` (collides on disk reload). Move into instances.
- **Make `update` dirty-driven** like `draw` (render-on-demand gates draw but not the per-node update work).

---

## Pending follow-ups (priority order)
1. Runtime-verify the `topo_sort` `.ves` change (load + save a real graph).
2. Systemic `ves_toforeign`/`ves_tostring` null/type guards (macro).
3. `editor.ves` save O(N²) and `Node.update` per-frame alloc.
4. `r16f` ImageData allocation size; GTxt glyph-style cache bound.
5. Architecture refactors above.

## Confirmed NOT bugs (so they aren't re-chased)
- The placement-assign-into-`ves_set_newforeign` pattern is safe — the VM zeroes the block.
- `wrap_Maths.cpp:391` `memcpy(mt->x, m, sizeof(m))` is correct (`m` is a 16-float stack array, not a pointer).
- gtxt glyph **bitmap** buffers are owned by a bounded LRU in `thirdparty/gtxt` — correctly not freed by the caller.
- `GTxt.cpp:203` `% GLPHY_HASH_CAP` is computed in `size_t` before the `int` assignment — never negative, not an OOB.
