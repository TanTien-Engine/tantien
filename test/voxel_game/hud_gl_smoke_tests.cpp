#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "modules/graphics/DTex.h"
#include "voxel_game/ui/GtxtHud.h"

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include <dtex/TexRenderer.h>
#include <dtex/TextureBuffer.h>

#include <unirender/AccessType.h>
#include <unirender/ClearState.h>
#include <unirender/Context.h>
#include <unirender/Device.h>
#include <unirender/Factory.h>
#include <unirender/Framebuffer.h>
#include <unirender/Texture.h>
#include <unirender/TextureDescription.h>
#include <unirender/opengl/Context.h>
#include <unirender/opengl/Device.h>

#include <chrono>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace
{

using tt::voxel_game::GtxtHud;
using tt::voxel_game::HudStatus;
using tt::voxel_game::HudStyle;

void require(bool condition, const char* message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

void require_gl(const char* where)
{
    const GLenum err = glGetError();
    if (err != GL_NO_ERROR)
    {
        throw std::runtime_error(std::string(where) + " GL error " + std::to_string(err));
    }
}

bool file_readable(const char* path)
{
    std::ifstream stream(path, std::ios::binary);
    return static_cast<bool>(stream);
}

const char* pick_font()
{
    if (file_readable("assets/fonts/hkljhw8.ttf"))
    {
        return "assets/fonts/hkljhw8.ttf";
    }
    if (file_readable("assets/fonts/default2.ttf"))
    {
        return "assets/fonts/default2.ttf";
    }
    return nullptr;
}

int count_text_pixels(int width, int height)
{
    std::vector<unsigned char> pixels(static_cast<std::size_t>(width * height * 4));
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    int lit = 0;
    for (std::size_t i = 0; i < pixels.size(); i += 4)
    {
        if (pixels[i] > 40 || pixels[i + 1] > 40 || pixels[i + 2] > 40)
        {
            ++lit;
        }
    }
    return lit;
}

} // namespace

int main()
{
    try
    {
        if (glfwInit() == GLFW_FALSE)
        {
            throw std::runtime_error("glfwInit failed");
        }
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        GLFWwindow* window = glfwCreateWindow(320, 180, "hud_gl_smoke", nullptr, nullptr);
        require(window != nullptr, "glfwCreateWindow");
        glfwMakeContextCurrent(window);
        require(gl3wInit() == 0, "gl3wInit");

        std::shared_ptr<ur::Device> device = std::make_shared<ur::opengl::Device>();
        require(static_cast<bool>(device), "OpenGL device");
        device->Init();

        constexpr GLint initial_viewport_x = 7;
        constexpr GLint initial_viewport_y = 11;
        constexpr GLsizei initial_viewport_w = 173;
        constexpr GLsizei initial_viewport_h = 101;
        glViewport(initial_viewport_x, initial_viewport_y,
                   initial_viewport_w, initial_viewport_h);
        std::shared_ptr<ur::Context> context = std::make_shared<ur::opengl::Context>(*device);
        require(static_cast<bool>(context), "OpenGL context");
        ur::ClearState initial_clear;
        initial_clear.color.FromRGBA(0x000000ff);
        context->Clear(initial_clear);
        GLint viewport_after_initial_clear[4] = {};
        glGetIntegerv(GL_VIEWPORT, viewport_after_initial_clear);
        require(viewport_after_initial_clear[0] == initial_viewport_x &&
                    viewport_after_initial_clear[1] == initial_viewport_y &&
                    viewport_after_initial_clear[2] == initial_viewport_w &&
                    viewport_after_initial_clear[3] == initial_viewport_h,
                "first Clear preserves the pre-context viewport without SetViewport");
        require_gl("initial viewport preservation");

        for (int i = 0; i < 100; ++i)
        {
            GtxtHud hud;
            require(hud.init(*device, "no_such_font.ttf") == HudStatus::FontLoadFailed,
                    "invalid font init");
            hud.shutdown();
        }

        const char* font_path = pick_font();
        require(font_path != nullptr, "need assets/fonts for HUD GL smoke");

        glfwMakeContextCurrent(nullptr);
        {
            GtxtHud no_ctx;
            require(no_ctx.init(*device, font_path) == HudStatus::InvalidArgument,
                    "init without current GL context is rejected");
        }
        glfwMakeContextCurrent(window);

        GtxtHud hud;
        for (int i = 0; i < 3; ++i)
        {
            GtxtHud repeat;
            require(repeat.init(*device, font_path) == HudStatus::Ok, "repeat valid init");
            require(repeat.shutdown() == HudStatus::Ok, "repeat valid shutdown");
        }

        require(hud.init(*device, font_path) == HudStatus::Ok, "valid font after 100 failures");

        {
            dtex::TexRenderer rd(*device);
            dtex::TextureBuffer buf(*device, 1024, 1024);
            ur::TextureDescription stamp;
            stamp.width = 16;
            stamp.height = 16;
            stamp.format = ur::TextureFormat::RGBA8;
            std::vector<unsigned char> pixels(16 * 16 * 4, 255);
            auto tex = device->CreateTexture(stamp, pixels.data());
            require(static_cast<bool>(tex) && tex->GetWidth() == 16, "stamp texture");
            dtex::Rect r;
            r.xmin = 0;
            r.ymin = 0;
            r.xmax = 16;
            r.ymax = 16;
            require(!buf.Load(nullptr, r, 99), "null symbol is rejected");
            dtex::Rect huge;
            huge.xmin = 0;
            huge.ymin = 0;
            huge.xmax = 600;
            huge.ymax = 600;
            require(!buf.Load(tex, huge, 100), "oversized symbol is rejected");

            rd.FailNextFlush(1);
            buf.LoadStart();
            require(buf.Load(tex, r, 1), "load A");
            require(!buf.LoadFinish(*context, rd), "injected fail after A");
            int block_id = -1;
            require(buf.Query(1, block_id) == nullptr, "A is not queryable before success");

            buf.LoadStart();
            require(buf.Load(tex, r, 2), "load B while A unpublished");
            require(buf.LoadFinish(*context, rd), "retry publishes A and B");
            require(buf.Query(1, block_id) != nullptr, "A published after retry");
            require(buf.Query(2, block_id) != nullptr, "B published after retry");
        }

        {
            dtex::TexRenderer rd(*device);
            dtex::TextureBuffer buf(*device, 1024, 1024);
            ur::TextureDescription page;
            page.width = 512;
            page.height = 512;
            page.format = ur::TextureFormat::RGBA8;
            std::vector<unsigned char> pixels(512 * 512 * 4, 255);
            auto tex = device->CreateTexture(page, pixels.data());
            require(static_cast<bool>(tex) && tex->GetWidth() == 512, "block texture");
            dtex::Rect r;
            r.xmin = 0;
            r.ymin = 0;
            r.xmax = 512;
            r.ymax = 512;

            buf.LoadStart();
            for (uint64_t key = 1; key <= 5; ++key)
            {
                require(buf.Load(tex, r, key), "load full-block");
            }
            const auto batch_t0 = std::chrono::steady_clock::now();
            require(buf.LoadFinish(*context, rd), "five-block batch returns");
            const auto batch_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                      std::chrono::steady_clock::now() - batch_t0)
                                      .count();
            require(batch_ms < 2000, "over-capacity batch is bounded");
            int visible = 0;
            int block_id = -1;
            for (uint64_t key = 1; key <= 5; ++key)
            {
                if (buf.Query(key, block_id))
                {
                    ++visible;
                }
            }
            require(visible == 4, "one full-block is deferred");
            require(buf.Query(5, block_id) == nullptr, "fifth stays unpublished in the first batch");

            buf.LoadStart();
            require(buf.LoadFinish(*context, rd), "deferred key publishes without Load(5)");
            require(buf.Query(5, block_id) != nullptr, "deferred key 5 auto-published");
            int old_visible = 0;
            for (uint64_t key = 1; key <= 4; ++key)
            {
                if (buf.Query(key, block_id))
                {
                    ++old_visible;
                }
            }
            require(old_visible == 3, "auto-publish evicts exactly one old key");
        }

        {
            dtex::TexRenderer rd(*device);
            dtex::TextureBuffer buf(*device, 1024, 1024);
            ur::TextureDescription page;
            page.width = 512;
            page.height = 512;
            page.format = ur::TextureFormat::RGBA8;
            std::vector<unsigned char> pixels(512 * 512 * 4, 255);
            auto tex = device->CreateTexture(page, pixels.data());
            dtex::Rect r;
            r.xmin = 0;
            r.ymin = 0;
            r.xmax = 512;
            r.ymax = 512;
            for (uint64_t key = 1; key <= 4; ++key)
            {
                buf.LoadStart();
                require(buf.Load(tex, r, key), "commit block");
                require(buf.LoadFinish(*context, rd), "commit block flush");
            }
            int block_id = -1;
            for (uint64_t key = 1; key <= 4; ++key)
            {
                require(buf.Query(key, block_id) != nullptr, "old key committed");
            }

            rd.FailNextFlush(1);
            buf.LoadStart();
            require(buf.Load(tex, r, 5), "load fifth onto full atlas");
            const auto fail_t0 = std::chrono::steady_clock::now();
            require(!buf.LoadFinish(*context, rd), "fifth flush injected fail");
            const auto fail_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                     std::chrono::steady_clock::now() - fail_t0)
                                     .count();
            require(fail_ms < 2000, "fifth fail returns in time");
            for (uint64_t key = 1; key <= 4; ++key)
            {
                require(buf.Query(key, block_id) != nullptr, "old key still queryable after failed eviction");
            }
            require(buf.Query(5, block_id) == nullptr, "fifth not queryable after fail");

            buf.LoadStart();
            require(buf.LoadFinish(*context, rd), "retry eviction succeeds");
            require(buf.Query(5, block_id) != nullptr, "fifth published after retry");
            int old_visible = 0;
            for (uint64_t key = 1; key <= 4; ++key)
            {
                if (buf.Query(key, block_id))
                {
                    ++old_visible;
                }
            }
            require(old_visible == 3, "retry commits exactly one eviction");
        }
        require(!tt::DTex::Instance()->LoadSymbol(0xdeadull, nullptr, sm::irect(0, 0, 8, 8)),
                "DTex null symbol fails");

        int fb_w = 0;
        int fb_h = 0;
        glfwGetFramebufferSize(window, &fb_w, &fb_h);
        require(hud.resize(fb_w, fb_h) == HudStatus::Ok, "resize");

        ur::ClearState clear;
        clear.color.FromRGBA(0x000020ff);

        HudStyle style;
        style.font_size_px = 24;
        style.color = {1.0f, 1.0f, 1.0f, 1.0f};

        require(hud.begin_frame(*context) == HudStatus::Ok, "prewarm begin");
        require(hud.prewarm_utf8("HUD 0123456789", style) == HudStatus::Ok, "prewarm");
        require(hud.flush(*context) == HudStatus::Ok, "prewarm-only flush");
        require(context->GetFramebuffer() == nullptr, "default FBO after prewarm");
        require_gl("prewarm");

        ur::TextureDescription desc;
        desc.width = fb_w;
        desc.height = fb_h;
        desc.format = ur::TextureFormat::RGBA8;
        auto fbo_color = device->CreateTexture(desc, nullptr);
        auto fbo = device->CreateFramebuffer();
        fbo->SetAttachment(ur::AttachmentType::Color0, ur::TextureTarget::Texture2D, fbo_color, nullptr);
        context->SetFramebuffer(fbo);
        context->CommitFramebuffer();
        context->SetViewport(4, 6, fb_w - 8, fb_h - 12);
        context->CommitViewport();
        GLint draw_fbo = 0;
        GLint read_fbo = 0;
        GLint vp[4] = {};
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &draw_fbo);
        glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &read_fbo);
        glGetIntegerv(GL_VIEWPORT, vp);
        require(hud.begin_frame(*context) == HudStatus::Ok, "fbo begin");
        require(hud.prewarm_utf8("FirstUpload", style) == HudStatus::Ok, "fbo first glyphs");
        require(hud.flush(*context) == HudStatus::Ok, "fbo first upload");
        require(context->GetFramebuffer() == fbo, "FBO kept after first glyph upload");
        GLint draw_after = 0;
        GLint read_after = 0;
        GLint vp_after[4] = {};
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &draw_after);
        glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &read_after);
        glGetIntegerv(GL_VIEWPORT, vp_after);
        require(draw_after == draw_fbo && read_after == read_fbo, "DRAW/READ FBO restored");
        require(vp_after[0] == vp[0] && vp_after[1] == vp[1] &&
                    vp_after[2] == vp[2] && vp_after[3] == vp[3],
                "viewport restored");
        require(hud.begin_frame(*context) == HudStatus::Ok, "fbo empty begin");
        require(hud.flush(*context) == HudStatus::Ok, "fbo empty batch");
        require(context->GetFramebuffer() == fbo, "FBO kept after empty batch");
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &draw_after);
        glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &read_after);
        require(draw_after == draw_fbo && read_after == read_fbo, "DRAW/READ FBO after empty batch");
        context->SetFramebuffer(nullptr);
        context->CommitFramebuffer();
        require_gl("fbo paths");

        context->SetViewport(0, 0, fb_w, fb_h);
        context->CommitViewport();
        context->Clear(clear);
        glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
        context->NotifyExternalStateMutation();
        context->Clear(clear);
        glFinish();
        unsigned char pixel[4] = {};
        glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
        require(pixel[2] > 16 && pixel[0] < 16, "same ClearState resubmits clear color");
        require_gl("clear resubmit");

        if (glBindImageTexture != nullptr)
        {
            context->SetImage(0, fbo_color, ur::AccessType::ReadWrite);
            context->CommitBindings();
            GLint image_before = 0;
            glGetIntegeri_v(GL_IMAGE_BINDING_NAME, 0, &image_before);
            if (image_before != 0)
            {
                glBindImageTexture(0, 0, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8);
                context->NotifyExternalStateMutation();
                context->CommitBindings();
                GLint image_name = 0;
                glGetIntegeri_v(GL_IMAGE_BINDING_NAME, 0, &image_name);
                require(image_name == image_before, "image unit rebound after Notify");
            }
            require_gl("image unit");
        }

        context->SetViewport(0, 0, fb_w, fb_h);
        context->Clear(clear);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glDisable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ZERO);
        context->NotifyExternalStateMutation();
        require(hud.begin_frame(*context) == HudStatus::Ok, "state begin");
        require(hud.draw_utf8("HUD 0123456789", {8.0f, 40.0f}, style) == HudStatus::Ok, "draw latin");
        require(hud.flush(*context) == HudStatus::Ok, "flush latin");
        require(glIsEnabled(GL_DEPTH_TEST) == GL_FALSE, "depth off");
        require(glIsEnabled(GL_CULL_FACE) == GL_FALSE, "cull off");
        require(glIsEnabled(GL_BLEND) == GL_TRUE, "blend on");
        GLint blend_src = 0;
        GLint blend_dst = 0;
        glGetIntegerv(GL_BLEND_SRC, &blend_src);
        glGetIntegerv(GL_BLEND_DST, &blend_dst);
        require(blend_src == GL_SRC_ALPHA, "SrcAlpha");
        require(blend_dst == GL_ONE_MINUS_SRC_ALPHA, "OneMinusSrcAlpha");
        require_gl("state after HUD");

        int frames_with_text = 0;
        for (int frame = 0; frame < 3; ++frame)
        {
            context->SetViewport(0, 0, fb_w, fb_h);
            context->Clear(clear);
            require(hud.begin_frame(*context) == HudStatus::Ok, "cached begin");
            require(hud.draw_utf8("HUD 0123456789", {8.0f, 40.0f}, style) == HudStatus::Ok,
                    "cached draw");
            require(hud.flush(*context) == HudStatus::Ok, "cached flush");
            glFinish();
            if (count_text_pixels(fb_w, fb_h) > 8)
            {
                ++frames_with_text;
            }
            glfwSwapBuffers(window);
            require_gl("cached frame");
        }
        require(frames_with_text == 3, "three cached frames have text pixels");

        context->SetViewport(0, 0, fb_w, fb_h);
        context->Clear(clear);
        require(hud.begin_frame(*context) == HudStatus::Ok, "cjk begin");
        const char cjk_utf8[] =
            "\xE6\x96\xB9\xE5\x9D\x97\xE3\x80\x81"
            "\xE8\x83\x8C\xE5\x8C\x85\xE3\x80\x81"
            "\xE7\x94\x9F\xE5\x91\xBD\xE5\x80\xBC\xE3\x80\x81"
            "\xE5\x9D\x90\xE6\xA0\x87";
        const HudStatus cjk = hud.draw_utf8(std::string_view(cjk_utf8, sizeof(cjk_utf8) - 1),
                                            {8.0f, 80.0f}, style);
        require(cjk == HudStatus::Ok, "CJK draw");
        require(hud.flush(*context) == HudStatus::Ok, "CJK flush");
        glFinish();
        require(count_text_pixels(fb_w, fb_h) > 8, "CJK pixels visible");
        require_gl("CJK");

        GLFWwindow* other = glfwCreateWindow(64, 64, "hud_gl_other", nullptr, nullptr);
        require(other != nullptr, "second GL window");
        glfwMakeContextCurrent(other);
        {
            GtxtHud foreign;
            require(foreign.init(*device, font_path) == HudStatus::GpuResourceFailed,
                    "second GL context is rejected");
        }
        glfwMakeContextCurrent(window);
        require(hud.begin_frame(*context) == HudStatus::Ok, "owner context after reject");
        require(hud.flush(*context) == HudStatus::Ok, "owner flush after reject");
        glfwDestroyWindow(other);

        hud.shutdown();
        context.reset();
        device.reset();
        glfwDestroyWindow(window);
        glfwTerminate();
        std::cout << "hud_gl_smoke_tests: PASS" << std::endl;
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "hud_gl_smoke_tests: FAIL: " << error.what() << std::endl;
        glfwTerminate();
        return 1;
    }
}
