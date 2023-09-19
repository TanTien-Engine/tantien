#define QUOTE(...) #__VA_ARGS__
static const char* vmModuleSource = QUOTE(

foreign class Bytecodes
{
	init() {}

	foreign store_num(reg, num)
	foreign print_num(reg)

	foreign add(reg_dst, reg_src1, reg_src2)

	foreign set_ret_reg(reg)
	foreign get_ret_reg()
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
}

);