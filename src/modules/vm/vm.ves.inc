#define QUOTE(...) #__VA_ARGS__
static const char* vmModuleSource = QUOTE(

foreign class Bytecodes
{
	init() {}

	foreign size()

	foreign set_pos(pos)
	foreign write_num(num)	

	foreign set_ret_reg(reg)
	foreign get_ret_reg()

	foreign store_bool(reg, bool)

	foreign store_num(reg, num)
	foreign print_num(reg)
	foreign negate_num(dst, src)

	foreign add_num(r_dst, r_src1, r_src2)
	foreign sub_num(r_dst, r_src1, r_src2)
	foreign mul_num(r_dst, r_src1, r_src2)
	foreign div_num(r_dst, r_src1, r_src2)

	foreign inc_num(reg)
	foreign dec_num(reg)

	foreign cmp_num(r_dst, r_src1, r_src2)

	foreign jump_if_not(offset, r_bool)

	// math
	foreign vec2_create_i(r_vec, x, y)
	foreign vec3_create_r(r_dst, r_x, r_y, r_z)
	foreign vec3_create_i(r_vec, x, y, z)
	foreign vec3_print(r_vec)
	foreign vec3_add(r_dst, r_src1, r_src2)
	foreign vec3_sub(r_dst, r_src1, r_src2)
	foreign vec3_transform(r_dst, r_mat)
	foreign vec3_get_x(r_dst, r_src)
	foreign vec3_get_y(r_dst, r_src)
	foreign vec3_get_z(r_dst, r_src)
	foreign vec4_create_i(r_vec, x, y, z, w)
	foreign create_mat4(r_mat)
	foreign mat4_rotate(r_mat, r_xyz)
	foreign mat4_translate(r_mat, r_xyz)
	foreign create_cube(r_dst, r_min, r_max)

	// stl
	foreign create_vector(r_list)
	foreign vector_add(r_list, r_item)
	foreign vector_concat(r_dst, r_src)
	foreign vector_get(r_dst, r_vec, idx)

	// geo
	foreign create_plane(r_dst, r_p0, r_p1, r_p2)
	foreign create_plane_2(r_dst, r_ori, r_dir)
	foreign create_polyface(r_dst, r_plane)
	foreign create_polytope(r_dst, r_faces)
	foreign create_polyface_2(r_dst, r_border, r_holes)
	foreign create_polytope_2(r_dst, r_points, r_faces)
	foreign polytope_transform(r_poly, r_mat)
	foreign polytope_subtract(r_dst, r_base, r_tool)
	foreign polytope_extrude(r_poly, r_dist)
	foreign polytope_clip(r_poly, r_plane, keep, seam)
	foreign polytope_set_dirty(r_poly)
	foreign polypoint_select(r_dst, r_poly, r_cube)
	foreign polyface_select(r_dst, r_poly, r_normal, r_region)	

	// multi
	foreign add(r_dst, r_src1, r_src2)
	foreign sub(r_dst, r_src1, r_src2)
	foreign mul(r_dst, r_src1, r_src2)
	foreign div(r_dst, r_src1, r_src2)
	foreign transform_unknown(r_obj, r_mat)
}

foreign class Compiler
{
	init() {}

	foreign new_reg()
	foreign free_reg(reg)
	foreign keep_reg(reg, keep)
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