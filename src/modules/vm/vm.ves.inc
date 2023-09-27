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
	foreign store_vec3(r_vec, r_xyz)
	foreign vec3_print(r_vec)
	foreign create_mat4(r_mat)
	foreign mat4_rotate(r_mat, r_xyz)
	foreign mat4_translate(r_mat, r_xyz)

	// stl
	foreign create_vector(r_list)
	foreign vector_add(r_list, r_item)

	// geo
	foreign create_plane(r_dst, r_p0, r_p1, r_p2)
	foreign create_polyface(r_dst, r_plane)
	foreign create_polytope(r_dst, r_faces)
	foreign polytope_transform(r_poly, r_mat)
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