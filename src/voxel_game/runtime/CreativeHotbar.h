#pragma once

#include "voxel_game/runtime/ActionMap.h"
#include "voxel_game/voxel/BlockState.h"

#include <array>
#include <cstddef>

namespace tt::voxel_game
{

class CreativeHotbar
{
public:
    static constexpr std::size_t SlotCount = 4;

    bool select_slot(std::size_t index) noexcept;
    void apply_slot_actions(const ActionFrame& frame) noexcept;
    std::size_t selected_slot() const noexcept;
    BlockState selected_block() const noexcept;

private:
    static constexpr std::array<BlockState, SlotCount> Slots = {
        BlockState::Stone,
        BlockState::Dirt,
        BlockState::Grass,
        BlockState::Clay,
    };

    std::size_t m_selected_slot = 0;
};

} // namespace tt::voxel_game
