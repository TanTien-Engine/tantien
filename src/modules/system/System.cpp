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
	return m_window ? glfwGetClipboardString(m_window) : "";
}

}