cmake_minimum_required(VERSION 3.16)

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

add_subdirectory(thirdparty/shadertrans/external/SPIRV-Cross)
add_subdirectory(thirdparty/shadertrans/external/glslang)
add_subdirectory(thirdparty/shadertrans/external/SpvGenTwo)
add_subdirectory(thirdparty/model/external/tinyobjloader)
add_subdirectory(thirdparty/model/external/assimp)
add_subdirectory(thirdparty/graph/external/ogdf)

add_executable(editor ${EDITOR_FILES})

set(FBX_SDK_DIR "C:/Program Files/Autodesk/FBX/FBX SDK/2020.3.7")
set(FBX_LIB_DIR "${FBX_SDK_DIR}/lib/arm64/debug")

target_include_directories(editor PRIVATE src include thirdparty thirdparty/sm thirdparty/vessel/src/include thirdparty/gl/include)

set(OCCT_DIR "Y:/OCCT/install") 
list(APPEND CMAKE_PREFIX_PATH "${OCCT_DIR}")
find_package(OpenCASCADE REQUIRED)
link_directories("Y:/OCCT/install/win64/vc14/libd")
set(OpenCASCADE_LIBRARIES TKernel TKMath TKG2d TKG3d TKGeomBase TKBRep TKTopAlgo TKMesh TKPrim TKOffset TKDESTEP TKDE TKXSBase TKXCAF TKLCAF TKCDF)

message(STATUS "OCCT Libs to link: ${OpenCASCADE_LIBRARIES}")

target_link_libraries(editor glfw gl3w tantien sm vessel unirender shadertrans guard tessellation polymesh3 geoshape halfedge dtex model easygui gtxt gimg primitive texpack ds fs logger easyvm brepvm spatialdb objcomp brepom graph wrapper cax freetype spirv-cross-core spirv-cross-glsl spirv-cross-util glslang SPIRV SpvGenTwoCommon SpvGenTwoLib tinyobjloader assimp OGDF C:/VulkanSDK/1.4.335.0/Lib/vulkan-1.lib ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/shadertrans/external/SPIRV-Tools/out_arm/source/Debug/SPIRV-Tools.lib ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/shadertrans/external/SPIRV-Tools/out_arm/source/link/Debug/SPIRV-Tools-link.lib ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/shadertrans/external/SPIRV-Tools/out_arm/source/opt/Debug/SPIRV-Tools-opt.lib ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/shadertrans/external/DirectXShaderCompiler/out_arm/Debug/lib/LLVMDxcSupport.lib ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/gimg/external/zlib/out_arm/Debug/zsd.lib ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/gimg/external/libpng/out_arm/Debug/libpng18_staticd.lib ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/gimg/external/libjpeg-turbo/out_arm/Debug/turbojpeg-static.lib ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/gimg/external/libtiff/out_arm/libtiff/Debug/tiffd.lib "${FBX_LIB_DIR}/libfbxsdk-md.lib" "${FBX_LIB_DIR}/libxml2-md.lib" ${OpenCASCADE_LIBRARIES})

target_link_options(editor PRIVATE "/FORCE:MULTIPLE")
