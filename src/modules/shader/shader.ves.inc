#define QUOTE(...) #__VA_ARGS__
static const char* shaderModuleSource = QUOTE(

foreign class ShaderTools
{
	foreign static code2spirv(stage, code, lang)
	foreign static disassemble(spirv)

	foreign static hlsl2glsl(stage, hlsl, entry_point)
}

foreign class ShaderModule
{
	init() {}

	foreign const_bool(b)
	foreign const_int(x)

	foreign const_float(x)
	foreign const_float2(x, y)
	foreign const_float3(x, y, z)
	foreign const_float4(x, y, z, w)

	foreign const_mat2(m)
	foreign const_mat3(m)
	foreign const_mat4(m)

	foreign query_func(name)

	foreign create_decl_func(func)
	foreign create_func(name, ret, args)
}

foreign class ShaderFunc
{
	init() {}

	foreign add_variable(name, value)

	foreign variable_float(name)
	foreign variable_float2(name)
	foreign variable_float3(name)
	foreign variable_float4(name)

	foreign get_block()
	foreign add_block(name)

	foreign get_param(index)
	foreign get_args()
	foreign call(callee, args)
}

foreign class ShaderBlock
{
	init() {}

	type_cast(from, to)
	{
		if (from.is_vector() or !to.is_vector()) {
		    return from
		}

		var num = to.get_vector_num()
		if (num == 2) {
			return this.compose_float2(from, from)
		} else if (num == 3) {
			return this.compose_float3(from, from, from)
		} else if (num == 4) {
			return this.compose_float4(from, from, from, from)
		}

		return from
	}

	add(a, b)
	{
		if (!a and !b) {
			return nil
		} else if (!a) {
			return b
		} else if (!b) {
			return a
		}

		if (a.is_vector() and !b.is_vector()) {
		    b = this.type_cast(b, a)
		} else if (!a.is_vector() and b.is_vector()) {
		    a = this.type_cast(a, b)
		}

		return this.add_(a, b)
	}

	sub(a, b)
	{
		if (!a and !b) {
			return nil
		} else if (!a) {
			return b
		} else if (!b) {
			return a
		}

		if (a.is_vector() and !b.is_vector()) {
		    b = this.type_cast(b, a)
		} else if (!a.is_vector() and b.is_vector()) {
		    a = this.type_cast(a, b)
		}

		return this.sub_(a, b)
	}

	mix(x, y, a)
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

		if (!x.is_vector() and !y.is_vector()) {
		    return this.mix_(x, y, a)
		}

		if (!x.is_vector() or !y.is_vector() or 
		    x.get_vector_num() != y.get_vector_num()) {
		    return nil
		}

		var num = x.get_vector_num()
		var comp = []
		for (var i in 0..num) {
			var mix = this.mix_(this.compose_extract(x, i), this.compose_extract(y, i), a)
			comp.add(mix)
		}
		if (num == 2) {
			return this.compose_float2(comp[0], comp[1])
		} else if (num == 3) {
			return this.compose_float3(comp[0], comp[1], comp[2])
		} else if (num == 4) {
			return this.compose_float4(comp[0], comp[1], comp[2], comp[3])
		}

		return nil
	}

	pow(x, y)
	{
		if (!x or !y) {
			return nil
		}

		return this.pow_(x, y)
	}

	sqrt(v)
	{
		if (!v.is_vector()) {
			return this.sqrt_(v)
		}

		var num = v.get_vector_num()
		var comp = []
		for (var i in 0..num) {
			var sqrt = this.sqrt_(this.compose_extract(v, i))
			comp.add(sqrt)
		}
		if (num == 2) {
			return this.compose_float2(comp[0], comp[1])
		} else if (num == 3) {
			return this.compose_float3(comp[0], comp[1], comp[2])
		} else if (num == 4) {
			return this.compose_float4(comp[0], comp[1], comp[2], comp[3])
		}

		return from		
	}

	dot(a, b)
	{
		if (!a or !b) {
			return nil
		}

		return this.dot_(a, b)
	}

	cross(a, b)
	{
		if (!a or !b) {
			return nil
		}

		return this.cross_(a, b)
	}

	negate(v)
	{
		if (!v.is_vector()) {
			return this.negate_(v)
		}

		var num = v.get_vector_num()
		var comp = []
		for (var i in 0..num) {
			var neg = this.negate_(this.compose_extract(v, i))
			comp.add(neg)
		}
		if (num == 2) {
			return this.compose_float2(comp[0], comp[1])
		} else if (num == 3) {
			return this.compose_float3(comp[0], comp[1], comp[2])
		} else if (num == 4) {
			return this.compose_float4(comp[0], comp[1], comp[2], comp[3])
		}

		return from		
	}

	foreign access_chain(base, index)
	foreign compose_float2(x, y)
	foreign compose_float3(x, y, z)
	foreign compose_float4(x, y, z, w)
	foreign compose_extract(comp, idx)
	foreign dot_(a, b)
	foreign cross_(a, b)
	foreign add_(a, b)
	foreign sub_(a, b)
	foreign mul(a, b)
	foreign div(a, b)
	foreign negate_(v)
	foreign reflect(I, N)
	foreign sqrt_(v)
	foreign pow_(x, y)
	foreign normalize(v)
	foreign length(v)
	foreign max(a, b)
	foreign min(a, b)
	foreign clamp(x, min, max)
	foreign mix_(x, y, a) 

	foreign is_equal(a, b)
	foreign is_not_equal(a, b)
	foreign is_greater(a, b)
	foreign is_greater_equal(a, b)
	foreign is_less(a, b)
	foreign is_less_equal(a, b)	

	foreign kill()

	foreign return_null()
	foreign return_value(inst)

	foreign load(val)
	foreign store(dst, src)

	foreign image_sample(img, uv, lod)

	foreign branch_if(cond, true_bb, false_bb, merge_bb)
}

foreign class ShaderInst
{
	init() {}

	foreign get_type()

	foreign is_vector()
	foreign get_vector_num()	
}

foreign class ShaderGen
{
	init() {}
	
	foreign add_input(name, type)
	foreign add_output(name, type)
	foreign add_uniform(module, name, type)
	foreign query_unif_name(unif)

	foreign add_module(stage, code, lang, name, entry_point)

	foreign func_replace(from, to)

	foreign get_main_module()
	foreign get_main_func()
	foreign add_link_decl(func, name, is_export)

	foreign connect_cs_main(glsl)
}

);