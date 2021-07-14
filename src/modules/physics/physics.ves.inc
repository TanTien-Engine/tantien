#define QUOTE(...) #__VA_ARGS__
static const char* physicsModuleSource = QUOTE(

foreign class World
{
	init() {}

	foreign update()
	foreign debug_draw()

	foreign add_body(body)
}

foreign class Body
{
	init(type) {}

	foreign add_shape(shape, filled)

	foreign get_pos()
}

);