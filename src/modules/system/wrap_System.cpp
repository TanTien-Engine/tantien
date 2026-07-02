#include "modules/system/wrap_System.h"
#include "modules/system/System.h"

#include <string.h>

#include <glfw/glfw3.h>

namespace
{

void w_System_set_clipboard_text()
{
    const char* text = ves_tostring(1);
    tt::System::Instance()->SetClipboardText(text);
}

void w_System_get_clipboard_text()
{
    const char* text = tt::System::Instance()->GetClipboardText();
    if (text) {
        ves_set_lstring(0, text, strlen(text));
    } else {
        ves_set_nil(0);
    }
}

void w_System_set_title()
{
    const char* text = ves_tostring(1);
    if (text) {
        tt::System::Instance()->SetWindowTitle(text);
    }
    ves_set_nil(0);
}

void w_System_request_redraw()
{
    tt::System::Instance()->RequestRedraw();
    ves_set_nil(0);
}

}

namespace tt
{

VesselForeignMethodFn SystemBindMethod(const char* signature)
{
    if (strcmp(signature, "static System.set_clipboard_text(_)") == 0) return w_System_set_clipboard_text;
    if (strcmp(signature, "static System.get_clipboard_text()") == 0) return w_System_get_clipboard_text;
    if (strcmp(signature, "static System.set_title(_)") == 0) return w_System_set_title;
    if (strcmp(signature, "static System.request_redraw()") == 0) return w_System_request_redraw;

    return NULL;
}

void SystemBindClass(const char* class_name, VesselForeignClassMethods* methods)
{
}

}