set(_tt_voxel_provider_required
    unirender
    gl3w
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
    dtex
    gtxt
    freetype
    fs
    ds
    texpack
    sm)
set(_tt_voxel_provider_missing "")
foreach(_tt_voxel_provider_tgt IN LISTS _tt_voxel_provider_required)
    if(NOT TARGET ${_tt_voxel_provider_tgt})
        list(APPEND _tt_voxel_provider_missing ${_tt_voxel_provider_tgt})
    endif()
endforeach()
if(_tt_voxel_provider_missing)
    message(WARNING
        "Tantien voxel providers skipped; missing targets: ${_tt_voxel_provider_missing}")
    return()
endif()

if(NOT TARGET tantien::voxel_render_stack)
    add_library(tantien_voxel_render_stack INTERFACE)
    add_library(tantien::voxel_render_stack ALIAS tantien_voxel_render_stack)
    target_compile_definitions(tantien_voxel_render_stack INTERFACE NO_BOOST)
    target_link_libraries(tantien_voxel_render_stack INTERFACE
        unirender
        gl3w
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
        SPIRV-Tools-opt)
    if(TARGET Vulkan::Vulkan)
        target_link_libraries(tantien_voxel_render_stack INTERFACE Vulkan::Vulkan)
    endif()
endif()

if(NOT TARGET tantien::voxel_hud_support)
    add_library(tantien_voxel_hud_support STATIC
        "${CMAKE_CURRENT_SOURCE_DIR}/src/modules/filesystem/ResourceUID.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/modules/graphics/DTex.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/modules/graphics/GtxtCore.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/modules/graphics/LoadingList.cpp")
    add_library(tantien::voxel_hud_support ALIAS tantien_voxel_hud_support)
    target_include_directories(tantien_voxel_hud_support PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_compile_definitions(tantien_voxel_hud_support PUBLIC NO_BOOST)
    target_compile_features(tantien_voxel_hud_support PUBLIC cxx_std_17)
    target_link_libraries(tantien_voxel_hud_support PUBLIC
        dtex
        gtxt
        unirender
        sm
        shadertrans
        freetype
        fs
        ds
        texpack)
    if(MSVC)
        target_compile_options(tantien_voxel_hud_support PRIVATE
            /utf-8 /Zc:__cplusplus)
    endif()
endif()
