#pragma once

// GPU voxel/SDF interactive-preview prototype (Phase 0 feasibility).
//
// Goal: prove that a committed B-rep body can be voxelized into a signed
// distance field and a CSG preview (body op tool) raymarched at interactive
// rates, as a *display-only* substitute for the exact OCCT boolean while the
// user drags a parameter. This module is pure additive and env-flag gated
// (CAX_GPU_PREVIEW); it never feeds back into the modeling graph, never
// touches topo-naming, and is never a committed result.
//
// Pipeline (one-time per body, at "drag start"):
//   STEP/B-rep -> OCCT triangulation -> CPU voxelization (parity inside test
//   + separable Euclidean distance transform) -> r32f sampler3D.
// Per frame (the interactive part):
//   fullscreen raymarch sampling the body SDF + an analytic tool SDF, combined
//   via CSG min/max, phong shaded.
//
// The voxelization is CPU for this first cut deliberately: it keeps the hard,
// hard-to-debug-headless part in code we can reason about, and isolates the
// GPU path to a straightforward raymarch. If the one-time voxelization proves
// to be the bottleneck, the documented next step is a GPU JFA voxelizer.

#include <memory>
#include <string>
#include <vector>

class TopoDS_Shape;
namespace ur { class Texture; class ShaderProgram; class TextureSampler; }

namespace tt
{

enum class SdfCsgOp { Union = 0, Cut = 1, Common = 2 };

class SdfPreview
{
public:
    // One-time build from a STEP file (decoupled from the editor graph). res is
    // the cubic grid resolution (e.g. 64/128/256). Returns false on failure.
    bool BuildFromStep(const std::string& step_path, int res);
    // One-time build from an already-evaluated body (Phase-1 reuse path).
    bool BuildFromShape(const TopoDS_Shape& shape, int res);

    bool IsReady() const { return m_ready; }

    // Draw the raymarched CSG preview over the current framebuffer. `anim` (any
    // monotonically increasing value, e.g. frame count) drives the synthetic
    // tool so we can watch per-frame interactivity. Logs the per-frame GPU time
    // once every ~60 calls.
    void Render(int screen_w, int screen_h, float anim);

    // glReadPixels the current framebuffer to a 24-bit BMP (reliable headless
    // visual capture, no image lib needed).
    static void DumpBMP(const std::string& path, int x, int y, int w, int h);

private:
    bool BuildFromTriangles(const std::vector<float>& tris /*xyz*3 per tri*/, int res);
    bool BuildProgram();

private:
    bool m_ready = false;
    int  m_res   = 0;

    // Padded cubic grid in world space.
    float m_bbmin[3] = {0,0,0};
    float m_bbmax[3] = {0,0,0};
    float m_voxel    = 0.0f;   // uniform voxel size (world units)

    std::shared_ptr<ur::Texture>        m_sdf_tex;
    std::shared_ptr<ur::TextureSampler> m_sampler;
    std::shared_ptr<ur::ShaderProgram>  m_prog;

    int m_frame = 0;
};

}
