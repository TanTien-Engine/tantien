#include "voxel_game/runtime/ActionMap.h"

#include <algorithm>
#include <stdexcept>

namespace tt::voxel_game
{
namespace
{

template <typename Values>
bool frame_value(const Values& values, GameAction action)
{
    const std::size_t index = static_cast<std::size_t>(action);
    if (index >= values.size())
    {
        throw std::out_of_range("ActionFrame rejects an unknown GameAction");
    }
    return values[index];
}

} // namespace

bool ActionFrame::held(GameAction action) const
{
    return frame_value(m_held, action);
}

bool ActionFrame::pressed(GameAction action) const
{
    return frame_value(m_pressed, action);
}

bool ActionFrame::released(GameAction action) const
{
    return frame_value(m_released, action);
}

void ActionMap::bind_key(GameAction action, int key)
{
    bind(action, {InputKind::Key, key});
}

void ActionMap::bind_mouse_button(GameAction action, int button)
{
    bind(action, {InputKind::MouseButton, button});
}

void ActionMap::begin_frame()
{
    if (m_frame_open && !m_sampled)
    {
        throw std::logic_error("ActionMap requires exactly one sample before the next frame");
    }
    m_frame_open = true;
    m_sampled = false;
}

ActionFrame ActionMap::sample(const InputState& input)
{
    if (!m_frame_open || m_sampled)
    {
        throw std::logic_error("ActionMap::sample requires one open, unsampled frame");
    }
    m_sampled = true;
    ActionFrame frame;
    for (std::size_t index = 0; index < ActionCount; ++index)
    {
        bool now_held = false;
        bool raw_pressed = false;
        bool raw_released = false;
        for (const Binding& binding : m_bindings[index])
        {
            now_held = now_held || binding_held(input, binding);
            raw_pressed = raw_pressed || binding_pressed(input, binding);
            raw_released = raw_released || binding_released(input, binding);
        }
        const bool was_held = m_action_held[index];
        frame.m_held[index] = now_held;
        frame.m_pressed[index] = !was_held && (now_held || raw_pressed);
        frame.m_released[index] = !now_held && (was_held || raw_released);
        m_action_held[index] = now_held;
    }
    return frame;
}

std::size_t ActionMap::action_index(GameAction action)
{
    const std::size_t index = static_cast<std::size_t>(action);
    if (index >= ActionCount)
    {
        throw std::out_of_range("ActionMap rejects an unknown GameAction");
    }
    return index;
}

void ActionMap::validate_code(int code)
{
    if (code < 0)
    {
        throw std::invalid_argument("ActionMap rejects a negative raw input code");
    }
}

void ActionMap::bind(GameAction action, Binding binding)
{
    const std::size_t index = action_index(action);
    validate_code(binding.code);
    std::vector<Binding>& bindings = m_bindings[index];
    if (std::find(bindings.begin(), bindings.end(), binding) == bindings.end())
    {
        bindings.push_back(binding);
    }
}

bool ActionMap::binding_held(const InputState& input, const Binding& binding)
{
    return binding.kind == InputKind::Key ? input.held(binding.code)
                                           : input.mouse_button_held(binding.code);
}

bool ActionMap::binding_pressed(const InputState& input, const Binding& binding)
{
    return binding.kind == InputKind::Key ? input.pressed(binding.code)
                                           : input.mouse_button_pressed(binding.code);
}

bool ActionMap::binding_released(const InputState& input, const Binding& binding)
{
    return binding.kind == InputKind::Key ? input.released(binding.code)
                                           : input.mouse_button_released(binding.code);
}

} // namespace tt::voxel_game
