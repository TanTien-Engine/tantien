#define QUOTE(...) #__VA_ARGS__
static const char* shaderModuleSource = QUOTE(

foreign class ShaderTools
{
	foreign static code2spirv(stage, code, lang)
	foreign static disassemble(spirv)
}

foreign class ShaderGen
{
	init() {}

	foreign add_uniform(name, type)
	foreign add_input(name, type)
	foreign add_output(name, type)

	foreign op_const_float(x)
	foreign op_const_float2(x, y)	
	foreign op_call(func, args)
	foreign op_access_chain(base, index)
	foreign op_store(dst, src)
	foreign op_return()

	// return entry func
	foreign add_lib(stage, glsl)

	foreign create_decl_func(func)
	foreign add_link_decl(func, name, is_export)

	foreign import_all()
	foreign finish_main()
	foreign link()
}

);