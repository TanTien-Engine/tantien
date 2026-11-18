#define QUOTE(...) #__VA_ARGS__
static const char* guiModuleSource = QUOTE(

// button
var MOUSE_LEFT   = 1
var MOUSE_RIGHT  = 2
var MOUSE_MIDDLE = 3

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
	foreign static rebuild(ctx)

	foreign static on_size(w, h)
	foreign static trans_scr_pos_to_proj(x, y, cam_x, cam_y, cam_scale)
	// btn: MOUSE_LEFT, MOUSE_RIGHT, MOUSE_MIDDLE
	// action: MOUSE_DOWN, MOUSE_UP, MOUSE_MOVE, MOUSE_DRAG
	foreign static mouse_input(ctx, btn, action, x, y, cam_x, cam_y, cam_scale)

	foreign static frame(ctx, x, y, width, height)
	foreign static button(ctx, label, x, y, width, height)
	foreign static slider(ctx, label, val, x, y, height, max, verticle)
	foreign static label(ctx, label, x, y)
	foreign static checkbox(ctx, label, val, x, y)
	foreign static radio_button(ctx, label, val, x, y)
	foreign static arrow_button(ctx, dir, x, y, height, repeat)
	foreign static selectable(ctx, label, val, x, y, length)
	foreign static combo(ctx, label, curr_item, items, x, y, length)

	// ---- PanelKit: panels / clip / scroll / fields / menus / capture -----
	// See modules/gui/PanelKit.h. Coordinates are gui_top projection space
	// (y-up, screen-centered) except wheel_input (raw screen coords).

	// Per-draw-frame reset of the shared hit-rect list. Editor.draw calls
	// this ONCE before any widget (both gui contexts share the kit).
	foreign static frame_begin()

	// Stable id scopes: mix a value into subsequent uid allocations so
	// list rows keep focus/drag identity across reorders.
	foreign static push_id(n)
	foreign static pop_id()
	foreign static next_uid()

	// THE canvas gate: true when pointer is over any interactive widget or
	// panel surface, a field has keyboard focus, a menu is open, or an egui
	// drag is in flight. Editor input dispatch consults this.
	foreign static input_blocked()
	foreign static wants_keyboard()
	// Feed a named key to the focused text field; returns true when consumed.
	foreign static key_input(key)
	// Screen-coord wheel; returns true when a scroll area consumed the step.
	foreign static wheel_input(x, y, dx, dy)

	// Panel chrome: bg + header + collapse toggle. Returns true while the
	// panel is open (caller draws content only then). id must be stable
	// across frames (use next_uid).
	foreign static panel_begin(ctx, id, title, x, y, w, h)
	foreign static panel_end(ctx)

	// Culling-based clip stack: cull skips BOTH drawing and hit-testing.
	foreign static clip_begin(ctx, x, y, w, h)
	foreign static clip_end(ctx)
	foreign static clip_culls(ctx, x, y, w, h)
	foreign static clip_mouse(ctx)

	// Scroll viewport: returns content-space y offset; pairs with scrollbar
	// (same id + same content_h) and scroll_end. Wheel is routed by
	// wheel_input, NOT here.
	foreign static scroll_begin(ctx, id, x, y, w, h, content_h)
	foreign static scroll_end(ctx)
	// Programmatic locate: set the stored offset of one scroll section (the
	// upper bound clamps at that section's next scroll_begin).
	foreign static scroll_set(ctx, id, offset)
	foreign static scroll_viewport(ctx)
	foreign static scrollbar(ctx, id, bar_w, content_h)
	foreign static scrollbar_release()

	// Text field: returns the committed string on Enter/blur-commit, else
	// nil. Number field: [changed, value]; commit parses the full string
	// (optional unit suffix), failure keeps the old value.
	foreign static text_field(ctx, value, x, y, w, h)
	foreign static number_field(ctx, value, x, y, w, h)

	// Primitive draws (rgba 0..255 each).
	foreign static rect_filled(ctx, x, y, w, h, r, g, b, a)
	foreign static rect_outline(ctx, x, y, w, h, r, g, b, a)
	foreign static progress(ctx, x, y, w, h, frac, r, g, b, a)

	// Popup menu layer: menu_begin(id..) -> menu_item(..)* -> menu_end().
	// menu_item fires on click and closes the layer.
	foreign static menu_begin(ctx, id, x, y, w, h)
	foreign static menu_item(ctx, label, x, y, w, h)
	foreign static menu_end()
}

);