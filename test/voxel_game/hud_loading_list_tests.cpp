#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef DTEX_ENABLE_TEST_SEAMS
#error hud_loading_list_tests requires a consistently seam-enabled DTex build
#endif

#include "modules/graphics/DTex.h"
#include "modules/graphics/LoadingList.h"

#include <dtex/PixelBuffer.h>
#include <dtex/TexBufBlock.h>
#include <dtex/TexBufPreNode.h>
#include <dtex/TexRenderer.h>
#include <dtex/TextureBuffer.h>

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include <unirender/Context.h>
#include <unirender/Device.h>
#include <unirender/Texture.h>
#include <unirender/TextureDescription.h>
#include <unirender/opengl/Context.h>
#include <unirender/opengl/Device.h>

#include <array>
#include <cstdint>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{

void require(bool condition, const char* message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

void require_gl(const char* where)
{
    const GLenum error = glGetError();
    if (error != GL_NO_ERROR)
    {
        throw std::runtime_error(std::string(where) + " GL error " + std::to_string(error));
    }
}

ur::TexturePtr make_texture(ur::Device& device, int width, int height)
{
    ur::TextureDescription desc;
    desc.width = width;
    desc.height = height;
    desc.format = ur::TextureFormat::RGBA8;
    std::vector<unsigned char> pixels(
        static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4, 255);
    return device.CreateTexture(desc, pixels.data());
}

dtex::Rect make_rect(int xmin, int ymin, int xmax, int ymax)
{
    dtex::Rect rect;
    rect.xmin = static_cast<std::int16_t>(xmin);
    rect.ymin = static_cast<std::int16_t>(ymin);
    rect.xmax = static_cast<std::int16_t>(xmax);
    rect.ymax = static_cast<std::int16_t>(ymax);
    return rect;
}

void test_null_symbol(tt::LoadingList& list, ur::Context& context)
{
    list.AddSymbol(0xabcull, nullptr, sm::irect(0, 0, 8, 8));
    const tt::DTexFlushResult first = list.Flush(context);
    require(!first.success, "null symbol flush fails");
    const tt::DTexFlushResult retry = list.Flush(context);
    require(!retry.success && retry.had_work, "null symbol stays queued for retry");
}

void test_oversize_symbol(tt::LoadingList& list, ur::Device& device, ur::Context& context)
{
    ur::TextureDescription stamp;
    stamp.width = 16;
    stamp.height = 16;
    stamp.format = ur::TextureFormat::RGBA8;
    std::vector<unsigned char> pixels(16 * 16 * 4, 255);
    auto tex = device.CreateTexture(stamp, pixels.data());
    require(static_cast<bool>(tex), "stamp texture");

    list.AddSymbol(0xdefull, tex, sm::irect(0, 0, 600, 600));
    const tt::DTexFlushResult first = list.Flush(context);
    require(!first.success, "oversize symbol flush fails");
    const tt::DTexFlushResult retry = list.Flush(context);
    require(!retry.success && retry.had_work, "oversize symbol stays queued for retry");
}

void test_invalid_texture_buffer_inputs(ur::Device& device, ur::Context& context)
{
    dtex::TexRenderer renderer(device);
    dtex::TextureBuffer buffer(device, 16, 16);
    auto texture = make_texture(device, 16, 16);
    require(static_cast<bool>(texture), "invalid-input texture");

    const dtex::Rect valid = make_rect(0, 0, 4, 4);
    require(!buffer.Load(texture, valid, 1), "Load outside a scope is rejected");
    require(!buffer.LoadFinish(context, renderer), "unpaired LoadFinish is rejected");
    require(buffer.LoadStart(), "valid LoadStart");

    require(!buffer.Load(texture, make_rect(0, 0, 0, 4), 2), "empty region rejected");
    require(!buffer.Load(texture, make_rect(4, 0, 2, 4), 3), "reversed region rejected");
    require(!buffer.Load(texture, make_rect(0, 0, 17, 4), 4), "source overflow rejected");
    require(!buffer.Load(texture, valid, 5, -1, 0, 0), "negative padding rejected");
    require(!buffer.Load(texture, valid, 6, 0, -1, 0), "negative extrude rejected");
    require(!buffer.Load(texture, valid, 7, 0, 0, -1), "negative source extrude rejected");
    require(!buffer.Load(texture, make_rect(0, 0, 4, 4), 70, 0, 0, 1),
            "source extrude before origin rejected");
    require(!buffer.Load(texture, make_rect(0, 0, 16, 16), 71, 0, 0, 1),
            "source extrude past texture rejected");
    require(!buffer.Load(texture, valid, 8, std::numeric_limits<int>::max(), 0, 0),
            "padding overflow rejected");
    require(buffer.Load(texture, valid, 9), "valid region accepted");
    require(buffer.LoadFinish(context, renderer), "valid scope finishes");

    int block_id = -1;
    require(buffer.Query(9, block_id) != nullptr, "valid key published");
    require(!buffer.LoadFinish(context, renderer), "second unpaired finish rejected");
    require(buffer.LoadStart(), "abort scope starts");
    require(buffer.AbortLoad(), "abort closes scope");
    require(!buffer.AbortLoad(), "second abort rejected");

    require(buffer.LoadStart() && buffer.LoadStart(), "nested scopes start");
    require(buffer.Load(texture, make_rect(4, 0, 8, 4), 72), "nested key accepted");
    require(buffer.LoadFinish(context, renderer), "inner scope closes");
    require(buffer.Query(72, block_id) == nullptr, "inner finish does not publish");
    require(buffer.LoadFinish(context, renderer), "outer scope publishes");
    require(buffer.Query(72, block_id) != nullptr, "outer finish published nested key");

    dtex::TexBufBlock block(texture, 0, 0, 8, 8);
    dtex::TexBufPreNode block_node(texture, valid, 73, 0, 0, 0);
    require(block.Insert(block_node, 0).rect.IsValid(), "block active pack succeeds");
    block.Insert(73, 0);
    block.CommitShadow();
    require(block.Query(73) == 0, "CommitShadow without shadow preserves active lookup");
    require(block.InsertShadow(block_node, 0).rect.IsValid(), "block shadow exists");
    block.Clear();
    require(!block.HasShadow() && block.Query(73) == -1, "Clear removes active and shadow state");
    require_gl("invalid inputs");
}

void test_pixel_buffer_deferred_retry(ur::Device& device, ur::Context& context)
{
    dtex::PixelBuffer pixels(device, 32, 32);
    dtex::TextureBuffer atlas(device, 8, 8);
    dtex::TexRenderer renderer(device);
    const std::array<std::uint32_t, 4> bitmap = {
        0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu
    };

    for (std::uint64_t key = 1; key <= 5; ++key)
    {
        pixels.Load(device, context, bitmap.data(), 2, 2, key);
    }

    const dtex::PixelBuffer::FlushResult first = pixels.Flush(context, atlas, renderer);
    require(first.had_work && !first.success, "first flush reports one deferred glyph");
    int block_id = -1;
    int published = 0;
    for (std::uint64_t key = 1; key <= 5; ++key)
    {
        if (atlas.Query(key, block_id))
        {
            ++published;
        }
    }
    require(published == 4, "first flush publishes exactly four glyphs");

    const dtex::PixelBuffer::FlushResult retry = pixels.Flush(context, atlas, renderer);
    require(retry.had_work && retry.success, "second Flush drives deferred glyph without Query");
    require(atlas.Query(5, block_id) != nullptr, "deferred glyph published on automatic retry");
    require_gl("deferred retry");
}

void test_exception_retry(ur::Device& device, ur::Context& context)
{
    dtex::TextureBuffer atlas(device, 8, 8);
    dtex::TexRenderer renderer(device);
    auto texture = make_texture(device, 4, 4);
    require(static_cast<bool>(texture), "exception texture");
    const dtex::Rect full = make_rect(0, 0, 4, 4);

    require(atlas.LoadStart(), "baseline scope starts");
    require(atlas.Load(texture, full, 10), "baseline key accepted");
    require(atlas.LoadFinish(context, renderer), "baseline key published");

    atlas.FailNextQueueDrawForTest(1);
    require(atlas.LoadStart(), "queue exception scope starts");
    require(atlas.Load(texture, full, 11), "queue exception key accepted");
    require(!atlas.LoadFinish(context, renderer), "queue bad_alloc becomes retryable failure");
    int block_id = -1;
    require(atlas.Query(10, block_id) != nullptr, "queue exception preserves old key");
    require(atlas.Query(11, block_id) == nullptr, "queue exception exposes no partial key");
    require(atlas.LoadStart(), "queue retry starts");
    require(atlas.LoadFinish(context, renderer), "queue retry succeeds without re-Load");
    require(atlas.Query(11, block_id) != nullptr, "queue retry publishes key");

    require(atlas.LoadStart(), "fill scope starts");
    require(atlas.Load(texture, full, 12), "fill key 12");
    require(atlas.Load(texture, full, 13), "fill key 13");
    require(atlas.LoadFinish(context, renderer), "four blocks filled");

    atlas.FailNextPublishForTest(1);
    require(atlas.LoadStart(), "publish exception scope starts");
    require(atlas.Load(texture, full, 14), "evicting key accepted");
    require(!atlas.LoadFinish(context, renderer), "publish bad_alloc becomes retryable failure");
    require(atlas.Query(14, block_id) == nullptr, "publish exception exposes no partial key");
    for (std::uint64_t key = 10; key <= 13; ++key)
    {
        require(atlas.Query(key, block_id) != nullptr,
                "publish exception preserves every old key");
    }
    require(atlas.LoadStart(), "publish retry starts");
    require(atlas.LoadFinish(context, renderer), "publish retry succeeds without re-Load");
    require(atlas.Query(14, block_id) != nullptr, "publish retry publishes key");
    int old_keys_remaining = 0;
    for (std::uint64_t key = 10; key <= 13; ++key)
    {
        if (atlas.Query(key, block_id))
        {
            ++old_keys_remaining;
        }
    }
    require(old_keys_remaining == 3, "publish retry evicts exactly one old key");

    std::vector<std::uint64_t> published_before_gpu_failure;
    for (std::uint64_t key = 10; key <= 14; ++key)
    {
        if (atlas.Query(key, block_id))
        {
            published_before_gpu_failure.push_back(key);
        }
    }
    require(published_before_gpu_failure.size() == 4,
            "atlas contains four keys before post-clear GPU failure");

    renderer.FailNextGpuFlushForTest(1);
    require(atlas.LoadStart(), "post-clear GPU failure scope starts");
    require(atlas.Load(texture, full, 15), "post-clear GPU failure key accepted");
    require(!atlas.LoadFinish(context, renderer), "post-clear GPU failure is retryable");
    require(atlas.Query(15, block_id) == nullptr, "failed GPU key is not published");
    int safe_old_keys = 0;
    for (const std::uint64_t key : published_before_gpu_failure)
    {
        if (atlas.Query(key, block_id))
        {
            ++safe_old_keys;
        }
    }
    require(safe_old_keys == 3,
            "cleared eviction victim becomes a miss instead of a stale lookup");
    require(atlas.LoadStart(), "post-clear GPU retry starts");
    require(atlas.LoadFinish(context, renderer), "post-clear GPU retry succeeds");
    require(atlas.Query(15, block_id) != nullptr, "post-clear GPU retry publishes key");

    dtex::PixelBuffer pixels(device, 32, 32);
    dtex::TextureBuffer pixel_atlas(device, 8, 8);
    dtex::TexRenderer pixel_renderer(device);
    const std::array<std::uint32_t, 4> bitmap = {
        0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu
    };

    pixels.FailNextLoadPrepareForTest(1);
    pixels.Load(device, context, bitmap.data(), 2, 2, 100);
    require(!pixels.Exist(100), "failed PixelBuffer Load does not publish Exist");
    auto no_load_work = pixels.Flush(context, pixel_atlas, pixel_renderer);
    require(no_load_work.success && !no_load_work.had_work,
            "failed PixelBuffer Load leaves no hidden pending work");
    pixels.Load(device, context, bitmap.data(), 2, 2, 100);
    require(pixels.Exist(100), "PixelBuffer Load retry commits key");
    auto load_retry = pixels.Flush(context, pixel_atlas, pixel_renderer);
    require(load_retry.success && load_retry.had_work,
            "PixelBuffer Load retry publishes normally");
    require(pixel_atlas.Query(100, block_id) != nullptr,
            "PixelBuffer Load retry reaches atlas lookup");

    ur::TexturePtr page_texture;
    require(!pixels.QueryAndInsert(100, nullptr, page_texture) && !page_texture,
            "QueryAndInsert strictly rejects null texcoords");
    auto null_output_flush = pixels.Flush(context, pixel_atlas, pixel_renderer);
    require(null_output_flush.success && !null_output_flush.had_work,
            "null texcoords rejection queues no hidden work");

    float texcoords[8] = {};
    pixels.FailNextQueryQueueForTest(1);
    require(!pixels.QueryAndInsert(100, texcoords, page_texture) && !page_texture,
            "QueryAndInsert catches pending allocation failure");
    auto failed_query_flush = pixels.Flush(context, pixel_atlas, pixel_renderer);
    require(failed_query_flush.success && !failed_query_flush.had_work,
            "failed QueryAndInsert leaves no hidden pending work");
    require(pixels.QueryAndInsert(100, texcoords, page_texture) && page_texture,
            "QueryAndInsert retry succeeds");
    auto query_retry = pixels.Flush(context, pixel_atlas, pixel_renderer);
    require(query_retry.success && query_retry.had_work,
            "QueryAndInsert retry is observable and drains");
    require_gl("exception retry");
}

void test_large_renderer_batch(ur::Device& device, ur::Context& context)
{
    dtex::TexRenderer renderer(device);
    auto source = make_texture(device, 8, 8);
    auto destination = make_texture(device, 8, 8);
    require(static_cast<bool>(source) && static_cast<bool>(destination),
            "large-batch textures");
    const dtex::Rect rect = make_rect(0, 0, 8, 8);

    for (int i = 0; i < 17000; ++i)
    {
        require(renderer.Draw(context, source, rect, destination, rect, false),
                "large-batch quad accepted");
    }
    require(renderer.Flush(context), "32-bit large batch flushes");
    require_gl("large renderer batch");
}

} // namespace

int main(int argc, char* argv[])
{
    try
    {
        require(argc == 2,
                "usage: hud_loading_list_tests <null|oversize|invalid|deferred|exception|large_batch>");
        const std::string test_case = argv[1];
        require(test_case == "null" || test_case == "oversize" || test_case == "invalid" ||
                test_case == "deferred" || test_case == "exception" ||
                test_case == "large_batch", "unknown test case");

        if (glfwInit() == GLFW_FALSE)
        {
            throw std::runtime_error("glfwInit failed");
        }
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        GLFWwindow* window = glfwCreateWindow(64, 64, "hud_loading_list", nullptr, nullptr);
        require(window != nullptr, "glfwCreateWindow");
        glfwMakeContextCurrent(window);
        require(gl3wInit() == 0, "gl3wInit");

        std::shared_ptr<ur::Device> device = std::make_shared<ur::opengl::Device>();
        require(static_cast<bool>(device), "OpenGL device");
        device->Init();
        std::shared_ptr<ur::Context> context = std::make_shared<ur::opengl::Context>(*device);
        require(static_cast<bool>(context), "OpenGL context");
        require(tt::DTex::Instance()->Init(*device), "DTex::Init");

        auto* list = tt::LoadingList::Instance();
        if (test_case == "null")
        {
            test_null_symbol(*list, *context);
        }
        else if (test_case == "oversize")
        {
            test_oversize_symbol(*list, *device, *context);
        }
        else if (test_case == "invalid")
        {
            test_invalid_texture_buffer_inputs(*device, *context);
        }
        else if (test_case == "deferred")
        {
            test_pixel_buffer_deferred_retry(*device, *context);
        }
        else if (test_case == "exception")
        {
            test_exception_retry(*device, *context);
        }
        else
        {
            test_large_renderer_batch(*device, *context);
        }

        context.reset();
        device.reset();
        glfwDestroyWindow(window);
        glfwTerminate();
        std::cout << "hud_loading_list_tests (" << test_case << "): PASS" << std::endl;
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "hud_loading_list_tests: FAIL: " << error.what() << std::endl;
        glfwTerminate();
        return 1;
    }
}
