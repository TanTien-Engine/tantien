#pragma once

#include <unordered_set>

namespace tt::voxel_game
{

enum class ButtonAction
{
    Press,
    Release,
    Repeat,
};

struct MouseDelta
{
    double x = 0.0;
    double y = 0.0;
};

class InputState
{
public:
    static constexpr int EscapeKey = 256;

    void begin_frame();
    void on_key(int key, ButtonAction action);
    void on_mouse_button(int button, ButtonAction action);
    void on_cursor_position(double x, double y);

    void set_mouse_captured(bool captured);
    bool mouse_captured() const;
    MouseDelta relative_mouse_delta() const;

    bool held(int key) const;
    bool pressed(int key) const;
    bool released(int key) const;
    bool mouse_button_held(int button) const;
    bool mouse_button_pressed(int button) const;
    bool mouse_button_released(int button) const;

private:
    void update_button(std::unordered_set<int>& held_buttons,
                       std::unordered_set<int>& pressed_buttons,
                       std::unordered_set<int>& released_buttons,
                       int button,
                       ButtonAction action);

    std::unordered_set<int> m_held_keys;
    std::unordered_set<int> m_pressed_keys;
    std::unordered_set<int> m_released_keys;
    std::unordered_set<int> m_held_mouse_buttons;
    std::unordered_set<int> m_pressed_mouse_buttons;
    std::unordered_set<int> m_released_mouse_buttons;
    MouseDelta m_relative_mouse_delta;
    double m_last_cursor_x = 0.0;
    double m_last_cursor_y = 0.0;
    bool m_mouse_captured = false;
    bool m_ignore_next_cursor_sample = true;
};

} // namespace tt::voxel_game
