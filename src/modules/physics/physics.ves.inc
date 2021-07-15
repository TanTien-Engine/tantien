#define QUOTE(...) #__VA_ARGS__
static const char* physicsModuleSource = QUOTE(

foreign class World
{
	init() {}

	physics_begin_contact(a, b)
	{
		if (a.get_type() == "kinematic") {
			this.remove_body(a)
		}
		if (b.get_type() == "kinematic") {
			this.remove_body(b)
		}
	}

	foreign update()
	foreign debug_draw()

	foreign add_body(body)
	foreign remove_body(body)
}

foreign class Body
{
	init(type) {}

	foreign add_shape(shape, filled)

	foreign get_pos()
	foreign get_type()
}

);