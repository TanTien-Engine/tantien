#include <unirender/Factory.h>
#include <unirender/Device.h>
#include <unirender/Context.h>
#include <unirender/ShaderProgram.h>
#include <unirender/VertexBuffer.h>
#include <unirender/VertexArray.h>
#include <unirender/ComponentDataType.h>
#include <unirender/VertexInputAttribute.h>
#include <shadertrans/ShaderTrans.h>

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <string>
#include <vector>

#if DrawState
#undef DrawState
#endif
#include <unirender/DrawState.h>

static void error_callback(int error, const char *msg) {
    std::cerr << "GLWT error " << error << ": " << msg << std::endl;
}

int main()
{
    int width = 640;
    int height = 480;

    glfwSetErrorCallback(error_callback);

    if(glfwInit() == GL_FALSE) {
        std::cerr << "failed to init GLFW" << std::endl;
        return 1;
    }

    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    GLFWwindow *window;
    if((window = glfwCreateWindow(width, height, "glfw3", 0, 0)) == 0) {
        std::cerr << "failed to open window" << std::endl;
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);

    if(gl3wInit()) {
        std::cerr << "failed to init GL3W" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    std::shared_ptr<ur::Device> dev = ur::CreateDevice(ur::APIType::OpenGL);
    dev->Init();

    auto ctx = ur::CreateContext(ur::APIType::OpenGL, *dev, nullptr);

    std::string vertex_source =
        "#version 330\n"
        "layout(location = 0) in vec4 vposition;\n"
        "layout(location = 1) in vec4 vcolor;\n"
        "out vec4 fcolor;\n"
        "void main() {\n"
        "   fcolor = vcolor;\n"
        "   gl_Position = vposition;\n"
        "}\n";

    std::string fragment_source =
        "#version 330\n"
        "in vec4 fcolor;\n"
        "layout(location = 0) out vec4 FragColor;\n"
        "void main() {\n"
        "   FragColor = fcolor;\n"
        "}\n";

    std::vector<unsigned int> vs, fs;
    shadertrans::ShaderTrans::GLSL2SpirV(shadertrans::ShaderStage::VertexShader, vertex_source, vs);
    shadertrans::ShaderTrans::GLSL2SpirV(shadertrans::ShaderStage::PixelShader, fragment_source, fs);
    auto prog = dev->CreateShaderProgram(vs, fs);

    GLfloat vertexData[] = {
    //  X     Y     Z           R     G     B
       1.0f, 1.0f, 0.0f,       1.0f, 0.0f, 0.0f, // vertex 0
      -1.0f, 1.0f, 0.0f,       0.0f, 1.0f, 0.0f, // vertex 1
       1.0f,-1.0f, 0.0f,       0.0f, 0.0f, 1.0f, // vertex 2
       1.0f,-1.0f, 0.0f,       0.0f, 0.0f, 1.0f, // vertex 3
      -1.0f, 1.0f, 0.0f,       0.0f, 1.0f, 0.0f, // vertex 4
      -1.0f,-1.0f, 0.0f,       1.0f, 0.0f, 0.0f, // vertex 5
    };

    auto va = dev->CreateVertexArray();

    auto vbuf_sz = sizeof(float) * 6 * 6;
    auto vbuf = dev->CreateVertexBuffer(ur::BufferUsageHint::StaticDraw, vbuf_sz);
    vbuf->ReadFromMemory(vertexData, vbuf_sz, 0);
    va->SetVertexBuffer(vbuf);

    int stride_in_bytes = 4 * 3 + 4 * 3;
    std::vector<std::shared_ptr<ur::VertexInputAttribute>> vbuf_attrs;
    vbuf_attrs.push_back(std::make_shared<ur::VertexInputAttribute>(
        0, ur::ComponentDataType::Float, 3, 0, stride_in_bytes
    ));
    vbuf_attrs.push_back(std::make_shared<ur::VertexInputAttribute>(
        1, ur::ComponentDataType::Float, 3, 4 * 3, stride_in_bytes
    ));
    va->SetVertexBufferAttrs(vbuf_attrs);

    while(!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        ur::DrawState ds;
        ds.program = prog;
        ds.vertex_array = va;
        ds.render_state.depth_test.enabled = false;
        ctx->Draw(ur::PrimitiveType::Triangles, ds, nullptr);

        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}