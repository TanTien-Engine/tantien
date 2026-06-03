#include "modules/render/Render.h"

#include <unirender/Device.h>
#include <unirender/Context.h>
#include <unirender/Factory.h>

#ifdef __APPLE__
#include "modules/system/System.h"
#include <cstdint>
struct GLFWwindow;
// defined in src/editor/metal_view_mac.mm
extern void* tt_get_metal_view(GLFWwindow* window, uint32_t* out_w, uint32_t* out_h);
#endif

namespace tt
{

TT_SINGLETON_DEFINITION(Render)

Render::Render()
{
    void* hwnd = nullptr;
    uint32_t w = 0, h = 0;
#ifdef __APPLE__
    const ur::APIType api = ur::APIType::Metal;
    if (GLFWwindow* win = System::Instance()->GetWindow()) {
        hwnd = tt_get_metal_view(win, &w, &h);
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