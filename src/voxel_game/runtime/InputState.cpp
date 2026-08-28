#include "voxel_game/runtime/InputState.h"

namespace tt::voxel_game
{

void InputState::begin_frame()
{
    m_pressed_keys.clear();
    m_released_keys.clear();
    m_pressed_mouse_buttons.clear();
    m_released_mouse_buttons.clear();
    m_relative_mouse_delta = {};
}

void InputState::on_key(int key, ButtonAction action)
{
    update_button(m_held_keys, m_pressed_keys, m_released_keys, key, action);
    if (key == EscapeKey && action == ButtonAction::Press)
    {
        set_mouse_captured(false);
    }
}

void InputState::on_mouse_button(int button, ButtonAction action)
{
    update_button(m_held_mouse_buttons,
                  m_pressed_mouse_buttons,
                  m_released_mouse_buttons,
                  button,
                  action);
}

void InputState::on_cursor_position(double x, double y)
{
    if (m_ignore_next_cursor_sample)
    {
        m_last_cursor_x = x;
        m_last_cursor_y = y;
        m_ignore_next_cursor_sample = false;
        return;
    }

    if (m_mouse_captured)
    {
        m_relative_mouse_delta.x += x - m_last_cursor_x;
        m_relative_mouse_delta.y += y - m_last_cursor_y;
    }

    m_last_cursor_x = x;
    m_last_cursor_y = y;
}

void InputState::set_mouse_captured(bool captured)
{
    if (m_mouse_captured == captured)
    {
        return;
    }

    m_mouse_captured = captured;
    m_relative_mouse_delta = {};
    m_ignore_next_cursor_sample = true;
}

bool InputState::mouse_captured() const
{
    return m_mouse_captured;
}

MouseDelta InputState::relative_mouse_delta() const
{
    return m_relative_mouse_delta;
}

bool InputState::held(int key) const
{
    return m_held_keys.find(key) != m_held_keys.end();
}

bool InputState::pressed(int key) const
{
    return m_pressed_keys.find(key) != m_pressed_keys.end();
}

bool InputState::released(int key) const
{
    return m_released_keys.find(key) != m_released_keys.end();
}

bool InputState::mouse_button_held(int button) const
{
    return m_held_mouse_buttons.find(button) != m_held_mouse_buttons.end();
}

bool InputState::mouse_button_pressed(int button) const
{
    return m_pressed_mouse_buttons.find(button) != m_pressed_mouse_buttons.end();
}

bool InputState::mouse_button_released(int button) const
{
    return m_released_mouse_buttons.find(button) != m_released_mouse_buttons.end();
}

void InputState::update_button(std::unordered_set<int>& held_buttons,
                               std::unordered_set<int>& pressed_buttons,
                               std::unordered_set<int>& released_buttons,
                               int button,
                               ButtonAction action)
{
    if (action == ButtonAction::Press)
    {
        if (held_buttons.insert(button).second)
        {
            pressed_buttons.insert(button);
        }
        return;
    }

    if (action == ButtonAction::Release && held_buttons.erase(button) != 0)
    {
        released_buttons.insert(button);
    }
}

} // namespace tt::voxel_game
