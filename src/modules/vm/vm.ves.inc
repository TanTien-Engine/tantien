#define QUOTE(...) #__VA_ARGS__
static const char* vmModuleSource = QUOTE(

foreign class Bytecodes
{
	init() {}

	foreign set_ret_reg(reg)
	foreign get_ret_reg()

	foreign store_num(reg, num)
	foreign print_num(reg)

	foreign add(reg_dst, reg_src1, reg_src2)

	// math
	foreign store_vec3(reg, xyz)
	foreign print_vec3(reg)
	// stl
	foreign create_vector(reg)
	foreign add_vector(reg_dst, reg_src)
	// geo
	foreign create_plane(reg, p0, p1, p2)
	foreign create_polyface(reg, plane)
	foreign create_polytope(reg, faces)
}

foreign class Compiler
{
	init() {}

	foreign new_reg()
	foreign free_reg(reg)
}

foreign class VM
{
	init(code) {}

	foreign run()

	foreign load_boolean(reg)
	foreign load_number(reg)
	foreign load_string(reg)

	// geo
	foreign load_plane(reg)
	foreign load_polyface(reg)
	foreign load_polytope(reg)
}

);