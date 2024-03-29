#define QUOTE(...) #__VA_ARGS__
static const char* renderModuleSource = QUOTE(

foreign class Shader
{
	init(vs, tcs, tes, gs, fs, inc_dir) {}
	init(cs, inc_dir) {}

	// name(string) + type(string) + value(list)
	foreign set_uniform_value(uniform)
}

foreign class VertexArray
{
	// data: float list
	// attrs: number of components
	// ibuf: index buffer
	init(data, attrs, ibuf) {}

	// type: "quad", "cube", "sphere", "grids"
	// layout: [ "position", "normal", "texture", "tangent_bitangent" ]
	// params: map { 
	//   "grids_size" : int
	// }
	init(type, layout, params) {}
}

foreign class Texture2D
{
	init(img_data, gamma_correction) {}
	// format: "rgb", "rgba8", "rgba16f", "rg16f", "rg32f", "r16", "depth"
	init(width, height, format, mipmap) {}

	foreign get_width()
	foreign get_height()

	foreign upload(pixels, x, y, w, h)
	foreign download(image)

	// fixme: move to image
	foreign max_pixel_val()
}

foreign class Texture3D
{
	// format: "rgb", "rgba8", "rgba16f", "rg16f", "rg32f", "r16", "depth"
	init(width, height, depth, format) {}

	foreign get_width()
	foreign get_height()
	foreign get_depth()
}

foreign class Cubemap
{
	// textures [ right, left, top, bottom, front, back ]
	init(textures) {}
	// format: "rgb", "rgb16f", "rgba8", "rgba16f", "r16"
	init(width, height, format) {}

	foreign get_width()
	foreign get_height()	
}

foreign class TextureSampler
{
	// "nearest_clamp", "linear_clamp", "linear_clamp_mipmap", "nearest_repeat", "linear_repeat", "linear_repeat_mipmap"
	init(type) {}
}

foreign class Framebuffer
{
	init() {}

	// attachment: "col0", "col1", ... , "col16", "depth", "stencil"
	// target: "tex2d", "tex3d", "cubemap0", ... , "cubemap6"
	foreign attach_tex(tex, attachment, target, mipmap_level)
	foreign attach_rbo(rbo, attachment)
}

foreign class ComputeBuffer
{
	// type: <"int", "float">
	// data: list
	init(type, data, id) {}

	foreign download(type, size)
}

foreign class StorageBuffer
{
	init(data) {}
}

foreign class TextureBuffer
{
	init() {}

	foreign get_texture()
}

foreign class RenderBuffer
{
	// format: "depth_component", "depth_stencil", "red", "rg", "rgb", "rgba"
	init(width, height, format) {}
}

foreign class RenderState
{
	init() {}

	// "face" : <"front", "back", "front_and_back">
	// "func" : <"never", "less", "equal", "lequal", "greater", "notequal", "gequal", "always">
	// "op" : <"zero", "invert", "keep", "replace", "increment", "decrement", "increment_wrap", "decrement_wrap">
	foreign stencil_test(face, func, ref, fail_op, pass_op)

	foreign z_write(enable)
	// "func" : <"never", "less", "equal", "lequal", "greater", "notequal", "gequal", "always">
	foreign z_test(enable, func)

	// "type" : <"disable", "front", "back", "front_and_back">
	foreign face_culling(type)

	// "mode" : <"point", "line", "fill">
	foreign rasterization_mode(mode)

	foreign clip_plane(enable)

	foreign blending(enable, src, dst)

	foreign prim_restart(enable)

	foreign depth_clamp(enable)
}

foreign class Render
{
	// prim_type: "points", "lines", "line_strip", "triangles", "tri_strip", "tri_adjacency", "patches"
	// render_state: map { 
	//   "depth_test" : bool,
	//   "depth_func" : <"never", "less", "equal", "lequal", "greater", "notequal", "gequal", "always">
	//   "stencil_test" : bool,
	//   "stencil_mask" : int
	//   "stencil_func" : <"never", "less", "equal", "lequal", "greater", "notequal", "gequal", "always">
	//   "stencil_ref" : int
	//   "cull" : <"disable", "front", "back", "front_and_back">
	//   "rasterization" : <"point", "line", "fill">
	//   "clip_plane" : bool
	//   "blend" : bool
	// }
	foreign static draw(prim_type, prog, va, render_state)
	foreign static draw_instanced(prim_type, prog, va, render_state, instanced_mats)
	foreign static draw_model(prog, model, render_state)
	
	foreign static compute(prog, num_groups_x, num_groups_y, num_groups_z)

	// masks: list ["color", "depth", "stencil"]
	// values: map { "color" : [r,g,b,a] }
	foreign static clear(masks, values)

	// stage: <"vertex", "tess_ctrl", "tess_eval",  "geometry", "pixel", "compute">
	// lang: <"glsl", "hlsl">
	// return: list, name + type + values
	// uniform types: unknown, bool, int, int2, int3, int4, float, float2, float3, float4, mat2, mat3, mat4, sampler
	foreign static get_shader_uniforms(stage, code, inc_dir, lang, entry_point)
	foreign static get_shader_func_argus(stage, code, inc_dir, lang, name)

	foreign static get_fbo()
	foreign static set_fbo(fbo)	

	// [x, y, w, h]
	foreign static get_viewport()
	foreign static set_viewport(vp)

	foreign static push_debug_group(msg)
	foreign static pop_debug_group()
}

);