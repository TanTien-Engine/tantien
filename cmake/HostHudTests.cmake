if(NOT TT_BUILD_HOST_HUD_TESTS)
    return()
endif()

if(NOT WIN32)
    message(WARNING
        "TT_BUILD_HOST_HUD_TESTS is Windows/WGL-only; skipping host HUD tests")
    return()
endif()

if(NOT TARGET tantien::voxel_hud_support OR NOT TARGET tantien::voxel_render_stack)
    message(WARNING
        "TT_BUILD_HOST_HUD_TESTS skipped; Tantien voxel providers are missing")
    return()
endif()

if(NOT TARGET glfw)
    message(WARNING "TT_BUILD_HOST_HUD_TESTS skipped; missing target glfw")
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
        tantien::voxel_render_stack
        glfw
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

# The GL smoke test needs DTex's failure seams. Keep its HUD adapter separate
# from voxel_hud_tantien so the executable cannot also pull the production
# DTex.cpp/LoadingList.cpp objects through tantien::voxel_hud_support.
add_library(tt_host_hud_test_adapter STATIC
    "${_tt_host_hud_root}/src/modules/filesystem/ResourceUID.cpp"
    "${_tt_host_hud_root}/src/modules/graphics/GtxtCore.cpp"
    "${_tt_host_hud_voxel_root}/src/voxel_game/ui/GtxtHud.cpp"
    "${_tt_host_hud_voxel_root}/src/voxel_game/ui/HudQuadRenderer.cpp")
target_include_directories(tt_host_hud_test_adapter PUBLIC
    "${_tt_host_hud_root}/src"
    "${_tt_host_hud_voxel_root}/src")
target_compile_definitions(tt_host_hud_test_adapter PUBLIC NO_BOOST)
target_compile_features(tt_host_hud_test_adapter PUBLIC cxx_std_17)
target_link_libraries(tt_host_hud_test_adapter PUBLIC
    voxel_hud_core
    dtex_test_seams
    tantien::voxel_render_stack
    gtxt
    freetype
    fs
    ds
    texpack
    sm)
tt_host_hud_apply_msvc(tt_host_hud_test_adapter)

if(TARGET tessellation)
    add_executable(gtxt_thread_tests
        "${_tt_host_hud_root}/test/host_hud/gtxt_thread_tests.cpp"
        "${_tt_host_hud_root}/src/modules/graphics/GTxt.cpp")
    target_link_libraries(gtxt_thread_tests PRIVATE
        tantien::voxel_hud_support
        tessellation)
    tt_host_hud_link_gl_closure(gtxt_thread_tests)
    target_compile_features(gtxt_thread_tests PRIVATE cxx_std_17)
    tt_host_hud_apply_msvc(gtxt_thread_tests)
    set_target_properties(gtxt_thread_tests PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "${_tt_host_hud_root}")
endif()

add_executable(hud_loading_list_tests
    "${_tt_host_hud_root}/test/host_hud/hud_loading_list_tests.cpp")
target_link_libraries(hud_loading_list_tests PRIVATE dtex_test_seams)
tt_host_hud_link_gl_closure(hud_loading_list_tests)
target_compile_features(hud_loading_list_tests PRIVATE cxx_std_17)
tt_host_hud_apply_msvc(hud_loading_list_tests)
set_target_properties(hud_loading_list_tests PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${_tt_host_hud_root}")

if(TARGET voxel_hud_tantien)
    add_executable(hud_core_tests
        "${_tt_host_hud_root}/test/host_hud/hud_core_tests.cpp")
    target_link_libraries(hud_core_tests PRIVATE
        voxel_hud_tantien
        tantien::voxel_hud_support)
    tt_host_hud_link_gl_closure(hud_core_tests)
    target_compile_features(hud_core_tests PRIVATE cxx_std_17)
    tt_host_hud_apply_msvc(hud_core_tests)
    set_target_properties(hud_core_tests PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "${_tt_host_hud_root}")

    add_executable(hud_gl_smoke_tests
        "${_tt_host_hud_root}/test/host_hud/hud_gl_smoke_tests.cpp")
    target_link_libraries(hud_gl_smoke_tests PRIVATE
        tt_host_hud_test_adapter)
    tt_host_hud_link_gl_closure(hud_gl_smoke_tests)
    target_compile_features(hud_gl_smoke_tests PRIVATE cxx_std_17)
    tt_host_hud_apply_msvc(hud_gl_smoke_tests)
    set_target_properties(hud_gl_smoke_tests PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "${_tt_host_hud_root}")
else()
    message(WARNING
        "TT_BUILD_HOST_HUD_TESTS: voxel_hud_tantien missing; "
        "hud_core_tests and hud_gl_smoke_tests were not created")
endif()

if(BUILD_TESTING)
    if(TARGET gtxt_thread_tests)
        add_test(NAME gtxt_thread_tests COMMAND gtxt_thread_tests)
        tt_host_hud_register_gl_test(gtxt_thread_tests)
    endif()

    add_test(NAME hud_loading_list_null COMMAND hud_loading_list_tests null)
    add_test(NAME hud_loading_list_oversize COMMAND hud_loading_list_tests oversize)
    add_test(NAME hud_loading_list_invalid COMMAND hud_loading_list_tests invalid)
    add_test(NAME hud_loading_list_deferred COMMAND hud_loading_list_tests deferred)
    add_test(NAME hud_loading_list_exception COMMAND hud_loading_list_tests exception)
    add_test(NAME hud_loading_list_large_batch COMMAND hud_loading_list_tests large_batch)
    foreach(_tt_seam_case IN ITEMS null oversize invalid deferred exception large_batch)
        tt_host_hud_register_gl_test(hud_loading_list_${_tt_seam_case})
    endforeach()

    if(TARGET hud_core_tests)
        add_test(NAME hud_core_tests COMMAND hud_core_tests)
        set_tests_properties(hud_core_tests PROPERTIES
            TIMEOUT 60
            WORKING_DIRECTORY "${_tt_host_hud_root}")
    endif()

    if(TARGET hud_gl_smoke_tests)
        add_test(NAME hud_gl_smoke_tests COMMAND hud_gl_smoke_tests)
        tt_host_hud_register_gl_test(hud_gl_smoke_tests)
    endif()
endif()
