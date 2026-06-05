// macOS-only: bridge a GLFW window to the native NSView (and its framebuffer size)
// so the unirender Metal context can attach its CAMetalLayer to it. Compiled as
// Objective-C++ and only added to the editor target on Apple.

#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#import <Cocoa/Cocoa.h>
#include <cstdint>

void* tt_get_metal_view(GLFWwindow* window, uint32_t* out_w, uint32_t* out_h)
{
    // Logical (point) size, NOT the framebuffer pixel size: the editor works in
    // points and the Metal layer's contentsScale is forced to 1, so the drawable
    // must be the point size or content lands in a sub-region on Retina displays.
    int ww = 0, wh = 0;
    glfwGetWindowSize(window, &ww, &wh);
    if (out_w) { *out_w = static_cast<uint32_t>(ww); }
    if (out_h) { *out_h = static_cast<uint32_t>(wh); }

    NSWindow* nswin = glfwGetCocoaWindow(window);
    if (!nswin) { return nullptr; }
    return (__bridge void*)nswin.contentView; // NSView*
}
