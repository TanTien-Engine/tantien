#define QUOTE(...) #__VA_ARGS__
static const char* renderModuleSource = QUOTE(

foreign class Shader
{
	// vs: string
	// fs: string
	init(vs, fs) {}
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
}

);