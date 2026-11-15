#pragma once

#include "core/macro.h"

#include <functional>
#include <string>

struct GLFWwindow;

namespace tt
{

class System
{
public:
	void SetWindow(GLFWwindow* window);
	GLFWwindow* GetWindow() const { return m_window; }

	void SetClipboardText(const char* text) const;
	const char* GetClipboardText() const;

	// Window title (e.g. an async load's "feature N/M" progress). No-op
	// until SetWindow ran.
	void SetWindowTitle(const char* title) const;

	// Render-on-demand hook: the editor's main loop only draws when
	// something requested a redraw (input, window refresh). A script-side
	// poller -- e.g. LoadZw returning nil while its worker converts --
	// calls RequestRedraw() so the next frame re-pulls it; the host app
	// installs the actual requester (editor main.cpp's request_redraw).
	void SetRedrawRequester(std::function<void()> fn) { m_redraw_requester = std::move(fn); }
	void RequestRedraw() const { if (m_redraw_requester) { m_redraw_requester(); } }

private:
	GLFWwindow* m_window = nullptr;

	std::function<void()> m_redraw_requester;

	TT_SINGLETON_DECLARATION(System)

}; // System

}