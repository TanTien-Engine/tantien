if(NOT TT_BUILD_HOST_HUD_TESTS)
    return()
endif()

if(NOT WIN32)
    message(WARNING
        "TT_BUILD_HOST_HUD_TESTS is Windows/WGL-only; skipping host HUD tests")
    return()
endif()

set(_tt_host_hud_required_targets
    dtex
    gtxt
    freetype
    fs
    ds
    texpack
    unirender
    shadertrans
    spirv-cross-core
    spirv-cross-glsl
    spirv-cross-util
    glslang
    SPIRV
    SpvGenTwoCommon
    SpvGenTwoLib
    SPIRV-Tools
    SPIRV-Tools-link
    SPIRV-Tools-opt
    gl3w
    glfw
    sm
    tessellation)
set(_tt_host_hud_missing "")
foreach(_tt_host_hud_tgt IN LISTS _tt_host_hud_required_targets)
    if(NOT TARGET ${_tt_host_hud_tgt})
        list(APPEND _tt_host_hud_missing ${_tt_host_hud_tgt})
    endif()
endforeach()
if(NOT TARGET Vulkan::Vulkan)
    list(APPEND _tt_host_hud_missing Vulkan::Vulkan)
endif()
if(_tt_host_hud_missing)
    message(WARNING
        "TT_BUILD_HOST_HUD_TESTS skipped; missing targets: ${_tt_host_hud_missing}")
    return()
endif()

set(_tt_host_hud_root "${CMAKE_CURRENT_SOURCE_DIR}")
set(_tt_host_hud_voxel_root "${_tt_host_hud_root}/thirdparty/voxel")

function(tt_host_hud_apply_msvc target_name)
    if(MSVC)
        target_compile_options(${target_name} PRIVATE /utf-8 /Zc:__cplusplus)
    endif()
endfunction()

function(tt_host_hud_link_gl_closure target_name)
    target_link_libraries(${target_name} PRIVATE
        glfw
        gl3w
        unirender
        shadertrans
        spirv-cross-core
        spirv-cross-glsl
        spirv-cross-util
        glslang
        SPIRV
        SpvGenTwoCommon
        SpvGenTwoLib
        SPIRV-Tools
        SPIRV-Tools-link
        SPIRV-Tools-opt
        Vulkan::Vulkan
        gtxt
        freetype
        fs
        ds
        texpack
        sm)
endfunction()

function(tt_host_hud_register_gl_test test_name)
    if(NOT BUILD_TESTING)
        return()
    endif()
    set_tests_properties(${test_name} PROPERTIES
        RUN_SERIAL TRUE
        TIMEOUT 90
        WORKING_DIRECTORY "${_tt_host_hud_root}")
endfunction()

add_library(tt_host_hud_ui STATIC
    "${_tt_host_hud_root}/src/modules/filesystem/ResourceUID.cpp"
    "${_tt_host_hud_root}/src/modules/graphics/GtxtCore.cpp"
    "${_tt_host_hud_voxel_root}/src/voxel_game/ui/GtxtHud.cpp"
    "${_tt_host_hud_voxel_root}/src/voxel_game/ui/HudBatch.cpp"
    "${_tt_host_hud_voxel_root}/src/voxel_game/ui/HudGlyphCache.cpp"
    "${_tt_host_hud_voxel_root}/src/voxel_game/ui/HudLayout.cpp"
    "${_tt_host_hud_voxel_root}/src/voxel_game/ui/HudQuadRenderer.cpp"
    "${_tt_host_hud_voxel_root}/src/voxel_game/ui/HudUtf8.cpp")
target_include_directories(tt_host_hud_ui PUBLIC
    "${_tt_host_hud_root}/src"
    "${_tt_host_hud_voxel_root}/src")
target_compile_definitions(tt_host_hud_ui PUBLIC NO_BOOST)
target_compile_features(tt_host_hud_ui PUBLIC cxx_std_17)
target_link_libraries(tt_host_hud_ui PUBLIC
    gtxt
    unirender
    sm
    shadertrans
    freetype
    fs
    ds
    texpack)
tt_host_hud_apply_msvc(tt_host_hud_ui)

add_library(tt_host_hud_support STATIC
    "${_tt_host_hud_root}/src/modules/graphics/DTex.cpp"
    "${_tt_host_hud_root}/src/modules/graphics/LoadingList.cpp")
target_compile_definitions(tt_host_hud_support PUBLIC NO_BOOST)
target_compile_features(tt_host_hud_support PUBLIC cxx_std_17)
target_link_libraries(tt_host_hud_support PUBLIC
    tt_host_hud_ui
    dtex)
tt_host_hud_apply_msvc(tt_host_hud_support)

add_library(dtex_test_seams STATIC
    "${_tt_host_hud_root}/thirdparty/dtex/source/PixBufPage.cpp"
    "${_tt_host_hud_root}/thirdparty/dtex/source/PixelBuffer.cpp"
    "${_tt_host_hud_root}/thirdparty/dtex/source/TexBufBlock.cpp"
    "${_tt_host_hud_root}/thirdparty/dtex/source/TexBufDrawTask.cpp"
    "${_tt_host_hud_root}/thirdparty/dtex/source/TexBufNode.cpp"
    "${_tt_host_hud_root}/thirdparty/dtex/source/TexBufPreNode.cpp"
    "${_tt_host_hud_root}/thirdparty/dtex/source/TexPacker.cpp"
    "${_tt_host_hud_root}/thirdparty/dtex/source/TexRenderer.cpp"
    "${_tt_host_hud_root}/thirdparty/dtex/source/TextureBuffer.cpp"
    "${_tt_host_hud_root}/src/modules/graphics/DTex.cpp"
    "${_tt_host_hud_root}/src/modules/graphics/LoadingList.cpp")
target_include_directories(dtex_test_seams PUBLIC
    "${_tt_host_hud_root}/src"
    "${_tt_host_hud_root}/thirdparty/dtex/include")
target_compile_definitions(dtex_test_seams PUBLIC
    DTEX_ENABLE_TEST_SEAMS
    NO_BOOST)
target_compile_features(dtex_test_seams PUBLIC cxx_std_17)
target_link_libraries(dtex_test_seams
    PUBLIC
        unirender
        sm
        gtxt
    PRIVATE
        texpack
        shadertrans)
tt_host_hud_apply_msvc(dtex_test_seams)

add_executable(gtxt_thread_tests
    "${_tt_host_hud_root}/test/voxel_game/gtxt_thread_tests.cpp"
    "${_tt_host_hud_root}/src/modules/graphics/GTxt.cpp")
target_link_libraries(gtxt_thread_tests PRIVATE
    tt_host_hud_support
    tessellation)
tt_host_hud_link_gl_closure(gtxt_thread_tests)
target_compile_features(gtxt_thread_tests PRIVATE cxx_std_17)
tt_host_hud_apply_msvc(gtxt_thread_tests)
set_target_properties(gtxt_thread_tests PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${_tt_host_hud_root}")

add_executable(hud_core_tests
    "${_tt_host_hud_root}/test/voxel_game/hud_core_tests.cpp")
target_link_libraries(hud_core_tests PRIVATE tt_host_hud_support)
tt_host_hud_link_gl_closure(hud_core_tests)
target_compile_features(hud_core_tests PRIVATE cxx_std_17)
tt_host_hud_apply_msvc(hud_core_tests)
set_target_properties(hud_core_tests PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${_tt_host_hud_root}")

add_executable(hud_loading_list_tests
    "${_tt_host_hud_root}/test/voxel_game/hud_loading_list_tests.cpp")
target_link_libraries(hud_loading_list_tests PRIVATE dtex_test_seams)
tt_host_hud_link_gl_closure(hud_loading_list_tests)
target_compile_features(hud_loading_list_tests PRIVATE cxx_std_17)
tt_host_hud_apply_msvc(hud_loading_list_tests)
set_target_properties(hud_loading_list_tests PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${_tt_host_hud_root}")

add_executable(hud_gl_smoke_tests
    "${_tt_host_hud_root}/test/voxel_game/hud_gl_smoke_tests.cpp")
target_link_libraries(hud_gl_smoke_tests PRIVATE
    tt_host_hud_ui
    dtex_test_seams)
tt_host_hud_link_gl_closure(hud_gl_smoke_tests)
target_compile_features(hud_gl_smoke_tests PRIVATE cxx_std_17)
tt_host_hud_apply_msvc(hud_gl_smoke_tests)
set_target_properties(hud_gl_smoke_tests PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${_tt_host_hud_root}")

if(BUILD_TESTING)
    add_test(NAME gtxt_thread_tests COMMAND gtxt_thread_tests)
    tt_host_hud_register_gl_test(gtxt_thread_tests)

    add_test(NAME hud_core_tests COMMAND hud_core_tests)
    set_tests_properties(hud_core_tests PROPERTIES
        TIMEOUT 60
        WORKING_DIRECTORY "${_tt_host_hud_root}")

    add_test(NAME hud_gl_smoke_tests COMMAND hud_gl_smoke_tests)
    tt_host_hud_register_gl_test(hud_gl_smoke_tests)

    foreach(_tt_seam_case IN ITEMS null oversize invalid deferred exception large_batch)
        add_test(NAME hud_loading_list_${_tt_seam_case}
                 COMMAND hud_loading_list_tests ${_tt_seam_case})
        tt_host_hud_register_gl_test(hud_loading_list_${_tt_seam_case})
    endforeach()
endif()
