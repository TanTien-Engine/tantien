#pragma once

#include "core/macro.h"

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

	void SetTitle(const char* title) const;
	// Editor's main loop currently polls unconditionally (no WaitEvents-based
	// render-on-demand yet), so every frame already redraws. Kept as a no-op
	// entry point so .ves callers (e.g. cadcvt's async LoadZw) have a stable
	// hook to pump once the loop grows an idle/wait path.
	void RequestRedraw() const;

private:
	GLFWwindow* m_window = nullptr;

	TT_SINGLETON_DECLARATION(System)

}; // System

}