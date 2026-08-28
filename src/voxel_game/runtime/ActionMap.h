#pragma once

#include "voxel_game/runtime/InputState.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace tt::voxel_game
{

enum class GameAction : std::uint8_t
{
    MoveForward = 0,
    MoveBackward,
    MoveLeft,
    MoveRight,
    Jump,
    BreakBlock,
    PlaceBlock,
    SelectSlot1,
    SelectSlot2,
    SelectSlot3,
    SelectSlot4,
    Count,
};

class ActionFrame
{
public:
    bool held(GameAction action) const;
    bool pressed(GameAction action) const;
    bool released(GameAction action) const;

private:
    friend class ActionMap;

    static constexpr std::size_t ActionCount = static_cast<std::size_t>(GameAction::Count);

    std::array<bool, ActionCount> m_held{};
    std::array<bool, ActionCount> m_pressed{};
    std::array<bool, ActionCount> m_released{};
};

class ActionMap
{
public:
    void bind_key(GameAction action, int key);
    void bind_mouse_button(GameAction action, int button);

    void begin_frame();
    ActionFrame sample(const InputState& input);

private:
    enum class InputKind : std::uint8_t
    {
        Key,
        MouseButton,
    };

    struct Binding
    {
        InputKind kind = InputKind::Key;
        int code = 0;

        bool operator==(const Binding& other) const
        {
            return kind == other.kind && code == other.code;
        }
    };

    static constexpr std::size_t ActionCount = static_cast<std::size_t>(GameAction::Count);

    static std::size_t action_index(GameAction action);
    static void validate_code(int code);

    void bind(GameAction action, Binding binding);
    static bool binding_held(const InputState& input, const Binding& binding);
    static bool binding_pressed(const InputState& input, const Binding& binding);
    static bool binding_released(const InputState& input, const Binding& binding);

    std::array<std::vector<Binding>, ActionCount> m_bindings;
    std::array<bool, ActionCount> m_action_held{};
    bool m_frame_open = false;
    bool m_sampled = false;
};

} // namespace tt::voxel_game
