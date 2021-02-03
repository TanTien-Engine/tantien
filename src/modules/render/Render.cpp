#include "modules/render/Render.h"

#include <unirender/Device.h>
#include <unirender/Context.h>
#include <unirender/Factory.h>

namespace tt
{

TT_SINGLETON_DEFINITION(Render)

Render::Render()
{
    m_dev = ur::CreateDevice(ur::APIType::OpenGL);
    m_dev->Init();

    m_ctx = ur::CreateContext(ur::APIType::OpenGL, *m_dev, nullptr);
}

Render::~Render()
{
}

}