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

	foreign add_joint(joint)
	foreign remove_joint(joint)

	foreign query_by_pos(pos)	

	foreign static get_scale_factor()
}

foreign class Body
{
	init(type, flag) {}

	foreign add_shape(shape, filled)

	foreign set_gravity_scale(density)

	foreign set_density(density)
	foreign set_restitution(restitution)
	foreign set_friction(friction)

	foreign apply_force(force)
	foreign apply_torque(torque)
	foreign apply_linear_impulse(impulse)
	foreign apply_angular_impulse(impulse)

	foreign set_transform(pos, angle)

	foreign get_pos()
	foreign get_type()
	foreign get_flag()

	foreign set_linear_velocity(x, y)
	foreign get_linear_velocity()

	foreign is_valid()

	foreign get_mass()
}

foreign class PrismaticJoint
{
	init(body_a, body_b, anchor, axis) {}
}

foreign class MouseJoint
{
	init(body_a, body_b, target, max_force) {}

	foreign set_target(target)
}

);