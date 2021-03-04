#define QUOTE(...) #__VA_ARGS__
static const char* renderModuleSource = QUOTE(

var SAMPLER_NEAREST_CLAMP  = 0
var SAMPLER_LINEAR_CLAMP   = 1
var SAMPLER_NEAREST_REPEAT = 2
var SAMPLER_LINEAR_REPEAT  = 3

foreign class Shader
{
	// vs: string
	// fs: string
	init(vs, fs) {}

	// name(string) + type(string) + value(list)
	foreign setUniformValue(uniform)
}

foreign class VertexArray
{
	// data: float list
	// attrs: number of components
	// ibuf: index buffer
	init(data, attrs, ibuf) {}

	// type: "quad", "cube", "sphere", "grids"
	// layout: [ "position", "texture", "normal", "tangent_bitangent" ]
	init(type, layout) {}
}

foreign class Texture
{
	init(img_data, gamma_correction) {}
	// format: "rgb", "rgba8", "rgba16f", "r16"
	init(width, height, format) {}
	// cubemap, textures: list [ right, left, top, bottom, front, back ]
	init(textures) {}

	foreign getWidth()
	foreign getHeight()
}

foreign class Framebuffer
{
	init() {}

	// type: "col0", "col1", ... , "col16", "depth", "stencil"
	foreign attachment(tex, type)
}

foreign class Render
{
	// prim_type: "triangles"
	// render_state: map { 
	//   "depth_test" : bool,
	//   "depth_func" : <"never", "less", "equal", "lequal", "greater", "notequal", "gequal", "always">
	//   "cull" : <"disable", "front", "back", "front_and_back">
	// }
	foreign static draw(prim_type, prog, va, render_state)

	foreign static compute(prog, num_groups_x, num_groups_y, num_groups_z)

	// masks: list ["color", "depth", "stencil"]
	// values: map { "color" : [r,g,b,a] }
	foreign static clear(masks, values)

	// stage: <"vertex", "tess_ctrl", "tess_eval", "pixel", "compute">
	// lang: <"glsl", "hlsl">
	// return: list, name + type + values
	// uniform types: unknown, bool, int, int2, int3, int4, float, float2, float3, float4, mat2, mat3, mat4, sampler
	foreign static getShaderUniforms(stage, code, lang)

	foreign static get_fbo()
	foreign static set_fbo(fbo)	

	// [x, y, w, h]
	foreign static get_viewport()
	foreign static set_viewport(vp)
}

);