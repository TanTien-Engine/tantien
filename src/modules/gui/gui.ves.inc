#define QUOTE(...) #__VA_ARGS__
static const char* guiModuleSource = QUOTE(

// button
var MOUSE_LEFT  = 1
var MOUSE_RIGHT = 2

// action
var MOUSE_DOWN = 0
var MOUSE_UP   = 1
var MOUSE_MOVE = 2
var MOUSE_DRAG = 3

// arrow dir
var ARROW_NONE  = 0
var ARROW_LEFT  = 1
var ARROW_RIGHT = 2
var ARROW_UP    = 3
var ARROW_DOWN  = 4

foreign class Context
{
	init() {}
}

foreign class GUI
{
	foreign static begin(ctx)
	foreign static end(ctx)
	foreign static update(ctx)

	foreign static onSize(w, h)
	foreign static transScrPosToProj(x, y, cam_x, cam_y, cam_scale)
	// btn: MOUSE_LEFT, MOUSE_RIGHT
	// action: MOUSE_DOWN, MOUSE_UP, MOUSE_MOVE, MOUSE_DRAG
	foreign static mouseInput(ctx, btn, action, x, y, cam_x, cam_y, cam_scale)

	foreign static frame(ctx, x, y, width, height)
	foreign static button(ctx, label, x, y, width, height)
	foreign static slider(ctx, label, val, x, y, height, max, verticle)
	foreign static label(ctx, label, x, y)
	foreign static checkbox(ctx, label, val, x, y)
	foreign static radio_button(ctx, label, val, x, y)
	foreign static arrow_button(ctx, dir, x, y, height, repeat)
	foreign static selectable(ctx, label, val, x, y, length)
	foreign static combo(ctx, label, curr_item, items, x, y, length)
}

);