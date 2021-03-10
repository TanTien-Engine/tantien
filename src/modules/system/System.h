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

	void SetClipboardText(const char* text) const;
	const char* GetClipboardText() const;

private:
	GLFWwindow* m_window = nullptr;

	TT_SINGLETON_DECLARATION(System)

}; // System

}