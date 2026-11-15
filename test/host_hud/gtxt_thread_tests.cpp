#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "modules/graphics/DTex.h"
#include "modules/graphics/GTxt.h"
#include "modules/graphics/GtxtCore.h"
#include "modules/graphics/Graphics.h"
#include "modules/graphics/LoadingList.h"

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include <unirender/Context.h>
#include <unirender/Device.h>
#include <unirender/opengl/Context.h>
#include <unirender/opengl/Device.h>

#include <windows.h>

#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>

namespace tt
{

Graphics* Graphics::m_instance = nullptr;

Graphics* Graphics::Instance()
{
    return m_instance;
}

void Graphics::DrawPainter(const tess::Painter&, const sm::rect&) const
{
}

void Graphics::DrawTexQuad(const float*, const float*, const ur::TexturePtr&, uint32_t)
{
}

} // namespace tt

namespace
{

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
        GLFWwindow* window = glfwCreateWindow(64, 64, "gtxt_thread", nullptr, nullptr);
        require(window != nullptr, "glfwCreateWindow");
        glfwMakeContextCurrent(window);
        require(gl3wInit() == 0, "gl3wInit");

        std::shared_ptr<ur::Device> device = std::make_shared<ur::opengl::Device>();
        require(static_cast<bool>(device), "OpenGL device");
        device->Init();
        std::shared_ptr<ur::Context> context = std::make_shared<ur::opengl::Context>(*device);
        require(static_cast<bool>(context), "OpenGL context");
        require(tt::DTex::Instance()->Init(*device), "DTex::Init");

        const char* font_path = pick_font();
        require(font_path != nullptr, "need assets/fonts for GTxt thread test");
        require(tt::GtxtCore::Ensure(), "GtxtCore::Ensure before GTxt::Init");
        require(tt::GtxtCore::AddFont("default", font_path) >= 0, "GtxtCore::AddFont");
        require(tt::GtxtCore::ready(), "core ready does not imply GTxt::Init");
        require(gtxt_label_cb_get() == nullptr, "process draw callback not installed yet");

        gtxt_label_style style;
        tt::GTxt::InitLabelStype(style);
        sm::Matrix2D mat;

        require(!tt::GTxt::Draw(*context, "AB", style, mat, 0xffffffff, 0, 0, false),
                "plain Draw before GTxt::Init");
        require(!tt::GTxt::Draw(*context, "XY", style, mat, 0xffffffff, 0, 0, true),
                "richtext Draw before GTxt::Init");
        const tt::DTexFlushResult before_init = tt::LoadingList::Instance()->Flush(*context);
        require(before_init.success && !before_init.had_work,
                "Draw before GTxt::Init must not queue LoadingList work");
        require_gl("flush before GTxt::Init");

        tt::GTxt::Instance()->Init(*device);
        require(tt::GtxtCore::OnOwnerThread(), "owner thread after Init");
        require(tt::GtxtCore::ready(), "GtxtCore ready after Init");
        require(gtxt_label_cb_get() != nullptr, "process draw callback installed");
        require(tt::GTxt::Instance()->LoadFont("default", font_path), "LoadFont default");

        bool worker_plain = true;
        bool worker_rich = true;
        bool worker_has_gl = true;
        std::thread worker([&]()
        {
            worker_has_gl = wglGetCurrentContext() != nullptr;
            worker_plain = tt::GTxt::Draw(*context, "AB", style, mat, 0xffffffff, 0, 0, false);
            worker_rich = tt::GTxt::Draw(*context, "XY", style, mat, 0xffffffff, 0, 0, true);
        });
        worker.join();
        require(!worker_has_gl, "worker has no GL context");
        require(!worker_plain, "worker plain Draw rejected");
        require(!worker_rich, "worker richtext Draw rejected");

        const tt::DTexFlushResult after_worker = tt::LoadingList::Instance()->Flush(*context);
        require(after_worker.success && !after_worker.had_work,
                "worker Draw must not queue LoadingList work");
        require_gl("flush after worker");

        glfwMakeContextCurrent(nullptr);
        bool stolen_plain = true;
        bool stolen_rich = true;
        bool stolen_has_gl = false;
        std::thread stolen([&]()
        {
            glfwMakeContextCurrent(window);
            stolen_has_gl = wglGetCurrentContext() != nullptr;
            stolen_plain = tt::GTxt::Draw(*context, "CD", style, mat, 0xffffffff, 0, 0, false);
            stolen_rich = tt::GTxt::Draw(*context, "EF", style, mat, 0xffffffff, 0, 0, true);
            glfwMakeContextCurrent(nullptr);
        });
        stolen.join();
        glfwMakeContextCurrent(window);
        require(stolen_has_gl, "worker received the real GL context");
        require(!stolen_plain, "wrong-thread plain Draw rejected with GL context");
        require(!stolen_rich, "wrong-thread richtext Draw rejected with GL context");
        const tt::DTexFlushResult after_stolen = tt::LoadingList::Instance()->Flush(*context);
        require(after_stolen.success && !after_stolen.had_work,
                "GL-stealing worker must not queue LoadingList work");
        require_gl("flush after GL-stealing worker");

        require(tt::GTxt::Draw(*context, "AB", style, mat, 0xffffffff, 0, 0, false),
                "owner plain Draw");
        require_gl("owner plain Draw");
        const tt::DTexFlushResult plain_flush = tt::LoadingList::Instance()->Flush(*context);
        require(plain_flush.success && plain_flush.had_work, "owner plain Draw uploads");
        require_gl("owner plain Flush");

        require(tt::GTxt::Draw(*context, "XY", style, mat, 0xffffffff, 0, 0, true),
                "owner richtext Draw");
        require_gl("owner richtext Draw");
        const tt::DTexFlushResult rich_flush = tt::LoadingList::Instance()->Flush(*context);
        require(rich_flush.success && rich_flush.had_work, "owner richtext Draw uploads");
        require_gl("owner richtext Flush");

        context.reset();
        device.reset();
        glfwDestroyWindow(window);
        glfwTerminate();
        std::cout << "gtxt_thread_tests: PASS" << std::endl;
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "gtxt_thread_tests: FAIL: " << error.what() << std::endl;
        glfwTerminate();
        return 1;
    }
}
