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
}

foreign class Render
{
	// prim_type: "triangles"
	// prog: program index
	// va: vertex array index
	// render_state: map { 
	//   "depth_test" : bool,
	//   "cull" : <"disable", "front", "back", "front_and_back">
	// }
	foreign static draw(prim_type, prog, va, render_state)

	// masks: list ["color", "depth", "stencil"]
	// values: map { "color" : [r,g,b,a] }
	foreign static clear(masks, values)

	// stage: <"vertex", "tess_ctrl", "tess_eval", "pixel", "compute">
	// lang: <"glsl", "hlsl">
	// return: list, name + type + values
	// uniform types: unknown, bool, int, int2, int3, int4, float, float2, float3, float4, mat2, mat3, mat4, sampler
	foreign static getShaderUniforms(stage, code, lang)
}

);