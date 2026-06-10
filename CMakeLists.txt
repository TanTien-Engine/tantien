cmake_minimum_required(VERSION 3.16)

if(NOT CMAKE_POLICY_VERSION_MINIMUM)
    set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
endif()

include("${CMAKE_CURRENT_SOURCE_DIR}/CMakeLocal.cmake" OPTIONAL)

project(tantien)

add_definitions(
    -DNO_RTTR
    -DNO_BOOST
)

################################################################################
# Source groups
################################################################################
set(core
    "src/core/macro.h"
)
source_group("core" FILES ${core})

set(include
    "include/tantien.cpp"
    "include/tantien.h"
)
source_group("include" FILES ${include})

set(modules__db
    "src/modules/db/DB.cpp"
    "src/modules/db/DB.h"
    "src/modules/db/db.ves.inc"
    "src/modules/db/wrap_DB.cpp"
    "src/modules/db/wrap_DB.h"
)
source_group("modules\\db" FILES ${modules__db})

set(modules__db__entity
    "src/modules/db/BRepKey.h"
)
source_group("modules\\db\\entity" FILES ${modules__db__entity})

set(modules__db__tools
    "src/modules/db/BRepSerialize.cpp"
    "src/modules/db/BRepSerialize.h"
    "src/modules/db/RTreeBuilder.cpp"
    "src/modules/db/RTreeBuilder.h"
    "src/modules/db/RTreeUpdate.cpp"
    "src/modules/db/RTreeUpdate.h"
)
source_group("modules\\db\\tools" FILES ${modules__db__tools})

set(modules__db__visitor
    "src/modules/db/PickVisitor.cpp"
    "src/modules/db/PickVisitor.h"
    "src/modules/db/RegionVisitor.cpp"
    "src/modules/db/RegionVisitor.h"
)
source_group("modules\\db\\visitor" FILES ${modules__db__visitor})

set(modules__filesystem
    "src/modules/filesystem/Filesystem.cpp"
    "src/modules/filesystem/Filesystem.h"
    "src/modules/filesystem/filesystem.ves.inc"
    "src/modules/filesystem/ResourceUID.cpp"
    "src/modules/filesystem/ResourceUID.h"
    "src/modules/filesystem/wrap_Filesystem.cpp"
    "src/modules/filesystem/wrap_Filesystem.h"
)
source_group("modules\\filesystem" FILES ${modules__filesystem})

set(modules__geometry
    "src/modules/geometry/geometry.ves.inc"
    "src/modules/geometry/ShapeMaths.cpp"
    "src/modules/geometry/ShapeMaths.h"
    "src/modules/geometry/wrap_Geometry.cpp"
    "src/modules/geometry/wrap_Geometry.h"
)
source_group("modules\\geometry" FILES ${modules__geometry})

set(modules__graph
    "src/modules/graph/graph.ves.inc"
    "src/modules/graph/wrap_Graph.cpp"
    "src/modules/graph/wrap_Graph.h"
)
source_group("modules\\graph" FILES ${modules__graph})

set(modules__graphics
    "src/modules/graphics/DTex.cpp"
    "src/modules/graphics/DTex.h"
    "src/modules/graphics/Graphics.cpp"
    "src/modules/graphics/Graphics.h"
    "src/modules/graphics/graphics.ves.inc"
    "src/modules/graphics/GTxt.cpp"
    "src/modules/graphics/GTxt.h"
    "src/modules/graphics/LoadingList.cpp"
    "src/modules/graphics/LoadingList.h"
    "src/modules/graphics/RenderBuffer.h"
    "src/modules/graphics/SpriteRenderer.cpp"
    "src/modules/graphics/SpriteRenderer.h"
    "src/modules/graphics/Viewport.cpp"
    "src/modules/graphics/Viewport.h"
    "src/modules/graphics/wrap_Graphics.cpp"
    "src/modules/graphics/wrap_Graphics.h"
)
source_group("modules\\graphics" FILES ${modules__graphics})

set(modules__gui
    "src/modules/gui/GUI.cpp"
    "src/modules/gui/GUI.h"
    "src/modules/gui/gui.ves.inc"
    "src/modules/gui/wrap_GUI.cpp"
    "src/modules/gui/wrap_GUI.h"
)
source_group("modules\\gui" FILES ${modules__gui})

set(modules__image
    "src/modules/image/image.ves.inc"
    "src/modules/image/ImageData.h"
    "src/modules/image/wrap_Image.cpp"
    "src/modules/image/wrap_Image.h"
)
source_group("modules\\image" FILES ${modules__image})

set(modules__io
    "src/modules/io/Keyboard.cpp"
    "src/modules/io/Keyboard.h"
    "src/modules/io/keyboard.ves.inc"
    "src/modules/io/wrap_Keyboard.cpp"
    "src/modules/io/wrap_Keyboard.h"
)
source_group("modules\\io" FILES ${modules__io})

set(modules__maths
    "src/modules/maths/float16.cpp"
    "src/modules/maths/float16.h"
    "src/modules/maths/maths.ves.inc"
    "src/modules/maths/wrap_Maths.cpp"
    "src/modules/maths/wrap_Maths.h"
)
source_group("modules\\maths" FILES ${modules__maths})

set(modules__model
    "src/modules/model/Model.cpp"
    "src/modules/model/Model.h"
    "src/modules/model/model.ves.inc"
    "src/modules/model/wrap_Model.cpp"
    "src/modules/model/wrap_Model.h"
)
source_group("modules\\model" FILES ${modules__model})

set(modules__om
    "src/modules/om/om.ves.inc"
    "src/modules/om/wrap_OM.cpp"
    "src/modules/om/wrap_OM.h"
)
source_group("modules\\om" FILES ${modules__om})

set(modules__physics
    "src/modules/physics/Physics.cpp"
    "src/modules/physics/Physics.h"
    "src/modules/physics/physics.ves.inc"
    "src/modules/physics/wrap_Physics.cpp"
    "src/modules/physics/wrap_Physics.h"
)
source_group("modules\\physics" FILES ${modules__physics})

set(modules__regen
    "src/modules/regen/PolyDiff.cpp"
    "src/modules/regen/PolyDiff.h"
    "src/modules/regen/regen.ves.inc"
    "src/modules/regen/wrap_Regen.cpp"
    "src/modules/regen/wrap_Regen.h"
)
source_group("modules\\regen" FILES ${modules__regen})

set(modules__render
    "src/modules/render/Render.cpp"
    "src/modules/render/Render.h"
    "src/modules/render/render.ves.inc"
    "src/modules/render/wrap_Render.cpp"
    "src/modules/render/wrap_Render.h"
)
source_group("modules\\render" FILES ${modules__render})

set(modules__scene
    "src/modules/scene/scene.ves.inc"
    "src/modules/scene/SceneTree.cpp"
    "src/modules/scene/SceneTree.h"
    "src/modules/scene/wrap_Scene.cpp"
    "src/modules/scene/wrap_Scene.h"
)
source_group("modules\\scene" FILES ${modules__scene})

set(modules__shader
    "src/modules/shader/shader.ves.inc"
    "src/modules/shader/wrap_Shader.cpp"
    "src/modules/shader/wrap_Shader.h"
)
source_group("modules\\shader" FILES ${modules__shader})

set(modules__system
    "src/modules/system/System.cpp"
    "src/modules/system/System.h"
    "src/modules/system/system.ves.inc"
    "src/modules/system/wrap_System.cpp"
    "src/modules/system/wrap_System.h"
)
source_group("modules\\system" FILES ${modules__system})

set(modules__vm
    "src/modules/vm/vm.ves.inc"
    "src/modules/vm/wrap_VM.cpp"
    "src/modules/vm/wrap_VM.h"
)
source_group("modules\\vm" FILES ${modules__vm})

set(ALL_FILES
    ${core}
    ${include}
    ${modules__db}
    ${modules__db__entity}
    ${modules__db__tools}
    ${modules__db__visitor}
    ${modules__filesystem}
    ${modules__geometry}
    ${modules__graph}
    ${modules__graphics}
    ${modules__gui}
    ${modules__image}
    ${modules__io}
    ${modules__maths}
    ${modules__model}
    ${modules__om}
    ${modules__physics}
    ${modules__regen}
    ${modules__render}
    ${modules__scene}
    ${modules__shader}
    ${modules__system}
    ${modules__vm}
)

set(CMAKE_CXX_STANDARD 17)

add_library(${PROJECT_NAME} STATIC ${ALL_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE 
src
thirdparty
thirdparty/sm
thirdparty/unirender/include
thirdparty/gtxt
thirdparty/vessel/src
thirdparty/vessel/src/include
thirdparty/vessel/src/optional
thirdparty/spatialdb/include
thirdparty/polymesh3/include
thirdparty/halfedge/include
thirdparty/geoshape/include
thirdparty/tinyobjloader
thirdparty/primitive/include
thirdparty/model/include
thirdparty/guard/
thirdparty/tessellation/include
thirdparty/easygui/include
thirdparty/gimg
thirdparty/shadertrans/include
thirdparty/guard/include
thirdparty/graph/include
thirdparty/objcomp/include
thirdparty/dtex/include
thirdparty/brepom/include
thirdparty/uniphysics/include
thirdparty/easyvm/include
thirdparty/brepvm/include
thirdparty/glfw/include
thirdparty/tinyobjloader
thirdparty/wrapper/include
)

set(EDITOR_FILES
    "src/editor/main.cpp"
)

# ----------------------------------------------------------------------------
# External dependencies
#
# Order matters: externals must be add_subdirectory'd BEFORE their consumers
# so that if(TARGET ...) checks in consumer CMakeLists succeed and their
# generated headers (e.g. assimp/config.h, ogdf/.../config_autogen.h,
# core_tables_header.inc) are exposed via the target's INTERFACE properties.
#
# C libraries (zlib, libpng, libjpeg-turbo, tiff) come from vcpkg — see
# vcpkg.json. Their vendored copies under thirdparty/gimg/external/ are no
# longer built. CMakeLocal.cmake must set CMAKE_TOOLCHAIN_FILE to the vcpkg
# toolchain for find_package to discover them.
# ----------------------------------------------------------------------------

find_package(ZLIB REQUIRED)
find_package(PNG  REQUIRED)
find_package(JPEG REQUIRED)
find_package(TIFF REQUIRED)
# Vulkan loader (vulkan-1.lib) — unirender's Vulkan backend calls these APIs.
find_package(Vulkan REQUIRED)

# DirectXShaderCompiler is too heavy to add_subdirectory (it's project(LLVM)).
# shadertrans only needs dxc/config.h to be present on the include path;
# the editor links against prebuilt DXC libs. The template has two
# #cmakedefine directives, both off — produce a stub config.h in the build dir.
set(_dxc_config_in  "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/shadertrans/external/DirectXShaderCompiler/include/dxc/config.h.cmake")
set(_dxc_config_out "${CMAKE_BINARY_DIR}/generated/dxc/include/dxc/config.h")
configure_file("${_dxc_config_in}" "${_dxc_config_out}" @ONLY)

# Disable SPIRV-Tools tests/executables (we only need the libraries).
# SPIRV_SKIP_EXECUTABLES implies SPIRV_SKIP_TESTS.
set(SPIRV_SKIP_EXECUTABLES ON CACHE BOOL "" FORCE)

# Disable assimp install step. With find_package(ZLIB) already cached above,
# assimp's "INSTALL TARGETS zlib zlibstatic" branch at CMakeLists.txt:559
# would otherwise fire — but those targets don't exist (vcpkg gives us the
# ZLIB::ZLIB imported target, not a plain zlib/zlibstatic target).
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
# Use the system (vcpkg) zlib rather than assimp's bundled copy.
set(ASSIMP_BUILD_ZLIB OFF CACHE BOOL "" FORCE)

# Vendored externals first — they must precede their consumers so the
# consumers' if(TARGET ...) checks pick up the targets and their generated
# header dirs (e.g. assimp/config.h, ogdf/.../config_autogen.h,
# core_tables_header.inc) propagate via INTERFACE properties.
add_subdirectory(thirdparty/shadertrans/external/SPIRV-Headers)
add_subdirectory(thirdparty/shadertrans/external/SPIRV-Tools)
add_subdirectory(thirdparty/shadertrans/external/SPIRV-Cross)
add_subdirectory(thirdparty/shadertrans/external/glslang)
# glslang's own CMakeLists set FOLDER="glslang" on every sub-target, including
# the target literally named "glslang". VS Solution Explorer can't host a
# folder and a project with the same name at the same level -- the .sln fails
# to load with "solution already contains an item named 'glslang'". Rename the
# folder to "glslang_libs" here (post-add_subdirectory) so we don't have to
# patch the submodule. Same pattern can be applied if other vendored trees
# trip the same VS rule.
foreach(_glslang_tgt
        glslang
        GenericCodeGen
        MachineIndependent
        OGLCompiler
        OSDependent
        SPIRV
        SPVRemapper
        glslang-default-resource-limits)
    if(TARGET ${_glslang_tgt})
        set_target_properties(${_glslang_tgt} PROPERTIES FOLDER "glslang_libs")
    endif()
endforeach()

add_subdirectory(thirdparty/shadertrans/external/SpvGenTwo)
add_subdirectory(thirdparty/model/external/tinyobjloader)
add_subdirectory(thirdparty/model/external/assimp)
add_subdirectory(thirdparty/graph/external/ogdf)

# Internal libs — order matches the original (intentionally unchanged)
# so existing inter-lib if(TARGET ...) relationships continue to hold.
add_subdirectory(thirdparty/glfw)
add_subdirectory(thirdparty/gl3w)
add_subdirectory(thirdparty/sm)
add_subdirectory(thirdparty/unirender)
add_subdirectory(thirdparty/vessel)
add_subdirectory(thirdparty/shadertrans)
add_subdirectory(thirdparty/guard)
add_subdirectory(thirdparty/tessellation)
add_subdirectory(thirdparty/polymesh3)
add_subdirectory(thirdparty/geoshape)
add_subdirectory(thirdparty/halfedge)
add_subdirectory(thirdparty/dtex)
add_subdirectory(thirdparty/model)
add_subdirectory(thirdparty/easygui)
add_subdirectory(thirdparty/gtxt)
add_subdirectory(thirdparty/gimg)
add_subdirectory(thirdparty/primitive)
add_subdirectory(thirdparty/texpack)
add_subdirectory(thirdparty/ds)
add_subdirectory(thirdparty/fs)
add_subdirectory(thirdparty/logger)
add_subdirectory(thirdparty/freetype)
#add_subdirectory(thirdparty/uniphysics)
add_subdirectory(thirdparty/easyvm)
add_subdirectory(thirdparty/brepvm)
add_subdirectory(thirdparty/spatialdb)
add_subdirectory(thirdparty/objcomp)
add_subdirectory(thirdparty/brepom)
add_subdirectory(thirdparty/graph)
add_subdirectory(thirdparty/wrapper)
add_subdirectory(thirdparty/cax)

# OGDF (thirdparty/graph/external/ogdf/CMakeLists.txt) force-pins
# CMAKE_CONFIGURATION_TYPES to "Debug;Release" via a cached FORCE set during its
# add_subdirectory, which drops RelWithDebInfo. Re-assert the full set here (after
# all subdirectories) so the optimized + PDB RelWithDebInfo config is available
# again -- used to profile the editor's own first-party code (no PDB in Release).
set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo" CACHE STRING "" FORCE)

add_executable(editor ${EDITOR_FILES})

# Per-machine SDK locations come from CMakeLocal.cmake (gitignored).
# Required variables (see CMakeLocal.cmake.example for the full list):
#   FBX_SDK_DIR, FBX_LIB_DIR, OCCT_DIR, OCCT_LIB_DIR,
#   VULKAN_LIBRARY, DXC_LIBRARY
if(NOT DEFINED FBX_SDK_DIR)
    message(FATAL_ERROR "FBX_SDK_DIR is not set. Define it in CMakeLocal.cmake — see CMakeLocal.cmake.example.")
endif()
if(NOT DEFINED FBX_LIB_DIR)
    set(FBX_LIB_DIR "${FBX_SDK_DIR}/lib/vs2022/x64/debug")
endif()

target_include_directories(editor PRIVATE src include thirdparty thirdparty/sm thirdparty/vessel/src/include thirdparty/gl/include)

list(APPEND CMAKE_PREFIX_PATH "${OCCT_DIR}")
find_package(OpenCASCADE REQUIRED)
if(DEFINED OCCT_LIB_DIR)
    link_directories("${OCCT_LIB_DIR}")
endif()
set(OpenCASCADE_LIBRARIES TKernel TKMath TKG2d TKG3d TKGeomBase TKBRep TKTopAlgo TKMesh TKPrim TKOffset TKDESTEP TKDE TKXSBase TKXCAF TKLCAF TKCDF)

message(STATUS "OCCT Libs to link: ${OpenCASCADE_LIBRARIES}")

target_link_libraries(editor
    # First-party
    tantien cax
    # Foundation libs
    glfw gl3w sm vessel unirender guard tessellation polymesh3 geoshape halfedge
    dtex model easygui gtxt gimg primitive texpack ds fs logger easyvm brepvm
    spatialdb objcomp brepom graph wrapper freetype
    # Shader / SPIRV stack (all in-tree via add_subdirectory)
    shadertrans
    spirv-cross-core spirv-cross-glsl spirv-cross-util
    glslang SPIRV
    SpvGenTwoCommon SpvGenTwoLib
    SPIRV-Tools SPIRV-Tools-link SPIRV-Tools-opt
    # Geometry / model deps (in-tree)
    tinyobjloader assimp OGDF
    # System C libs via vcpkg
    ZLIB::ZLIB PNG::PNG JPEG::JPEG TIFF::TIFF
    # Vulkan loader via vcpkg
    Vulkan::Vulkan
    # OCCT
    ${OpenCASCADE_LIBRARIES}
)

# Optional per-machine SDKs (FBX, DXC) — only linked if the file actually exists.
# /FORCE:MULTIPLE below means missing symbols won't be fatal, but unresolved
# externals still surface at runtime if the corresponding feature is used.
foreach(_opt_lib IN ITEMS
    "${DXC_LIBRARY}"
    "${FBX_LIB_DIR}/libfbxsdk-md.lib"
    "${FBX_LIB_DIR}/libxml2-md.lib"
)
    if(_opt_lib AND EXISTS "${_opt_lib}")
        target_link_libraries(editor "${_opt_lib}")
    elseif(_opt_lib)
        message(WARNING "Optional SDK lib not found, skipping link: ${_opt_lib}")
    endif()
endforeach()

# When real LLVMDxcSupport.lib is linked, it already provides
# Unicode::UTF8ToWideString / WideToUTF8String — suppress our local stubs in
# shadertrans/source/ShaderTrans.cpp to avoid the inline-vs-extern collision.
if(DXC_LIBRARY AND EXISTS "${DXC_LIBRARY}")
    target_compile_definitions(shadertrans PRIVATE HAVE_DXC_UNICODE)
endif()

target_link_options(editor PRIVATE "/FORCE:MULTIPLE")

# main.cpp's read_module() resolves vessel imports like "editor.editor" via
# paths relative to CWD (src/script/, src/modules/, src/). Pin the VS debug
# CWD to the project root so F5 works without manually running from a shell.
set_target_properties(editor PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

# Editor loads resources (fonts, scripts, etc.) via relative paths like
# "assets/fonts/default2.ttf" and "src/script/...". So CWD must be the repo
# root, NOT the build output dir. Set VS debugger to launch with that CWD,
# and require a real argv[1] (editor name like "trackgraph"/"trackeditor").
set_target_properties(editor PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    VS_DEBUGGER_COMMAND_ARGUMENTS "trackeditor")

# Stage a SPIR-V-enabled dxcompiler.dll next to editor.exe so shadertrans's
# CompilerDX (which LoadLibrary's "dxcompiler.dll" at runtime) picks up the
# right one, not Windows' D3D-only copy from System32.
#
# Resolution order:
#   1. DXC_DLL set in CMakeLocal.cmake (explicit override)
#   2. ${VULKAN_SDK}/Bin/dxcompiler.dll  (from the Vulkan SDK install env var)
#   3. CMake-installed Vulkan_INCLUDE_DIR/../Bin/dxcompiler.dll
# If none is found, print a warning — editor builds but HLSL paths will fail
# at runtime with "SPIR-V CodeGen not available".
if(WIN32)
    if(NOT DXC_DLL)
        if(DEFINED ENV{VULKAN_SDK} AND EXISTS "$ENV{VULKAN_SDK}/Bin/dxcompiler.dll")
            set(DXC_DLL "$ENV{VULKAN_SDK}/Bin/dxcompiler.dll")
        elseif(Vulkan_INCLUDE_DIR AND EXISTS "${Vulkan_INCLUDE_DIR}/../Bin/dxcompiler.dll")
            set(DXC_DLL "${Vulkan_INCLUDE_DIR}/../Bin/dxcompiler.dll")
        endif()
    endif()
    if(DXC_DLL AND EXISTS "${DXC_DLL}")
        message(STATUS "Will stage dxcompiler.dll from: ${DXC_DLL}")
        add_custom_command(TARGET editor POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${DXC_DLL}" "$<TARGET_FILE_DIR:editor>/dxcompiler.dll"
            COMMENT "Staging SPIR-V-enabled dxcompiler.dll next to editor.exe")
        # dxil.dll is the signing helper DXC loads alongside dxcompiler.dll.
        get_filename_component(_dxc_dll_dir "${DXC_DLL}" DIRECTORY)
        if(EXISTS "${_dxc_dll_dir}/dxil.dll")
            add_custom_command(TARGET editor POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${_dxc_dll_dir}/dxil.dll" "$<TARGET_FILE_DIR:editor>/dxil.dll")
        endif()
    else()
        message(WARNING
            "Could not locate a SPIR-V-enabled dxcompiler.dll. "
            "Install the LunarG Vulkan SDK (https://vulkan.lunarg.com/sdk/home) "
            "or set DXC_DLL in CMakeLocal.cmake to its full path. "
            "Otherwise the editor will fail at runtime with "
            "\"SPIR-V CodeGen not available\" when HLSL→SPIR-V is exercised.")
    endif()
endif()
