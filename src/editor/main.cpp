#include "tantien.h"

// macOS/POSIX shims for a couple of Win32-isms used below (VM/Windows build unaffected).
#ifndef _WIN32
#include <unistd.h> // usleep
#define _strdup strdup
#define Sleep(ms) ::usleep(static_cast<useconds_t>(ms) * 1000) // Win32 Sleep takes ms
// shadertrans's DXC WinAdapter (CHandle::~CHandle) references CloseHandle; the DXC
// WinFunctions.cpp that defines it drags in unbuilt LLVM headers, so stub the one
// symbol here (BOOL=bool, HANDLE=void* per dxc/WinAdapter.h). Never exercised on macOS.
bool CloseHandle(void*) { return true; }
#endif
// The .ves module registry + loader/binders now live in ves_host.{h,cpp},
// shared with the headless ves_test harness. Only the runtime headers the
// editor itself calls into remain here.
#include "ves_host.h"
#ifdef __APPLE__
#include "modules/render/Render.h"  // Render::Instance()->Context() for the Metal present
#include <unirender/Context.h>
#endif
#include "modules/system/System.h"      // tt::System::Instance() singleton
#include "modules/io/Keyboard.h"        // tt::Keyboard::RegisterCallback
#include "cax/brepgraph_c/BrepGraph.h"  // brepgraph::init_cb()
#include "cax/brepdb_c/BrepDBInit.h"    // brepdb::init_cb()

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <filesystem>
#include <string>
#include <cstdlib>

#include <assert.h>

#include "SdfPreview.h"  // GPU voxel/SDF interactive-preview prototype (env-gated)

namespace
{

bool error = false;

void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
    error = true;
    std::cout << "[OpenGL Error](" << type << ") " << message << std::endl;
}

void error_callback(int error, const char *msg) {
    std::cerr << "GLWT error " << error << ": " << msg << std::endl;
}

// --- render-on-demand ---
// The main loop skips draw()+present on idle frames to cut idle CPU/GPU. Any
// real input (detected in process_input + the GLFW callbacks below) requests a
// short BURST of redraws: >=2 refills BOTH double buffers (we render straight to
// the default framebuffer and swap, so one redraw fills only one buffer) and
// lets the immediate-mode GUI settle before going idle. Starts >0 so the
// startup/load frames draw. NOTE: purely data-driven animation with no input (a
// GlobalTicker or a Time node wired into the graph -- absent from the CAD
// graphs) would need an explicit request_redraw() hook; add one if such a graph
// is ever loaded.
const int REDRAW_BURST = 3;
int g_redraw_frames = 8;
void request_redraw() { if (g_redraw_frames < REDRAW_BURST) { g_redraw_frames = REDRAW_BURST; } }

void call_sizechanged(int w, int h)
{
    ves_pushnumber(w);
    ves_pushnumber(h);
    ves_pushstring("sizechanged(_,_)");
    ves_call(2, 0);
}

void window_size_callback(GLFWwindow* window, int width, int height)
{
    request_redraw();
    call_sizechanged(width, height);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    request_redraw();

    double x, y;
    glfwGetCursorPos(window, &x, &y);

    ves_pushnumber(x);
    ves_pushnumber(y);
    ves_pushnumber(xoffset);
    ves_pushnumber(yoffset);
    ves_pushstring("mousescrolled(_,_,_,_)");
    ves_call(4, 0);
}

void call_keypressed(const char* str)
{
    ves_pushstring(str);
    ves_pushstring("keypressed(_)");
    ves_call(1, 0);
}

void call_keyreleased(const char* str)
{
    ves_pushstring(str);
    ves_pushstring("keyreleased(_)");
    ves_call(1, 0);
}

// Text-entry path: printables + navigation reach the script as `keytyped(_)`.
// The editor routes them into the focused panel text field (gui PanelKit)
// and IGNORES them otherwise, so existing graphs never see these events.
void call_keytyped(const char* str)
{
    ves_pushstring(str);
    ves_pushstring("keytyped(_)");
    ves_call(1, 0);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    request_redraw();

    const bool ctrl_pressed =
        glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;
    if (key == GLFW_KEY_O && action == GLFW_PRESS && ctrl_pressed) {
        ves_pushstring("loadfromfile()");
        ves_call(0, 0);
    } else if (key == GLFW_KEY_S && action == GLFW_PRESS && ctrl_pressed) {
        ves_pushstring("savetofile()");
        ves_call(0, 0);
    } else if (key == GLFW_KEY_C && action == GLFW_PRESS && ctrl_pressed) {
        ves_pushstring("copy_to_clipboard()");
        ves_call(0, 0);
    } else if (key == GLFW_KEY_V && action == GLFW_PRESS && ctrl_pressed) {
        ves_pushstring("paste_from_clipboard()");
        ves_call(0, 0);
    } else if (key == GLFW_KEY_F5 && action == GLFW_PRESS) {
        ves_pushstring("refresh()");
        ves_call(0, 0);
    } else if (key == GLFW_KEY_DELETE && action == GLFW_PRESS) {
        call_keypressed("delete");
    } else if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
        call_keypressed("space");
    } else if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        call_keypressed("escape");
    } else if (key == GLFW_KEY_PRINT_SCREEN && action == GLFW_PRESS) {
        ves_pushstring("print_screen()");
        ves_call(0, 0);
    } else if (!ctrl_pressed &&
               (action == GLFW_PRESS || action == GLFW_REPEAT)) {
        // Text-entry forwarding for panel fields (editor routes into the
        // focused field via GUI.key_input; unmatched keys are ignored there).
        const bool shift = (mods & GLFW_MOD_SHIFT) != 0;
        if (key >= GLFW_KEY_A && key <= GLFW_KEY_Z) {
            std::string str;
            str.push_back(static_cast<char>((shift ? 'A' : 'a') + (key - GLFW_KEY_A)));
            call_keytyped(str.c_str());
        } else if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9) {
            std::string str;
            str.push_back(static_cast<char>('0' + (key - GLFW_KEY_0)));
            call_keytyped(str.c_str());
        } else if (key == GLFW_KEY_MINUS) {
            call_keytyped("-");
        } else if (key == GLFW_KEY_EQUAL) {
            call_keytyped("=");
        } else if (key == GLFW_KEY_PERIOD) {
            call_keytyped(".");
        } else if (key == GLFW_KEY_COMMA) {
            call_keytyped(",");
        } else if (key == GLFW_KEY_SLASH) {
            call_keytyped("/");
        } else if (key == GLFW_KEY_ENTER || key == GLFW_KEY_KP_ENTER) {
            call_keytyped("enter");
        } else if (key == GLFW_KEY_BACKSPACE) {
            call_keytyped("backspace");
        } else if (key == GLFW_KEY_LEFT) {
            call_keytyped("left");
        } else if (key == GLFW_KEY_RIGHT) {
            call_keytyped("right");
        } else if (key == GLFW_KEY_UP) {
            call_keytyped("up");
        } else if (key == GLFW_KEY_DOWN) {
            call_keytyped("down");
        } else if (key == GLFW_KEY_HOME) {
            call_keytyped("home");
        } else if (key == GLFW_KEY_END) {
            call_keytyped("end");
        }
    } else if (key >= GLFW_KEY_A && key <= GLFW_KEY_Z && action == GLFW_RELEASE) {
        std::string str;
        str.push_back('a' + (key - GLFW_KEY_A));
        call_keyreleased(str.c_str());
    }
}

void mouse_press_callback(GLFWwindow* window, int button, int action, int mods)
{
    request_redraw();

    // double click
    if (action == GLFW_RELEASE)
    {
        static auto before = std::chrono::system_clock::now();
        auto now = std::chrono::system_clock::now();
        double diff_ms = std::chrono::duration <double, std::milli>(now - before).count();
        before = now;
        if (diff_ms > 10 && diff_ms < 250) 
        {
            double x, y;
            glfwGetCursorPos(window, &x, &y);

            ves_pushnumber(x);
            ves_pushnumber(y);
            ves_pushnumber(button);
            ves_pushstring("mouseclicked(_,_,_)");
            ves_call(3, 0);
        }
    }
}

void drop_callback(GLFWwindow* window, int count, const char** paths)
{
    if (count == 0) {
        return;
    }

    request_redraw();

    double x, y;
    glfwGetCursorPos(window, &x, &y);

    if (y < 50)
    {
        ves_pushstring("clear()");
        ves_call(0, 0);

        ves_pushstring(paths[0]);
        ves_pushstring("loadfromfile(_)");
        ves_call(1, 0);
    }
    else
    {
        ves_pushstring(paths[0]);
        ves_pushnumber(x);
        ves_pushnumber(y);
        ves_pushstring("add_node_from_file(_,_,_)");
        ves_call(3, 0);
    }
}

void call_mousemoved(double x, double y, int button)
{
    ves_pushnumber(x);
    ves_pushnumber(y);
    ves_pushnumber(button);
    ves_pushstring("mousemoved(_,_,_)");
    ves_call(3, 0);
}

void call_mousedragged(double x, double y, int button)
{
    ves_pushnumber(x);
    ves_pushnumber(y);
    ves_pushnumber(button);
    ves_pushstring("mousedragged(_,_,_)");
    ves_call(3, 0);
}

void call_mousepressed(double x, double y, int button)
{
    ves_pushnumber(x);
    ves_pushnumber(y);
    ves_pushnumber(button);
    ves_pushstring("mousepressed(_,_,_)");
    ves_call(3, 0);
}

void call_mousereleased(double x, double y, int button)
{
    ves_pushnumber(x);
    ves_pushnumber(y);
    ves_pushnumber(button);
    ves_pushstring("mousereleased(_,_,_)");
    ves_call(3, 0);
}

enum class MouseButton
{
    None   = 0,
    Left   = 1,
    Right  = 2,
    Middle = 3,
};

enum class MouseStatus
{
    Default,
    LeftPressed,
    RightPressed,
    MiddlePressed,
};

MouseStatus mouse_status = MouseStatus::Default;

void process_input(GLFWwindow *window)
{
    bool active = false;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    } else if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        call_keypressed("w");
        active = true;
    } else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        call_keypressed("s");
        active = true;
    } else if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        call_keypressed("a");
        active = true;
    } else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        call_keypressed("d");
        active = true;
    }

    double x, y;
    glfwGetCursorPos(window, &x, &y);

    // Real cursor movement is activity. process_input synthesizes a mousemoved
    // every frame even when the cursor is still, so compare against the last pos
    // rather than treating "the callback ran" as a change.
    static double last_x = 0, last_y = 0;
    static bool have_last = false;
    if (!have_last || x != last_x || y != last_y) {
        active = true;
    }
    last_x = x;
    last_y = y;
    have_last = true;

    switch (mouse_status)
    {
    case MouseStatus::Default:
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            mouse_status = MouseStatus::LeftPressed;
            call_mousepressed(x, y, static_cast<int>(MouseButton::Left));
            active = true;
        } else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
            mouse_status = MouseStatus::RightPressed;
            call_mousepressed(x, y, static_cast<int>(MouseButton::Right));
            active = true;
        } else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS) {
            mouse_status = MouseStatus::MiddlePressed;
            call_mousepressed(x, y, static_cast<int>(MouseButton::Middle));
            active = true;
        } else {
            call_mousemoved(x, y, static_cast<int>(MouseButton::None));
        }
        break;
    case MouseStatus::LeftPressed:
        active = true;   // dragging or releasing is activity
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE) {
            mouse_status = MouseStatus::Default;
            call_mousereleased(x, y, static_cast<int>(MouseButton::Left));
        } else {
            call_mousedragged(x, y, static_cast<int>(MouseButton::Left));
        }
        break;
    case MouseStatus::RightPressed:
        active = true;
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_RELEASE) {
            mouse_status = MouseStatus::Default;
            call_mousereleased(x, y, static_cast<int>(MouseButton::Right));
        } else {
            call_mousedragged(x, y, static_cast<int>(MouseButton::Right));
        }
        break;
    case MouseStatus::MiddlePressed:
        active = true;
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_RELEASE) {
            mouse_status = MouseStatus::Default;
            call_mousereleased(x, y, static_cast<int>(MouseButton::Middle));
        } else {
            call_mousedragged(x, y, static_cast<int>(MouseButton::Middle));
        }
        break;
    default:
        assert(0);
    }

    if (active) {
        request_redraw();
    }
}

void show_fps(GLFWwindow* window)
{
    static double last_time = 0;
    static int frames = 0;

    double curr_time = glfwGetTime();
    double delta = curr_time - last_time;
    frames++;
    if (delta >= 1.0)
    {
        //std::cout << 1000.0/double(frames) << std::endl;

        double fps = double(frames) / delta;

        std::stringstream ss;
        ss << " [" << fps << " FPS]";

        glfwSetWindowTitle(window, ss.str().c_str());

        frames = 0;
        last_time = curr_time;
    }
}

void limit_fps(int fps)
{
    static double last_time = 0;
    double curr_time = glfwGetTime();
    double delta = curr_time - last_time;
    if (delta < 1.0f / fps) {
        Sleep(static_cast<unsigned long>((1.0f / fps - delta) * 1000));
    }
    last_time = glfwGetTime();
}

}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        std::cerr << "Need editor name!" << std::endl;
        glfwTerminate();
        return 1;
    }

    int width = 1024;
    int height = 768;

    glfwSetErrorCallback(error_callback);

    if(glfwInit() == GL_FALSE) {
        std::cerr << "failed to init GLFW" << std::endl;
        return 1;
    }

#ifdef __APPLE__
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Metal: no OpenGL context on this window
    // Keep the framebuffer 1:1 with the window: the editor works in logical points,
    // but a Retina drawable would be 2x the size -> content rendered into a quarter.
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_FALSE);
#else
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
#endif

    GLFWwindow *window;
    if((window = glfwCreateWindow(width, height, argv[1], 0, 0)) == 0) {
        std::cerr << "failed to open window" << std::endl;
        glfwTerminate();
        return 1;
    }

#ifndef __APPLE__
    glfwMakeContextCurrent(window); // Metal has no GL context to make current
#endif
    glfwSetWindowSizeCallback(window, window_size_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_press_callback);
    glfwSetDropCallback(window, drop_callback);
    // Render-on-demand: OS-driven repaint (uncover/restore, window move with no
    // resize, RDP/odd-driver surface loss) carries no input event, so request a
    // redraw burst here too. Cheap insurance; DWM already survives most occlusion.
    glfwSetWindowRefreshCallback(window, [](GLFWwindow*){ request_redraw(); });

#ifndef __APPLE__ // gl3w loads OpenGL entry points; the macOS build uses Metal
    if(gl3wInit()) {
        std::cerr << "failed to init GL3W" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
#endif

    // Enable debug output
    //glEnable(GL_DEBUG_OUTPUT);
    //glDebugMessageCallback(MessageCallback, 0);

    tt::Keyboard::Funs keyboard_cb;
    keyboard_cb.is_ctrl_pressed = [&]()->bool {
        return
            glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;
    };
    keyboard_cb.is_shift_pressed = [&]()->bool {
        return
            glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
    };
    tt::Keyboard::RegisterCallback(keyboard_cb);

    brepgraph::init_cb();
    brepdb::init_cb();

    ves_init_vm();

    tt::System::Instance()->SetWindow(window);
    // Let script-side pollers (e.g. LoadZw converting on a worker thread)
    // keep the render-on-demand loop pumping while they wait.
    tt::System::Instance()->SetRedrawRequester([]() { request_redraw(); });

    tt_enable_dtex();

    VesselConfiguration cfg{};
    tt_ves_host::configure(&cfg);  // module loader + foreign binders + stdout write
    ves_set_config(&cfg);
    tt_ves_host::prime_lazy_import_classes();

    std::string editor_path = argv[1];
    if (std::filesystem::is_regular_file(editor_path))
    {
        auto filepath = std::filesystem::path(editor_path);

        auto dir = filepath.parent_path();
        tt_ves_host::search_paths.push_back(dir.string() + "/");

        auto file = filepath.stem().string();
        auto cls_name = file;
        cls_name[0] = std::toupper(cls_name[0]);

        char code[255];
        snprintf(code, sizeof(code), "import \"%s\" for %s\nvar _editor = %s()", file.c_str(), cls_name.c_str(), cls_name.c_str());

        ves_interpret("editor", code);
    }
    else
    {
        auto cls_name = editor_path;
        cls_name[0] = std::toupper(cls_name[0]);

        char code[255];
        snprintf(code, sizeof(code), "import \"editor.%s\" for %s\nvar _editor = %s()", argv[1], cls_name.c_str(), cls_name.c_str());
        ves_interpret("editor", code);
    }

    ves_getglobal("_editor");
    ves_pushstring("load()");
    ves_call(0, 0);

    call_sizechanged(width, height);

    if (argc > 2) {
        ves_pushstring(argv[2]);
        ves_pushstring("loadfromfile(_)");
        ves_call(1, 0);
    }

    // render-on-demand frame stats (probe): prints drawn vs skipped frames + the
    // last draw's wall time every ~2s so the idle skipping and the per-draw cost
    // are measurable. Idle should show skipped >> drawn; a draw shows the ms we save.
    double stat_t0 = glfwGetTime();
    int stat_drawn = 0;
    int stat_skipped = 0;
    double stat_last_draw_ms = 0.0;

    while(!glfwWindowShouldClose(window))
    {
        // process_input + the GLFW callbacks raise g_redraw_frames on real input.
        // Poll first so a callback-driven event counts toward THIS frame's decision.
        process_input(window);
        glfwPollEvents();

        // update() is cheap and must run every frame (scene/GUI/tickers advance and
        // it is where input-driven dirties are set); only the expensive draw()+present
        // is gated by render-on-demand.
        ves_pushstring("update()");
        ves_call(0, 0);

        if (g_redraw_frames > 0)
        {
            --g_redraw_frames;

            double draw_t0 = glfwGetTime();

            ves_pushstring("draw()");
            ves_call(0, 0);

#ifndef __APPLE__
        // ---- GPU voxel/SDF interactive-preview prototype (Phase 0) ----
        // Pure additive, display-only, env-gated. Loads a committed B-rep body
        // from a STEP file (CAX_PREVIEW_SHAPE), voxelizes it once into an SDF,
        // then raymarches a CSG preview (body cut by an animated tool) over the
        // framebuffer every frame, logging one-time voxelize + per-frame ms.
        // Never feeds the modeling graph / topo-naming. See SdfPreview.{h,cpp}.
        if (std::getenv("CAX_GPU_PREVIEW"))
        {
            static tt::SdfPreview* s_prev = nullptr;
            static bool s_prev_failed = false;
            static int  s_prev_frame  = 0;
            if (!s_prev && !s_prev_failed)
            {
                const char* shape = std::getenv("CAX_PREVIEW_SHAPE");
                int res = 128;
                if (const char* r = std::getenv("CAX_PREVIEW_RES")) res = atoi(r);
                s_prev = new tt::SdfPreview();
                bool ok = false;
                if (shape && shape[0]) ok = s_prev->BuildFromStep(shape, res);
                else printf("[sdfprev] set CAX_PREVIEW_SHAPE=<path to ascii .step> to build\n");
                if (!ok) { s_prev_failed = true; delete s_prev; s_prev = nullptr; }
            }
            if (s_prev && s_prev->IsReady())
            {
                int fw = 0, fh = 0; glfwGetFramebufferSize(window, &fw, &fh);
                s_prev->Render(fw, fh, (float)s_prev_frame);
                if (s_prev_frame == 90 && std::getenv("CAX_PREVIEW_DUMP")) {
                    tt::SdfPreview::DumpBMP(std::getenv("CAX_PREVIEW_DUMP"), 0, 0, fw, fh);
                }
                ++s_prev_frame;
            }
        }
#endif

#ifdef __APPLE__
            // Metal present: EndFrame() presents the drawable + commits the command buffer
            // (the GL path's glfwSwapBuffers has no equivalent without a GL context).
            if (auto ctx = tt::Render::Instance()->Context()) { ctx->Flush(); }
#else
            glfwSwapBuffers(window);
#endif
            stat_last_draw_ms = (glfwGetTime() - draw_t0) * 1000.0;
            ++stat_drawn;
        }
        else
        {
            ++stat_skipped;
        }

        double stat_now = glfwGetTime();
        if (stat_now - stat_t0 >= 2.0)
        {
            std::cout << "[render-on-demand] drew " << stat_drawn << " skipped " << stat_skipped
                      << " in " << (stat_now - stat_t0) << "s, last draw " << stat_last_draw_ms << "ms"
                      << std::endl;
            stat_drawn = 0;
            stat_skipped = 0;
            stat_t0 = stat_now;
        }

        //show_fps(window);
        limit_fps(60);
    }

    ves_free_vm();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}