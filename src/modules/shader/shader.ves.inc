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

	foreign add_uniform(module, name, type)
	foreign add_input(name, type)
	foreign add_output(name, type)

	foreign access_chain(func, base, index)
	foreign compose_float2(func, x, y)
	foreign compose_float3(func, x, y, z)
	foreign compose_float4(func, x, y, z, w)
	foreign compose_extract(func, comp, idx)
	foreign dot(func, a, b)
	foreign add(func, a, b)
	foreign sub(func, a, b)
	foreign mul(func, a, b)
	foreign div(func, a, b)
	foreign negate(func, v)
	foreign store(func, dst, src)
	foreign load(func, val)

	foreign add_module(stage, glsl, name)
	foreign query_func(lib, name)

	foreign func_replace(from, to)

	foreign get_main_module()
	foreign get_main_func()
	foreign create_decl_func(module, func)
	foreign add_link_decl(func, name, is_export)

	foreign create_func(module, name, ret, args)
	foreign get_func_param(func, index)
	foreign get_func_args(func)
	foreign func_call(caller, callee, args)
	foreign func_return(func)
	foreign func_return_value(func, inst)

	foreign const_float(module, x)
	foreign const_float2(module, x, y)
	foreign const_float3(module, x, y, z)
	foreign const_float4(module, x, y, z, w)

	foreign const_mat3(module, m)

	foreign import_all()
	foreign finish_main()

	foreign print(lib)
}

);