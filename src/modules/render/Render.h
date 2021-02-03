#pragma once

#include "core/macro.h"

#include <memory>

namespace ur { class Device; class Context; }

namespace tt
{

class Render
{
public:
	auto Device() const { return m_dev; }
	auto Context() const { return m_ctx; }

private:
	std::shared_ptr<ur::Device>  m_dev = nullptr;
	std::shared_ptr<ur::Context> m_ctx = nullptr;

	TT_SINGLETON_DECLARATION(Render)

}; // Render

}