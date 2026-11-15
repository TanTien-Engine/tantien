#include "modules/system/System.h"

#include <glfw/glfw3.h>

namespace tt
{

TT_SINGLETON_DEFINITION(System)

System::System()
{
}

System::~System()
{
}

void System::SetWindow(GLFWwindow* window)
{
	m_window = window;
}

void System::SetClipboardText(const char* text) const
{
	if (m_window) {
		glfwSetClipboardString(m_window, text);
	}
}

const char* System::GetClipboardText() const
{
	return m_window ? glfwGetClipboardString(m_window) : nullptr;
}

void System::SetWindowSize(int w, int h) const
{
	if (m_window) {
		glfwSetWindowSize(m_window, w, h);
	}
}

void System::RequestWindowSize(int w, int h)
{
	m_pending_w = w;
	m_pending_h = h;
}

bool System::ConsumePendingResize(int* w, int* h)
{
	if (m_pending_w <= 0 || m_pending_h <= 0) {
		return false;
	}
	*w = m_pending_w;
	*h = m_pending_h;
	m_pending_w = 0;
	m_pending_h = 0;
	return true;
}

void System::SetWindowTitle(const char* title) const
{
	if (m_window && title) {
		glfwSetWindowTitle(m_window, title);
	}
}

}