#include "modules/gui/PanelKit.h"

#include <cstdlib>

namespace tt
{

// ---------------------------------------------------------------- frame ---

void PanelKit::FrameBegin()
{
	m_widgets.clear();
	m_surfaces.clear();
	m_has_mouse = false;
	m_scope.clear();
	m_scope_hash = 0x811c9dc5;
	m_scope_counter = 0;
	m_clips.clear();
	m_scroll_stack.clear();
	m_scrollbar_drag_id = 0;
	// Focus and menu state intentionally survive across frames: focus is
	// only released by a release/Enter/Escape, never by frame boundaries.
}

// ----------------------------------------------------------- hit capture ---

void PanelKit::RecordWidget(float x, float y, float w, float h)
{
	if (m_widgets.size() < 8192) {
		m_widgets.push_back({ x, y, w, h });
	}
}

void PanelKit::RecordSurface(float x, float y, float w, float h)
{
	if (m_surfaces.size() < 1024) {
		m_surfaces.push_back({ x, y, w, h });
	}
}

void PanelKit::OnMouse(int action, float x, float y)
{
	m_mouse[0] = x;
	m_mouse[1] = y;
	m_has_mouse = true;
	if (action == 0)
	{
		m_press_active = true;
		m_press_pos[0] = x;
		m_press_pos[1] = y;
		m_press_seq++;
	}
	else if (action == 1)
	{
		m_press_active = false;
		// A release while a field is focused = click elsewhere = commit the
		// typed buffer (standard blur-commit) and drop focus.
		if (m_focus_id != 0)
		{
			m_commit_id = m_focus_id;
			m_commit_buffer = m_focus_buffer;
			m_focus_id = 0;
		}
	}
}

void PanelKit::ConsumePress()
{
	m_press_active = false;
	m_press_pos[0] = -1e9f;
	m_press_pos[1] = -1e9f;
}

bool PanelKit::InputBlocked() const
{
	if (m_drag_locked || m_focus_id != 0 || m_menu_id != 0) {
		return true;
	}
	if (!m_has_mouse) {
		return false;
	}
	for (auto& r : m_widgets) {
		if (Inside(r, m_mouse[0], m_mouse[1])) {
			return true;
		}
	}
	for (auto& r : m_surfaces) {
		if (Inside(r, m_mouse[0], m_mouse[1])) {
			return true;
		}
	}
	return false;
}

// ----------------------------------------------------------------- wheel ---

bool PanelKit::Wheel(float x, float y, float dx, float dy)
{
	if (m_drag_locked || m_scrollbar_drag_id != 0) {
		// A scrollbar drag owns the gesture; the wheel must not fight it.
		return m_scrollbar_drag_id != 0;
	}
	// Topmost (most recently begun) scroll area under the pointer wins.
	// An area whose content does NOT overflow must NOT consume the wheel:
	// the step falls through to the canvas (zoom), exactly like hovering
	// bare canvas.
	for (int i = static_cast<int>(m_scroll_stack.size()) - 1; i >= 0; --i)
	{
		Scroll& s = m_scroll_stack[i];
		if (!Inside(s.viewport, x, y)) {
			continue;
		}
		const float view_h = s.viewport.h;
		const float max_off = s.content_h > view_h ? s.content_h - view_h : 0.0f;
		if (max_off <= 0.0f) {
			continue;
		}
		float next = s.offset - dy * 24.0f;
		if (next < 0.0f) {
			next = 0.0f;
		}
		if (next > max_off) {
			next = max_off;
		}
		s.offset = next;
		m_scroll_offset[s.id] = next;
		return true;
	}
	return false;
}

// ------------------------------------------------------------ id scopes ---

void PanelKit::PushId(uint32_t v)
{
	// FNV-1a step mixing the caller value into the running scope hash; the
	// result seeds subsequent uid allocations so widgets in a list row keep
	// their identity when the row moves.
	m_scope.push_back(m_scope_hash);
	m_scope_hash = (m_scope_hash ^ v) * 0x01000193u;
	m_scope_counter = 0;
}

void PanelKit::PopId()
{
	if (!m_scope.empty()) {
		m_scope_hash = m_scope.back();
		m_scope.pop_back();
	}
	m_scope_counter = 0;
}

uint32_t PanelKit::NextScopedId()
{
	// Hash the scope with a per-call counter: stable for the same call
	// sequence under the same scope, distinct between siblings.
	const uint32_t h = (m_scope_hash ^ (m_scope_counter++ * 0x9e3779b9u)) * 0x01000193u;
	// egui treats id 0 as "no item" -- keep it out of the id space.
	return h == 0 ? 1u : h;
}

// ---------------------------------------------------------------- panels ---

bool PanelKit::PanelBegin(uint32_t id, float x, float y, float w, float h)
{
	RecordSurface(x, y, w, h);
	auto it = m_panel_open.find(id);
	if (it == m_panel_open.end()) {
		m_panel_open[id] = true;
		return true;
	}
	return it->second;
}

void PanelKit::PanelSetOpen(uint32_t id, bool open)
{
	m_panel_open[id] = open;
}

// ------------------------------------------------------------ clip stack ---

void PanelKit::ClipBegin(float x, float y, float w, float h)
{
	if (!m_clips.empty())
	{
		// Intersect with the parent clip: nested viewports can only shrink.
		const Rect& p = m_clips.back();
		const float x0 = x > p.x ? x : p.x;
		const float y0 = y > p.y ? y : p.y;
		const float x1 = x + w < p.x + p.w ? x + w : p.x + p.w;
		const float y1 = y + h < p.y + p.h ? y + h : p.y + p.h;
		m_clips.push_back({ x0, y0, x1 > x0 ? x1 - x0 : 0.0f, y1 > y0 ? y1 - y0 : 0.0f });
	}
	else
	{
		m_clips.push_back({ x, y, w, h });
	}
}

void PanelKit::ClipEnd()
{
	if (!m_clips.empty()) {
		m_clips.pop_back();
	}
}

bool PanelKit::ClipCulls(float x, float y, float w, float h) const
{
	if (m_clips.empty()) {
		return false;
	}
	const Rect& c = m_clips.back();
	if (c.w <= 0 || c.h <= 0) {
		return true;
	}
	return x + w <= c.x || x >= c.x + c.w || y + h <= c.y || y >= c.y + c.h;
}

bool PanelKit::ClipContainsMouse() const
{
	if (m_clips.empty()) {
		return true;
	}
	if (!m_has_mouse) {
		return false;
	}
	const Rect& c = m_clips.back();
	return Inside(c, m_mouse[0], m_mouse[1]);
}

void PanelKit::ClipTop(float* x, float* y, float* w, float* h) const
{
	if (m_clips.empty())
	{
		*x = 0; *y = 0; *w = 0; *h = 0;
		return;
	}
	const Rect& c = m_clips.back();
	*x = c.x; *y = c.y; *w = c.w; *h = c.h;
}

// -------------------------------------------------------------- scrolling ---

float PanelKit::ScrollBegin(uint32_t id, float x, float y, float w, float h, float content_h)
{
	Scroll s;
	s.id = id;
	s.viewport = { x, y, w, h };
	s.content_h = content_h > 0.0f ? content_h : 0.0f;
	auto it = m_scroll_offset.find(id);
	s.offset = it != m_scroll_offset.end() ? it->second : 0.0f;
	const float max_off = s.content_h > h ? s.content_h - h : 0.0f;
	if (s.offset > max_off)
	{
		s.offset = max_off;
		m_scroll_offset[id] = max_off;
	}
	m_scroll_stack.push_back(s);
	RecordSurface(x, y, w, h);
	return s.offset;
}

void PanelKit::ScrollEnd()
{
	if (!m_scroll_stack.empty())
	{
		m_scroll_offset[m_scroll_stack.back().id] = m_scroll_stack.back().offset;
		m_scroll_stack.pop_back();
	}
}

void PanelKit::ScrollViewport(float* x, float* y, float* w, float* h) const
{
	if (m_scroll_stack.empty())
	{
		*x = 0; *y = 0; *w = 0; *h = 0;
		return;
	}
	const Rect& r = m_scroll_stack.back().viewport;
	*x = r.x; *y = r.y; *w = r.w; *h = r.h;
}

void PanelKit::ScrollBy(float dy)
{
	if (m_scroll_stack.empty()) {
		return;
	}
	Scroll& s = m_scroll_stack.back();
	const float max_off = s.content_h > s.viewport.h ? s.content_h - s.viewport.h : 0.0f;
	float next = s.offset + dy;
	if (next < 0.0f) {
		next = 0.0f;
	}
	if (next > max_off) {
		next = max_off;
	}
	s.offset = next;
	m_scroll_offset[s.id] = next;
}

float PanelKit::ScrollbarDrag(uint32_t id, float bar_x, float bar_w)
{
	if (m_scroll_stack.empty()) {
		return 0.0f;
	}
	Scroll& s = m_scroll_stack.back();
	const float view_h = s.viewport.h;
	const float max_off = s.content_h > view_h ? s.content_h - view_h : 0.0f;
	if (max_off <= 0.0f) {
		return s.offset;
	}

	// Bar geometry mirrors the wrap's drawing: the track spans the viewport
	// height; the grab occupies view_h * view_h / content_h.
	const float track_y = s.viewport.y;
	const float track_h = view_h;
	const float grab_h = view_h < s.content_h ? view_h * view_h / s.content_h : track_h;
	const float grab_center_off = grab_h * 0.5f;

	if (m_scrollbar_drag_id == id)
	{
		// Active drag: follow the pointer even outside the track.
		float t = (m_mouse[1] - grab_center_off - track_y) / (track_h - grab_h);
		if (t < 0.0f) { t = 0.0f; }
		if (t > 1.0f) { t = 1.0f; }
		s.offset = t * max_off;
		m_scroll_offset[id] = s.offset;
		return s.offset;
	}

	// Begin: a held press inside the track column starts a drag.
	if (m_press_active && m_has_mouse
		&& m_press_pos[0] >= bar_x && m_press_pos[0] <= bar_x + bar_w
		&& m_press_pos[1] >= track_y && m_press_pos[1] <= track_y + track_h)
	{
		m_scrollbar_drag_id = id;
		float t = (m_press_pos[1] - grab_center_off - track_y) / (track_h - grab_h);
		if (t < 0.0f) { t = 0.0f; }
		if (t > 1.0f) { t = 1.0f; }
		s.offset = t * max_off;
		m_scroll_offset[id] = s.offset;
		ConsumePress();
	}
	return s.offset;
}

void PanelKit::ScrollbarRelease()
{
	m_scrollbar_drag_id = 0;
}

// --------------------------------------------------------- text fields ----

bool PanelKit::KeyInput(const std::string& key)
{
	if (m_focus_id == 0) {
		return false;
	}
	if (key == "escape")
	{
		// Cancel: drop buffer and focus, no commit.
		m_focus_id = 0;
		return true;
	}
	if (key == "enter")
	{
		m_commit_id = m_focus_id;
		m_commit_buffer = m_focus_buffer;
		m_focus_id = 0;
		return true;
	}
	if (key == "backspace")
	{
		if (m_focus_caret > 0)
		{
			// Erase one UTF-8 character before the caret.
			int i = m_focus_caret - 1;
			while (i > 0 && (static_cast<unsigned char>(m_focus_buffer[i]) & 0xC0) == 0x80) {
				i--;
			}
			m_focus_buffer.erase(static_cast<size_t>(i), static_cast<size_t>(m_focus_caret - i));
			m_focus_caret = i;
		}
		return true;
	}
	if (key == "delete")
	{
		if (m_focus_caret < static_cast<int>(m_focus_buffer.size())) {
			m_focus_buffer.erase(static_cast<size_t>(m_focus_caret), 1);
		}
		return true;
	}
	if (key == "left")
	{
		if (m_focus_caret > 0) {
			m_focus_caret--;
			while (m_focus_caret > 0
				&& (static_cast<unsigned char>(m_focus_buffer[m_focus_caret]) & 0xC0) == 0x80) {
				m_focus_caret--;
			}
		}
		return true;
	}
	if (key == "right")
	{
		if (m_focus_caret < static_cast<int>(m_focus_buffer.size())) {
			m_focus_caret++;
			while (m_focus_caret < static_cast<int>(m_focus_buffer.size())
				&& (static_cast<unsigned char>(m_focus_buffer[m_focus_caret]) & 0xC0) == 0x80) {
				m_focus_caret++;
			}
		}
		return true;
	}
	if (key == "home") { m_focus_caret = 0; return true; }
	if (key == "end")  { m_focus_caret = static_cast<int>(m_focus_buffer.size()); return true; }

	if (key == "space")
	{
		m_focus_buffer.insert(static_cast<size_t>(m_focus_caret), 1, ' ');
		m_focus_caret++;
		return true;
	}
	if (key.size() == 1)
	{
		const unsigned char c = static_cast<unsigned char>(key[0]);
		if (c >= 0x20 && c < 0x7F) {
			m_focus_buffer.insert(static_cast<size_t>(m_focus_caret), 1, static_cast<char>(c));
			m_focus_caret++;
			return true;
		}
	}
	// Named keys the field does not use are still consumed: the focused
	// field owns the keyboard while it exists.
	return true;
}

bool PanelKit::TextField(uint32_t id, float x, float y, float w, float h,
	const std::string& value, std::string* committed)
{
	// Acquire focus on a held press inside the field rect.
	if (m_press_active && m_has_mouse
		&& m_press_pos[0] >= x && m_press_pos[0] <= x + w
		&& m_press_pos[1] >= y && m_press_pos[1] <= y + h)
	{
		if (m_focus_id != id)
		{
			m_focus_id = id;
			m_focus_buffer = value;
			m_focus_seed = value;
			m_focus_caret = static_cast<int>(m_focus_buffer.size());
		}
		ConsumePress();
	}

	// Report a commit that happened since this field's previous call.
	if (m_commit_id == id)
	{
		m_commit_id = 0;
		*committed = m_commit_buffer;
		return true;
	}
	return false;
}

// ----------------------------------------------------------------- menus ----

bool PanelKit::MenuBegin(uint32_t id, float x, float y, float w, float h)
{
	if (m_menu_id != 0 && m_menu_id != id)
	{
		// Another menu is open: a press outside this new menu's rect (that
		// is not the press that opened the layer) closes the layer and the
		// caller must not draw.
		if (m_press_active && m_press_seq != m_menu_open_seq
			&& !(m_press_pos[0] >= x && m_press_pos[0] <= x + w
				&& m_press_pos[1] >= y && m_press_pos[1] <= y + h))
		{
			m_menu_id = 0;
			return false;
		}
	}
	if (m_menu_id == 0 || m_menu_id != id) {
		m_menu_open_seq = m_press_seq;
	}
	m_menu_id = id;
	RecordSurface(x, y, w, h);
	return true;
}

bool PanelKit::MenuItem(float x, float y, float w, float h)
{
	RecordWidget(x, y, w, h);
	if (!m_press_active || !m_has_mouse) {
		return false;
	}
	if (m_press_pos[0] < x || m_press_pos[0] > x + w
		|| m_press_pos[1] < y || m_press_pos[1] > y + h) {
		return false;
	}
	// Consume: the press must not also reach the canvas or another row.
	ConsumePress();
	m_menu_id = 0;
	return true;
}

void PanelKit::MenuEnd()
{
	m_menu_id = 0;
}

}
