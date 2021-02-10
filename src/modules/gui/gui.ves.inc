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

foreign class GUI
{
	foreign static begin()
	foreign static end()
	foreign static update()

	foreign static onSize(w, h)
	foreign static transScrPosToProj(x, y)
	// btn: MOUSE_LEFT, MOUSE_RIGHT
	// action: MOUSE_DOWN, MOUSE_UP, MOUSE_MOVE, MOUSE_DRAG
	foreign static mouseInput(btn, action, x, y)

	foreign static button(label, x, y, width, height)
	foreign static slider(label, val, x, y, height, max, verticle)
	foreign static label(label, x, y)
	foreign static checkbox(label, val, x, y)
	foreign static radio_button(label, val, x, y)
	foreign static arrow_button(dir, x, y, height, repeat)
	foreign static selectable(label, val, x, y, length)
	foreign static combo(label, curr_item, items, x, y, length)
}

);