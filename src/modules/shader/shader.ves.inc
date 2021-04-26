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
	foreign op_const_float3(x, y, z)
	foreign op_const_float4(x, y, z, w)
	foreign op_call(func, args)
	foreign op_access_chain(base, index)
	foreign op_compose_float2(x, y)
	foreign op_compose_float3(x, y, z)
	foreign op_compose_float4(x, y, z, w)
	foreign op_compose_extract(comp, idx)
	foreign op_dot(a, b)
	foreign op_add(a, b)
	foreign op_sub(a, b)
	foreign op_mul(a, b)
	foreign op_div(a, b)
	foreign op_store(dst, src)
	foreign op_return()

	foreign add_lib(stage, glsl)
	foreign get_func(lib, index)

	foreign func_replace(from, to)

	foreign get_main_module()
	foreign create_decl_func(module, func)
	foreign add_link_decl(func, name, is_export)

	foreign import_all()
	foreign finish_main()
	foreign link()

	foreign print(lib)
}

);