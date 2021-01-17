#define QUOTE(...) #__VA_ARGS__
static const char* renderModuleSource = QUOTE(

foreign class Render
{
	// vs: string
	// fs: string
	foreign static newShader(vs, fs)
	// data: float list
	// attrs: number of components
	foreign static newVertexArray(data, attrs)

	// prim_type: "triangles"
	// prog: program index
	// va: vertex array index
	// render_state: map { "depth_test" : bool }
	foreign static draw(prim_type, prog, va, render_state)

	// masks: list ["color", "depth", "stencil"]
	// values: map { "color" : [r,g,b,a] }
	foreign static clear(masks, values)
}

);