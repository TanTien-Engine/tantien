#define QUOTE(...) #__VA_ARGS__
static const char* physicsModuleSource = QUOTE(

foreign class World
{
	init() {}

	foreign update()
	foreign debug_draw()

	foreign add_body(_)
}

foreign class Body
{
	init() {}

	foreign add_shape(_)

	foreign get_pos()
}

);