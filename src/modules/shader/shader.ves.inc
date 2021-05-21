#define QUOTE(...) #__VA_ARGS__
static const char* shaderModuleSource = QUOTE(

foreign class ShaderTools
{
	foreign static code2spirv(stage, code, lang)
	foreign static disassemble(spirv)
}

foreign class SpirvGenTwo
{
	foreign static get_type(inst)
	foreign static is_vector(inst)
	foreign static get_vector_num(inst)

	foreign static add(func, a, b)
	foreign static sub(func, a, b)
	foreign static mix(func, x, y, a)
	foreign static pow(func, x, y)
	foreign static sqrt(func, v)
	foreign static dot(func, a, b)
}

foreign class ShaderGen
{
	init() {}

	type_cast(func, from, to)
	{
		if (SpirvGenTwo.is_vector(from) or 
		    !SpirvGenTwo.is_vector(to)) {
		    return from
		}

		var num = SpirvGenTwo.get_vector_num(to)
		if (num == 2) {
			return this.compose_float2(func, from, from)
		} else if (num == 3) {
			return this.compose_float3(func, from, from, from)
		} else if (num == 4) {
			return this.compose_float4(func, from, from, from, from)
		}

		return from
	}

	add(func, a, b)
	{
		if (!a and !b) {
			return nil
		} else if (!a) {
			return b
		} else if (!b) {
			return a
		}

		if (SpirvGenTwo.is_vector(a) and
		    !SpirvGenTwo.is_vector(b)) {
		    b = this.type_cast(func, b, a)
		} else if (!SpirvGenTwo.is_vector(a) and
		    SpirvGenTwo.is_vector(b)) {
		    a = this.type_cast(func, a, b)
		}

		return SpirvGenTwo.add(func, a, b)
	}

	sub(func, a, b)
	{
		if (!a and !b) {
			return nil
		} else if (!a) {
			return b
		} else if (!b) {
			return a
		}

		if (SpirvGenTwo.is_vector(a) and
		    !SpirvGenTwo.is_vector(b)) {
		    b = this.type_cast(func, b, a)
		} else if (!SpirvGenTwo.is_vector(a) and
		    SpirvGenTwo.is_vector(b)) {
		    a = this.type_cast(func, a, b)
		}

		return SpirvGenTwo.sub(func, a, b)
	}

	mix(func, x, y, a)
	{
		if (!x and !y) {
			return nil
		} else if (!x) {
			return y
		} else if (!y) {
			return x
		} else if (!a) {
			return x
		}

		if (!SpirvGenTwo.is_vector(x) and 
		    !SpirvGenTwo.is_vector(y)) {
		    return SpirvGenTwo.mix(func, x, y, a)
		}

		if (!SpirvGenTwo.is_vector(x) or 
		    !SpirvGenTwo.is_vector(y) or 
		    SpirvGenTwo.get_vector_num(x) != SpirvGenTwo.get_vector_num(y)) {
		    return nil
		}

		var num = SpirvGenTwo.get_vector_num(x)
		var comp = []
		for (var i in 0..num) {
			var mix = SpirvGenTwo.mix(func, this.compose_extract(func, x, i), this.compose_extract(func, y, i), a)
			comp.add(mix)
		}
		if (num == 2) {
			return this.compose_float2(func, comp[0], comp[1])
		} else if (num == 3) {
			return this.compose_float3(func, comp[0], comp[1], comp[2])
		} else if (num == 4) {
			return this.compose_float4(func, comp[0], comp[1], comp[2], comp[3])
		}

		return nil
	}

	pow(func, x, y)
	{
		if (!x or !y) {
			return nil
		}

		return SpirvGenTwo.pow(func, x, y)
	}

	sqrt(func, v)
	{
		if (!SpirvGenTwo.is_vector(v)) {
			return SpirvGenTwo.sqrt(func, v)
		}

		var num = SpirvGenTwo.get_vector_num(v)
		var comp = []
		for (var i in 0..num) {
			var sqrt = SpirvGenTwo.sqrt(func, this.compose_extract(func, v, i))
			comp.add(sqrt)
		}
		if (num == 2) {
			return this.compose_float2(func,  comp[0], comp[1])
		} else if (num == 3) {
			return this.compose_float3(func, comp[0], comp[1], comp[2])
		} else if (num == 4) {
			return this.compose_float4(func, comp[0], comp[1], comp[2], comp[3])
		}

		return from		
	}

	dot(func, a, b)
	{
		if (!a or !b) {
			return nil
		}

		return SpirvGenTwo.dot(func, a, b)
	}

	foreign add_input(name, type)
	foreign add_output(name, type)
	foreign add_uniform(module, name, type)

	foreign access_chain(func, base, index)
	foreign compose_float2(func, x, y)
	foreign compose_float3(func, x, y, z)
	foreign compose_float4(func, x, y, z, w)
	foreign compose_extract(func, comp, idx)
	foreign mul(func, a, b)
	foreign div(func, a, b)
	foreign negate(func, v)
	foreign normalize(func, v)
	foreign max(func, a, b)
	foreign min(func, a, b)
	foreign clamp(func, x, min, max)
	foreign store(func, dst, src)
	foreign load(func, val)
	foreign image_sample(func, img, uv)

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

	foreign variable_float(func)

	foreign const_float(module, x)
	foreign const_float2(module, x, y)
	foreign const_float3(module, x, y, z)
	foreign const_float4(module, x, y, z, w)

	foreign const_mat2(module, m)
	foreign const_mat3(module, m)
	foreign const_mat4(module, m)

	foreign import_all()
	foreign finish_main()

	foreign connect_cs_main(glsl)

	foreign print(lib)
}

);