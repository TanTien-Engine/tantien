#include "SdfPreview.h"

#include "modules/render/Render.h"

#include <GL/gl3w.h>

#include <unirender/Device.h>
#include <unirender/Context.h>
#include <unirender/ShaderProgram.h>
#include <unirender/Uniform.h>
#include <unirender/DrawState.h>
#include <unirender/TextureFormat.h>
#include <unirender/VertexLayoutType.h>
#include <unirender/PrimitiveType.h>

#include <shadertrans/ShaderTrans.h>

// OCCT
#include <STEPControl_Reader.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS.hxx>
#include <TopExp_Explorer.hxx>
#include <BRep_Tool.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <Poly_Triangulation.hxx>
#include <Poly_Triangle.hxx>
#include <TopLoc_Location.hxx>
#include <gp_Pnt.hxx>
#include <gp_Trsf.hxx>
#include <Bnd_Box.hxx>
#include <BRepBndLib.hxx>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <vector>

// One of the headers above pulls in <windows.h> (via gl3w / WGL), whose
// min / max macros clobber the std::min / std::max calls below -- MSVC
// reports "'(': illegal token on right side of '::'". Undo them for this
// translation unit.
#ifdef min
#  undef min
#endif
#ifdef max
#  undef max
#endif

namespace
{

using clk = std::chrono::steady_clock;
static double ms_since(const clk::time_point& t0)
{
    return std::chrono::duration<double, std::milli>(clk::now() - t0).count();
}

// ------------------------------------------------------------------ embedded shaders

static const char* kVert = R"GLSL(
#version 430
layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec2 a_uv;
out vec2 v_uv;
void main() {
    v_uv = a_uv;
    gl_Position = vec4(a_pos, 1.0);
}
)GLSL";

static const char* kFrag = R"GLSL(
#version 430
in vec2 v_uv;
out vec4 frag_color;

// This pipeline parses GLSL with Vulkan rules (glslang EShClientVulkan), which
// rejects non-opaque uniforms outside a block. So every scalar/vec uniform must
// live inside a `uniform UBO { ... };`; samplers stay as plain uniforms with an
// auto-mapped binding. Members are set by bare name (see draw_mesh_single.ves).
uniform sampler3D u_sdf;

uniform UBO
{
    vec3  u_bbmin;
    vec3  u_bbsize;
    vec3  u_eye;
    vec3  u_cam_f;
    vec3  u_cam_r;
    vec3  u_cam_u;
    vec2  u_tan;
    float u_voxel;
    int   u_csg;       // 0 union, 1 cut, 2 common
    vec3  u_tool_c;
    float u_tool_r;
    float u_tool_h;
    vec3  u_light_dir;
};

float sdf_body(vec3 p) {
    vec3 t = (p - u_bbmin) / u_bbsize;
    if (any(lessThan(t, vec3(0.0))) || any(greaterThan(t, vec3(1.0)))) {
        vec3 q = max(max(u_bbmin - p, p - (u_bbmin + u_bbsize)), vec3(0.0));
        return length(q) + 1.0e-3;
    }
    return texture(u_sdf, t).r;
}
float sdf_tool(vec3 p) {
    vec3 d = p - u_tool_c;
    float radial = length(d.xy) - u_tool_r;
    float axial  = abs(d.z) - u_tool_h;
    float outside = length(max(vec2(radial, axial), vec2(0.0)));
    float inside  = min(max(radial, axial), 0.0);
    return outside + inside;
}
float sdf_scene(vec3 p) {
    float a = sdf_body(p);
    float b = sdf_tool(p);
    if (u_csg == 1) return max(a, -b);   // cut
    if (u_csg == 2) return max(a,  b);   // common
    return min(a, b);                    // union
}
vec3 calc_normal(vec3 p) {
    float e = max(u_voxel * 0.5, 1e-4);
    vec2 h = vec2(e, 0.0);
    return normalize(vec3(
        sdf_scene(p + h.xyy) - sdf_scene(p - h.xyy),
        sdf_scene(p + h.yxy) - sdf_scene(p - h.yxy),
        sdf_scene(p + h.yyx) - sdf_scene(p - h.yyx)));
}
bool ray_box(vec3 ro, vec3 rd, vec3 bmin, vec3 bmax, out float t0, out float t1) {
    vec3 inv = 1.0 / rd;
    vec3 a = (bmin - ro) * inv;
    vec3 b = (bmax - ro) * inv;
    vec3 tmn = min(a, b);
    vec3 tmx = max(a, b);
    t0 = max(max(tmn.x, tmn.y), tmn.z);
    t1 = min(min(tmx.x, tmx.y), tmx.z);
    return t1 >= max(t0, 0.0);
}
void main() {
    vec2 ndc = v_uv * 2.0 - 1.0;
    vec3 rd = normalize(u_cam_f + ndc.x * u_tan.x * u_cam_r + ndc.y * u_tan.y * u_cam_u);
    vec3 ro = u_eye;

    vec3 bmin = u_bbmin - u_bbsize * 0.15;
    vec3 bmax = u_bbmin + u_bbsize * 1.15;
    float t0, t1;
    vec4 bg = vec4(0.10, 0.11, 0.13, 1.0);
    if (!ray_box(ro, rd, bmin, bmax, t0, t1)) { frag_color = bg; return; }

    float t = max(t0, 0.0);
    float eps = u_voxel * 0.25;
    bool hit = false;
    for (int i = 0; i < 256; ++i) {
        vec3 p = ro + rd * t;
        float d = sdf_scene(p);
        if (d < eps) { hit = true; break; }
        t += max(d * 0.9, eps);
        if (t > t1) break;
    }
    if (!hit) { frag_color = bg; return; }

    vec3 p = ro + rd * t;
    vec3 n = calc_normal(p);
    vec3 L = normalize(u_light_dir);
    float diff = max(dot(n, L), 0.0);
    vec3 base = vec3(0.72, 0.74, 0.80);
    vec3 col = base * (0.25 + diff * 0.85);
    col += vec3(0.06) * pow(1.0 - max(dot(n, -rd), 0.0), 2.0);
    frag_color = vec4(col, 1.0);
}
)GLSL";

// ------------------------------------------------------------------ 1D squared EDT
// Felzenszwalb & Huttenlocher exact distance transform on squared distances.
static void edt_1d(const float* f, float* d, int n, int* v, float* z)
{
    const float INF = 1e20f;
    int k = 0;
    v[0] = 0;
    z[0] = -INF;
    z[1] = +INF;
    for (int q = 1; q < n; ++q) {
        float s = ((f[q] + (float)q * q) - (f[v[k]] + (float)v[k] * v[k]))
                / (2.0f * q - 2.0f * v[k]);
        while (s <= z[k]) {
            --k;
            s = ((f[q] + (float)q * q) - (f[v[k]] + (float)v[k] * v[k]))
              / (2.0f * q - 2.0f * v[k]);
        }
        ++k;
        v[k] = q;
        z[k] = s;
        z[k + 1] = +INF;
    }
    k = 0;
    for (int q = 0; q < n; ++q) {
        while (z[k + 1] < (float)q) ++k;
        float dq = (float)(q - v[k]);
        d[q] = dq * dq + f[v[k]];
    }
}

} // anonymous namespace

namespace tt
{

bool SdfPreview::BuildFromStep(const std::string& step_path, int res)
{
    auto t0 = clk::now();
    STEPControl_Reader reader;
    if (reader.ReadFile(step_path.c_str()) != IFSelect_RetDone) {
        printf("[sdfprev] STEP read FAILED: %s\n", step_path.c_str());
        return false;
    }
    reader.TransferRoots();
    TopoDS_Shape shape = reader.OneShape();
    if (shape.IsNull()) {
        printf("[sdfprev] STEP produced a null shape\n");
        return false;
    }
    printf("[sdfprev] STEP read %.0f ms: %s\n", ms_since(t0), step_path.c_str());
    return BuildFromShape(shape, res);
}

bool SdfPreview::BuildFromShape(const TopoDS_Shape& shape, int res)
{
    auto t0 = clk::now();

    Bnd_Box box;
    BRepBndLib::Add(shape, box);
    if (box.IsVoid()) { printf("[sdfprev] empty bbox\n"); return false; }
    double xmin, ymin, zmin, xmax, ymax, zmax;
    box.Get(xmin, ymin, zmin, xmax, ymax, zmax);
    double diag = std::sqrt((xmax-xmin)*(xmax-xmin) + (ymax-ymin)*(ymax-ymin) + (zmax-zmin)*(zmax-zmin));
    double defl = std::max(diag * 1.0e-3, 0.05);

    // Mesh once (this is the only OCCT cost in the preview build).
    auto tm = clk::now();
    BRepMesh_IncrementalMesh mesher(shape, defl, Standard_False, 0.6, Standard_True);
    mesher.Perform();
    double mesh_ms = ms_since(tm);

    std::vector<float> tris; // x,y,z * 3 per triangle
    tris.reserve(1 << 18);
    for (TopExp_Explorer ex(shape, TopAbs_FACE); ex.More(); ex.Next()) {
        TopoDS_Face f = TopoDS::Face(ex.Current());
        TopLoc_Location loc;
        Handle(Poly_Triangulation) tri = BRep_Tool::Triangulation(f, loc);
        if (tri.IsNull()) continue;
        const gp_Trsf& trsf = loc.Transformation();
        const int nbt = tri->NbTriangles();
        for (int i = 1; i <= nbt; ++i) {
            int a, b, c;
            tri->Triangle(i).Get(a, b, c);
            gp_Pnt pa = tri->Node(a); pa.Transform(trsf);
            gp_Pnt pb = tri->Node(b); pb.Transform(trsf);
            gp_Pnt pc = tri->Node(c); pc.Transform(trsf);
            tris.push_back((float)pa.X()); tris.push_back((float)pa.Y()); tris.push_back((float)pa.Z());
            tris.push_back((float)pb.X()); tris.push_back((float)pb.Y()); tris.push_back((float)pb.Z());
            tris.push_back((float)pc.X()); tris.push_back((float)pc.Y()); tris.push_back((float)pc.Z());
        }
    }
    const size_t ntri = tris.size() / 9;
    printf("[sdfprev] mesh %.0f ms, %zu triangles (defl=%.3f, diag=%.1f)\n",
           mesh_ms, ntri, defl, diag);
    if (ntri == 0) { printf("[sdfprev] no triangles\n"); return false; }

    // ---- cubic padded grid ----
    res = std::max(8, std::min(res, 256));
    m_res = res;
    float cx = 0.5f * (float)(xmin + xmax);
    float cy = 0.5f * (float)(ymin + ymax);
    float cz = 0.5f * (float)(zmin + zmax);
    float half = 0.5f * (float)std::max(std::max(xmax-xmin, ymax-ymin), zmax-zmin);
    half *= 1.10f;                          // 10% padding
    half = std::max(half, (float)defl * 4); // avoid degenerate
    m_bbmin[0] = cx - half; m_bbmin[1] = cy - half; m_bbmin[2] = cz - half;
    m_bbmax[0] = cx + half; m_bbmax[1] = cy + half; m_bbmax[2] = cz + half;
    m_voxel = (2.0f * half) / (float)res;

    if (!BuildFromTriangles(tris, res)) return false;
    printf("[sdfprev] BUILD TOTAL %.0f ms (res=%d, voxel=%.3f)\n", ms_since(t0), res, m_voxel);
    return true;
}

bool SdfPreview::BuildFromTriangles(const std::vector<float>& tris, int res)
{
    const int N = res;
    const size_t N2 = (size_t)N * N;
    const size_t N3 = N2 * N;
    const float voxel = m_voxel;
    const float ox = m_bbmin[0], oy = m_bbmin[1], oz = m_bbmin[2];

    // ---- 1) inside/outside mask via +Z ray parity ----
    auto tp = clk::now();
    std::vector<std::vector<float>> col(N2);          // crossings per (i,j) column
    const size_t ntri = tris.size() / 9;
    for (size_t ti = 0; ti < ntri; ++ti) {
        const float* T = &tris[ti * 9];
        float ax = T[0], ay = T[1], az = T[2];
        float bx = T[3], by = T[4], bz = T[5];
        float cxp= T[6], cyp= T[7], cz2= T[8];
        float minx = std::min(ax, std::min(bx, cxp));
        float maxx = std::max(ax, std::max(bx, cxp));
        float miny = std::min(ay, std::min(by, cyp));
        float maxy = std::max(ay, std::max(by, cyp));
        int i0 = (int)std::floor((minx - ox) / voxel);
        int i1 = (int)std::floor((maxx - ox) / voxel);
        int j0 = (int)std::floor((miny - oy) / voxel);
        int j1 = (int)std::floor((maxy - oy) / voxel);
        i0 = std::max(i0, 0); i1 = std::min(i1, N - 1);
        j0 = std::max(j0, 0); j1 = std::min(j1, N - 1);
        if (i0 > i1 || j0 > j1) continue;
        // 2D barycentric setup (project to XY)
        float denom = (by - cyp) * (ax - cxp) + (cxp - bx) * (ay - cyp);
        if (std::fabs(denom) < 1e-12f) continue; // degenerate in XY
        float inv = 1.0f / denom;
        for (int i = i0; i <= i1; ++i) {
            float px = ox + (i + 0.5f) * voxel;
            for (int j = j0; j <= j1; ++j) {
                float py = oy + (j + 0.5f) * voxel;
                float la = ((by - cyp) * (px - cxp) + (cxp - bx) * (py - cyp)) * inv;
                float lb = ((cyp - ay) * (px - cxp) + (ax - cxp) * (py - cyp)) * inv;
                float lc = 1.0f - la - lb;
                if (la < -1e-5f || lb < -1e-5f || lc < -1e-5f) continue;
                float zh = la * az + lb * bz + lc * cz2;
                col[(size_t)j * N + i].push_back(zh);
            }
        }
    }
    std::vector<uint8_t> inside(N3, 0);
    size_t n_inside = 0;
    for (size_t cidx = 0; cidx < N2; ++cidx) {
        auto& cr = col[cidx];
        if (cr.empty()) continue;
        std::sort(cr.begin(), cr.end());
        int i = (int)(cidx % N);
        int j = (int)(cidx / N);
        for (int k = 0; k < N; ++k) {
            float zc = oz + (k + 0.5f) * voxel;
            // count crossings strictly below zc
            int cnt = (int)(std::lower_bound(cr.begin(), cr.end(), zc) - cr.begin());
            if (cnt & 1) { inside[((size_t)k * N + j) * N + i] = 1; ++n_inside; }
        }
    }
    double parity_ms = ms_since(tp);

    // ---- 2) boundary seed + separable squared EDT ----
    auto te = clk::now();
    const float INF = 1e20f;
    std::vector<float> g(N3, INF);
    // NB: 'IN' is a Windows SAL macro (minwindef.h), so the lambda is named 'at_in'.
    auto at_in = [&](int x, int y, int z) -> uint8_t {
        if (x < 0 || y < 0 || z < 0 || x >= N || y >= N || z >= N) return 0; // outside
        return inside[((size_t)z * N + y) * N + x];
    };
    for (int z = 0; z < N; ++z)
    for (int y = 0; y < N; ++y)
    for (int x = 0; x < N; ++x) {
        uint8_t s = at_in(x, y, z);
        // boundary cell = differs from any 6-neighbour
        if (s != at_in(x-1,y,z) || s != at_in(x+1,y,z) ||
            s != at_in(x,y-1,z) || s != at_in(x,y+1,z) ||
            s != at_in(x,y,z-1) || s != at_in(x,y,z+1)) {
            g[((size_t)z * N + y) * N + x] = 0.0f;
        }
    }
    // separable transform: reuse temp line buffers
    std::vector<float> fline(N), dline(N);
    std::vector<int>   vbuf(N);
    std::vector<float> zbuf(N + 1);
    // X
    for (int z = 0; z < N; ++z)
    for (int y = 0; y < N; ++y) {
        size_t base = ((size_t)z * N + y) * N;
        for (int x = 0; x < N; ++x) fline[x] = g[base + x];
        edt_1d(fline.data(), dline.data(), N, vbuf.data(), zbuf.data());
        for (int x = 0; x < N; ++x) g[base + x] = dline[x];
    }
    // Y
    for (int z = 0; z < N; ++z)
    for (int x = 0; x < N; ++x) {
        for (int y = 0; y < N; ++y) fline[y] = g[((size_t)z * N + y) * N + x];
        edt_1d(fline.data(), dline.data(), N, vbuf.data(), zbuf.data());
        for (int y = 0; y < N; ++y) g[((size_t)z * N + y) * N + x] = dline[y];
    }
    // Z
    for (int y = 0; y < N; ++y)
    for (int x = 0; x < N; ++x) {
        for (int z = 0; z < N; ++z) fline[z] = g[((size_t)z * N + y) * N + x];
        edt_1d(fline.data(), dline.data(), N, vbuf.data(), zbuf.data());
        for (int z = 0; z < N; ++z) g[((size_t)z * N + y) * N + x] = dline[z];
    }
    // signed distance in world units, in x-fastest order for glTexImage3D
    std::vector<float> sdf(N3);
    float dmin = INF, dmax = -INF;
    for (size_t idx = 0; idx < N3; ++idx) {
        float d = std::sqrt(g[idx]) * voxel;
        if (inside[idx]) d = -d;
        sdf[idx] = d;
        dmin = std::min(dmin, d); dmax = std::max(dmax, d);
    }
    double edt_ms = ms_since(te);

    // ---- 3) upload r32f 3D texture ----
    auto tu = clk::now();
    auto dev = tt::Render::Instance()->Device();
    m_sdf_tex = dev->CreateTexture3D((size_t)N, (size_t)N, (size_t)N,
                                     ur::TextureFormat::R32F, sdf.data(), sdf.size() * sizeof(float));
    m_sampler = dev->GetTextureSampler(ur::Device::TextureSamplerType::LinearClamp);
    double up_ms = ms_since(tu);

    printf("[sdfprev] voxelize: parity %.0f ms, edt %.0f ms, upload %.0f ms | inside=%.1f%% sdf[%.2f,%.2f]\n",
           parity_ms, edt_ms, up_ms, 100.0 * (double)n_inside / (double)N3, dmin, dmax);

    if (!m_sdf_tex) { printf("[sdfprev] 3D texture creation FAILED\n"); return false; }
    if (!BuildProgram()) return false;
    m_ready = true;
    return true;
}

bool SdfPreview::BuildProgram()
{
    if (m_prog) return true;
    std::vector<unsigned int> vs, fs;
    shadertrans::ShaderTrans::GLSL2SpirV(shadertrans::ShaderStage::VertexShader, kVert, "", vs);
    shadertrans::ShaderTrans::GLSL2SpirV(shadertrans::ShaderStage::PixelShader,  kFrag, "", fs);
    if (vs.empty() || fs.empty()) { printf("[sdfprev] shader GLSL->SPIRV FAILED\n"); return false; }
    m_prog = tt::Render::Instance()->Device()->CreateShaderProgram(vs, fs);
    if (!m_prog) { printf("[sdfprev] shader program link FAILED\n"); return false; }
    return true;
}

void SdfPreview::Render(int screen_w, int screen_h, float anim)
{
    if (!m_ready || !m_prog || !m_sdf_tex) return;

    auto ctx = tt::Render::Instance()->Context();

    // ---- auto-framed camera around the grid box ----
    float cx = 0.5f * (m_bbmin[0] + m_bbmax[0]);
    float cy = 0.5f * (m_bbmin[1] + m_bbmax[1]);
    float cz = 0.5f * (m_bbmin[2] + m_bbmax[2]);
    float sx = m_bbmax[0] - m_bbmin[0];
    float sy = m_bbmax[1] - m_bbmin[1];
    float sz = m_bbmax[2] - m_bbmin[2];
    float radius = 0.5f * std::sqrt(sx*sx + sy*sy + sz*sz);
    const float fov = 35.0f * 3.14159265f / 180.0f;
    float dist = radius / std::tan(fov * 0.5f) * 1.25f;
    // slowly orbit so we can see it's live
    float ang = anim * 0.01f;
    float dirx = std::cos(ang) * 0.7f, diry = std::sin(ang) * 0.7f, dirz = 0.55f;
    float dl = std::sqrt(dirx*dirx + diry*diry + dirz*dirz);
    dirx/=dl; diry/=dl; dirz/=dl;
    float eye[3] = { cx + dirx*dist, cy + diry*dist, cz + dirz*dist };
    // camera basis
    float fx = cx-eye[0], fy = cy-eye[1], fz = cz-eye[2];
    float fl = std::sqrt(fx*fx+fy*fy+fz*fz); fx/=fl; fy/=fl; fz/=fl;
    float up[3] = {0,0,1};
    // r = f x up
    float rx = fy*up[2]-fz*up[1], ry = fz*up[0]-fx*up[2], rz = fx*up[1]-fy*up[0];
    float rl = std::sqrt(rx*rx+ry*ry+rz*rz); rx/=rl; ry/=rl; rz/=rl;
    // u = r x f
    float ux = ry*fz-rz*fy, uy = rz*fx-rx*fz, uz = rx*fy-ry*fx;
    float aspect = (screen_h > 0) ? (float)screen_w / (float)screen_h : 1.0f;
    float tanh = std::tan(fov * 0.5f);
    float tanv[2] = { tanh * aspect, tanh };

    float bbmin[3] = { m_bbmin[0], m_bbmin[1], m_bbmin[2] };
    float bbsize[3] = { sx, sy, sz };
    float camf[3] = {fx,fy,fz}, camr[3] = {rx,ry,rz}, camu[3] = {ux,uy,uz};
    float light[3] = { 0.4f, 0.5f, 1.0f };

    // ---- synthetic animated tool: a cylinder cut drilling through Z ----
    float minext = std::min(std::min(sx, sy), sz);
    float toolr = 0.10f * minext * (0.7f + 0.5f * (0.5f + 0.5f * std::sin(anim * 0.05f)));
    float toolc[3] = { cx + 0.22f * sx * std::sin(anim * 0.03f), cy, cz };
    float toolh = sz; // through-hole
    int csg = 1;      // cut

    auto setv = [&](const char* nm, const float* v, int n) {
        auto u = m_prog->QueryUniform(nm);
        if (u) u->SetValue(v, n);
    };
    auto seti = [&](const char* nm, int iv) {
        auto u = m_prog->QueryUniform(nm);
        if (u) u->SetValue(&iv, 1);
    };

    setv("u_bbmin", bbmin, 3);
    setv("u_bbsize", bbsize, 3);
    setv("u_eye", eye, 3);
    setv("u_cam_f", camf, 3);
    setv("u_cam_r", camr, 3);
    setv("u_cam_u", camu, 3);
    setv("u_tan", tanv, 2);
    float voxel = m_voxel; setv("u_voxel", &voxel, 1);
    seti("u_csg", csg);
    setv("u_tool_c", toolc, 3);
    setv("u_tool_r", &toolr, 1);
    setv("u_tool_h", &toolh, 1);
    setv("u_light_dir", light, 3);

    int slot = m_prog->QueryTexSlot("u_sdf");
    if (slot < 0) slot = 0;
    ctx->SetTexture(slot, m_sdf_tex);
    ctx->SetTextureSampler(slot, m_sampler);

    ur::DrawState ds;
    ds.program = m_prog;
    ds.vertex_array = tt::Render::Instance()->Device()->GetVertexArray(
        ur::Device::PrimitiveType::Quad, ur::VertexLayoutType::PosTex, false);
    ds.render_state.depth_test.enabled = false;
    ds.render_state.depth_mask = false;

    glFinish();
    auto t0 = clk::now();
    ctx->Draw(ur::PrimitiveType::TriangleStrip, ds, nullptr);
    glFinish();
    double frame_ms = ms_since(t0);

    if ((m_frame % 60) == 0) {
        printf("[sdfprev] raymarch %.2f ms/frame (%dx%d)\n", frame_ms, screen_w, screen_h);
    }
    ++m_frame;
}

void SdfPreview::DumpBMP(const std::string& path, int x, int y, int w, int h)
{
    std::vector<uint8_t> rgba((size_t)w * h * 4);
    glFinish();
    glReadPixels(x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());

    const int row = w * 3;
    const int pad = (4 - (row % 4)) % 4;
    const int img = (row + pad) * h;
    const int total = 54 + img;

    std::vector<uint8_t> bmp(total, 0);
    auto p16 = [&](int off, uint16_t v){ bmp[off]=v&0xff; bmp[off+1]=(v>>8)&0xff; };
    auto p32 = [&](int off, uint32_t v){ bmp[off]=v&0xff; bmp[off+1]=(v>>8)&0xff; bmp[off+2]=(v>>16)&0xff; bmp[off+3]=(v>>24)&0xff; };
    bmp[0]='B'; bmp[1]='M';
    p32(2, total); p32(10, 54);
    p32(14, 40); p32(18, w); p32(22, h);
    p16(26, 1); p16(28, 24); p32(34, img);
    // GL framebuffer origin is bottom-left; BMP rows are bottom-up -> no flip.
    int o = 54;
    for (int yy = 0; yy < h; ++yy) {
        for (int xx = 0; xx < w; ++xx) {
            size_t s = ((size_t)yy * w + xx) * 4;
            bmp[o++] = rgba[s + 2]; // B
            bmp[o++] = rgba[s + 1]; // G
            bmp[o++] = rgba[s + 0]; // R
        }
        o += pad;
    }
    FILE* fp = fopen(path.c_str(), "wb");
    if (fp) { fwrite(bmp.data(), 1, bmp.size(), fp); fclose(fp); printf("[sdfprev] wrote %s\n", path.c_str()); }
    else    { printf("[sdfprev] could not open %s for write\n", path.c_str()); }
}

} // namespace tt
