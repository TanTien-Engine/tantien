#pragma once

// PanelKit: immediate-mode panel toolkit layered over egui for the CAD review
// workbench. It keeps ALL cross-frame interaction state that egui lacks:
//
//   * hit rects of every interactive widget drawn last frame (so the editor
//     can keep panel gestures out of the node canvas / 3D view),
//   * panel/scroll/clip rect stacks (culling-based clipping),
//   * per-scroll-area offsets with wheel routing,
//   * keyboard focus with a text buffer for text/number fields,
//   * a popup menu layer with outside-click close,
//   * stable id scopes so list items keep their identity across reorders.
//
// Pure state + math lives here; drawing is done by the wrap_GUI bindings with
// egui primitives so every widget still renders through the shared
// RenderBuffer pipeline. Coordinates are the same y-up screen-projection
// space the gui_top context uses.

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace tt
{

class PanelKit
{
public:
	// ---- frame -----------------------------------------------------------

	// Clears last frame's hit rects and transient press state. Called once
	// per editor draw (NOT per egui context: both contexts feed this kit).
	void FrameBegin();

	// ---- hit capture -----------------------------------------------------

	// Record an interactive widget's hit rect for this frame (egui coords).
	void RecordWidget(float x, float y, float w, float h);
	// Record a passive panel/scroll viewport rect: pointer inside blocks the
	// canvas even when no widget is directly under the cursor.
	void RecordSurface(float x, float y, float w, float h);

	// Feed the converted pointer position + button action (same egui coords
	// the contexts receive). action: 0 down, 1 up, 2 move, 3 drag.
	void OnMouse(int action, float x, float y);
	// Monotonic count of press events (menu open/close sequencing).
	uint32_t PressSeq() const { return m_press_seq; }
	// Last known pointer position (egui coords).
	float MouseX() const { return m_mouse[0]; }
	float MouseY() const { return m_mouse[1]; }
	bool  HasMouse() const { return m_has_mouse; }
	// True while a press is held, plus the point where it started.
	bool PressActive() const { return m_press_active; }
	float PressX() const { return m_press_pos[0]; }
	float PressY() const { return m_press_pos[1]; }
	// Drop the current press (a kit widget consumed it; canvas must not see
	// it and later items must not re-fire).
	void ConsumePress();

	// Drag-lock mirror fed from the wrap (context state at event time).
	void SetDragLocked(bool locked) { m_drag_locked = locked; }

	// THE editor-facing query: true when the GUI owns the pointer -- pointer
	// over any recorded widget/surface, a text field has keyboard focus, a
	// kit menu is open, or an egui drag is in flight.
	bool InputBlocked() const;

	// ---- wheel -----------------------------------------------------------

	// Route a wheel step at the given point (egui coords). Returns true when
	// a scroll area under the pointer consumed the step.
	bool Wheel(float x, float y, float dx, float dy);

	// ---- id scopes -------------------------------------------------------

	// Mix a value into subsequent uid allocations (hash combine); keeps
	// list-row widgets stable across reorders.
	void PushId(uint32_t v);
	void PopId();
	// Next uid in the current scope (the wrap's uid allocator delegates here).
	uint32_t NextScopedId();

	// ---- panels ----------------------------------------------------------

	// Panel registration: records the surface rect (background blocks the
	// canvas), returns the persisted open/collapse state.
	bool PanelBegin(uint32_t id, float x, float y, float w, float h);
	void PanelSetOpen(uint32_t id, bool open);

	// ---- clip stack (culling) --------------------------------------------

	void ClipBegin(float x, float y, float w, float h);
	void ClipEnd();
	// True when the rect is fully outside the current clip (caller skips
	// both drawing AND hit-testing the widget).
	bool ClipCulls(float x, float y, float w, float h) const;
	// True when the pointer is inside the current clip.
	bool ClipContainsMouse() const;
	void ClipTop(float* x, float* y, float* w, float* h) const;

	// ---- scrolling -------------------------------------------------------

	// Begin a scroll viewport; returns the content-space y offset (how far
	// the content is scrolled up from its top). Registers the viewport for
	// wheel routing and records a surface rect for pointer capture.
	float ScrollBegin(uint32_t id, float x, float y, float w, float h, float content_h);
	void  ScrollEnd();
	// Active viewport rect during a scroll section (scrollbar drawing).
	void ScrollViewport(float* x, float* y, float* w, float* h) const;
	// Scroll the active area (wheel inside nested clip / programmatic).
	void ScrollBy(float dy);
	// Programmatic locate: set the stored offset of ONE scroll section by
	// id (clamped at the next ScrollBegin against that section's real
	// content height). Lets a UI scroll a target row into view.
	void ScrollSet(uint32_t id, float offset);
	// Track press/drag on a scrollbar for the active viewport; returns the
	// new offset. bar_x/bar_w describe the drawn track column.
	float ScrollbarDrag(uint32_t id, float bar_x, float bar_w);
	void  ScrollbarRelease();
	bool  ScrollbarDragging() const { return m_scrollbar_drag_id != 0; }

	// ---- keyboard focus / text fields ------------------------------------

	bool WantsKeyboard() const { return m_focus_id != 0; }
	// Id of the field that currently owns keyboard focus (0 = none).
	uint32_t FocusId() const { return m_focus_id; }
	// Feed a named key to the focused field ("a", "0", "space", "backspace",
	// "enter", "escape", "left", "right", "home", "end", ...). Returns true
	// when consumed. Enter ends focus WITH commit; Escape ends without.
	bool KeyInput(const std::string& key);
	// The focused field's live buffer + caret (for drawing).
	const std::string& FocusBuffer() const { return m_focus_buffer; }
	int FocusCaret() const { return m_focus_caret; }
	// Value the focused field was seeded with (to detect modifications).
	const std::string& FocusSeed() const { return m_focus_seed; }

	// Text field behaviour for one widget call. On press inside the field
	// rect the field takes focus seeded with `value`. When the field lost
	// focus with commit since its last call (Enter or click elsewhere),
	// *committed carries the final text and true is returned.
	bool TextField(uint32_t id, float x, float y, float w, float h,
		const std::string& value, std::string* committed);

	// ---- menu layer ------------------------------------------------------

	// Begin a popup menu rect; records the capture surface. Returns false
	// when another menu is open and a press outside closed the layer.
	bool MenuBegin(uint32_t id, float x, float y, float w, float h);
	// Hit test one menu row; returns true on click (and consumes the press).
	bool MenuItem(float x, float y, float w, float h);
	void MenuEnd();

private:
	struct Rect { float x = 0, y = 0, w = 0, h = 0; };

	static bool Inside(const Rect& r, float x, float y) {
		return x >= r.x && x <= r.x + r.w && y >= r.y && y <= r.y + r.h;
	}

	std::vector<Rect> m_widgets;
	std::vector<Rect> m_surfaces;

	float m_mouse[2] = { 0.0f, 0.0f };
	bool  m_has_mouse = false;
	bool  m_press_active = false;
	float m_press_pos[2] = { 0.0f, 0.0f };
	bool  m_drag_locked = false;

	uint32_t m_menu_id = 0;
	// press sequence the open menu was opened with: a press outside the menu
	// rect closes it only when it is a NEW press (not the opener still held).
	uint32_t m_menu_open_seq = 0;
	uint32_t m_press_seq = 0;

	// text field focus
	uint32_t    m_focus_id = 0;
	std::string m_focus_buffer;
	std::string m_focus_seed;
	int         m_focus_caret = 0;
	// set when a field lost focus WITH commit; consumed by TextField(id)
	uint32_t    m_commit_id = 0;
	std::string m_commit_buffer;

	// id scope stack (FNV-1a mix chain)
	std::vector<uint32_t> m_scope;
	uint32_t m_scope_hash = 0x811c9dc5;
	uint32_t m_scope_counter = 0;

	// panel open state
	std::unordered_map<uint32_t, bool> m_panel_open;

	// clip stack
	std::vector<Rect> m_clips;

	// scroll state
	struct Scroll { uint32_t id = 0; Rect viewport; float offset = 0.0f; float content_h = 0.0f; };
	std::vector<Scroll> m_scroll_stack;
	std::unordered_map<uint32_t, float> m_scroll_offset;
	uint32_t m_scrollbar_drag_id = 0;
};

}
