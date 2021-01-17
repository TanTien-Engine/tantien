#define QUOTE(...) #__VA_ARGS__
static const char* renderModuleSource = QUOTE(

foreign class Render
{
	foreign static newShader(vs, fs)
	foreign static newVertexArray(data, attrs)

	foreign static draw(prim_type, prog, va, render_state)
}

);