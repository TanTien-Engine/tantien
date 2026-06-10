#include "modules/render/Render.h"

#include <unirender/Device.h>
#include <unirender/Context.h>
#include <unirender/Factory.h>

#ifdef __APPLE__
#include "modules/system/System.h"
#include <cstdint>
#include <cstdlib>
#include <string>
struct GLFWwindow;
// defined in src/editor/metal_view_mac.mm
extern void* tt_get_metal_view(GLFWwindow* window, uint32_t* out_w, uint32_t* out_h);
extern void* tt_get_metal_layer(GLFWwindow* window, uint32_t* out_w, uint32_t* out_h);
#endif

namespace tt
{

TT_SINGLETON_DEFINITION(Render)

Render::Render()
{
    void* hwnd = nullptr;
    uint32_t w = 0, h = 0;
#ifdef __APPLE__
    // Default to the native Metal backend. Opt into the Vulkan-on-MoltenVK path with
    // env var TT_RENDER_API=vulkan (no rebuild needed to switch back to Metal).
    ur::APIType api = ur::APIType::Metal;
    if (const char* api_env = std::getenv("TT_RENDER_API")) {
        if (std::string(api_env) == "vulkan" || std::string(api_env) == "Vulkan") {
            api = ur::APIType::Vulkan;
        }
    }
    if (GLFWwindow* win = System::Instance()->GetWindow()) {
        // Metal needs the NSView; Vulkan (vkCreateMetalSurfaceEXT) needs a CAMetalLayer.
        hwnd = (api == ur::APIType::Vulkan) ? tt_get_metal_layer(win, &w, &h)
                                            : tt_get_metal_view(win, &w, &h);
    }
#else
    const ur::APIType api = ur::APIType::OpenGL;
#endif
    m_dev = ur::CreateDevice(api);
    m_dev->Init();

    m_ctx = ur::CreateContext(api, *m_dev, hwnd, w, h);
}

Render::~Render()
{
}

void Render::SetViewport(int x, int y, int w, int h)
{
    m_ctx->SetViewport(x, y, w, h);
}

}