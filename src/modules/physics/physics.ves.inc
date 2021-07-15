#define QUOTE(...) #__VA_ARGS__
static const char* physicsModuleSource = QUOTE(

foreign class World
{
	init() {}

	begin_contact(a, b)
	{
		import "editor" for _editor
		_editor.trigger_list.run("evt_begin_contact", [a, b])
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