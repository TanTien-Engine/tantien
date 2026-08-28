#pragma once

#include "voxel_game/streaming/ChunkToken.h"
#include "voxel_game/streaming/StreamingCoord.h"
#include "voxel_game/voxel/BlockState.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace tt::voxel_game::streaming
{

inline constexpr int kSectionSide = 16;
inline constexpr std::size_t kBlocksPerSection =
    static_cast<std::size_t>(kSectionSide) * kSectionSide * kSectionSide;
inline constexpr std::size_t kFaceCells =
    static_cast<std::size_t>(kSectionSide) * kSectionSide;

enum class FaceIndex : std::uint8_t
{
    PosX = 0, NegX, PosY, NegY, PosZ, NegZ, Count
};

namespace detail
{

constexpr std::size_t block_index_unchecked(int x, int y, int z) noexcept
{
    return static_cast<std::size_t>(x) +
           kSectionSide * (static_cast<std::size_t>(z) +
           kSectionSide * static_cast<std::size_t>(y));
}

constexpr std::size_t face_cell_index_unchecked(int u, int v) noexcept
{
    return static_cast<std::size_t>(u) + kSectionSide * static_cast<std::size_t>(v);
}

inline bool known_block_state(BlockState state) noexcept
{
    switch (state) {
    case BlockState::Air:
    case BlockState::Grass:
    case BlockState::Stone:
    case BlockState::Clay:
    case BlockState::Dirt:
        return true;
    }
    return false;
}

inline void check_block_coord(int x, int y, int z)
{
    if (x < 0 || x >= kSectionSide || y < 0 || y >= kSectionSide ||
        z < 0 || z >= kSectionSide) {
        throw std::out_of_range("SectionSnapshot block coordinate out of range");
    }
}

inline void check_face(FaceIndex face, int u, int v)
{
    if (static_cast<std::uint8_t>(face) >= static_cast<std::uint8_t>(FaceIndex::Count)) {
        throw std::out_of_range("SectionSnapshot face index out of range");
    }
    if (u < 0 || u >= kSectionSide || v < 0 || v >= kSectionSide) {
        throw std::out_of_range("SectionSnapshot face cell out of range");
    }
}

inline void check_block_state(BlockState state)
{
    if (!known_block_state(state)) {
        throw std::invalid_argument("SectionSnapshot unknown BlockState");
    }
}

} // namespace detail

class SectionSnapshotBuilder;

// Immutable worker input. It has no public writable members and no public
// constructor: normal callers must transfer a builder through freeze().
class SectionSnapshot final
{
public:
    const ChunkToken& token() const noexcept { return m_token; }

    BlockState block(int x, int y, int z) const
    {
        detail::check_block_coord(x, y, z);
        const BlockState state = m_blocks[detail::block_index_unchecked(x, y, z)];
        detail::check_block_state(state);
        return state;
    }

    BlockState face_cell(FaceIndex face, int u, int v) const
    {
        detail::check_face(face, u, v);
        const BlockState state =
            m_faces[static_cast<std::size_t>(face)][detail::face_cell_index_unchecked(u, v)];
        detail::check_block_state(state);
        return state;
    }

private:
    friend class SectionSnapshotBuilder;

    explicit SectionSnapshot(ChunkToken token) : m_token(token) {}

    ChunkToken m_token{};
    std::array<BlockState, kBlocksPerSection> m_blocks{};
    std::array<std::array<BlockState, kFaceCells>,
               static_cast<std::size_t>(FaceIndex::Count)> m_faces{};
};

// Producer-only mutable phase. freeze() is rvalue-qualified and consumes the
// builder, so routine callers cannot retain a mutable alias after submission.
class SectionSnapshotBuilder final
{
public:
    explicit SectionSnapshotBuilder(ChunkToken token)
        : m_snapshot(new SectionSnapshot(token))
    {
    }

    SectionSnapshotBuilder(SectionSnapshotBuilder&&) noexcept = default;
    SectionSnapshotBuilder& operator=(SectionSnapshotBuilder&&) noexcept = default;
    SectionSnapshotBuilder(const SectionSnapshotBuilder&) = delete;
    SectionSnapshotBuilder& operator=(const SectionSnapshotBuilder&) = delete;

    void set_block(int x, int y, int z, BlockState state)
    {
        require_live();
        detail::check_block_coord(x, y, z);
        detail::check_block_state(state);
        m_snapshot->m_blocks[detail::block_index_unchecked(x, y, z)] = state;
    }

    void set_face_cell(FaceIndex face, int u, int v, BlockState state)
    {
        require_live();
        detail::check_face(face, u, v);
        detail::check_block_state(state);
        m_snapshot->m_faces[static_cast<std::size_t>(face)]
                           [detail::face_cell_index_unchecked(u, v)] = state;
    }

    std::shared_ptr<const SectionSnapshot> freeze() &&
    {
        require_live();
        return std::shared_ptr<const SectionSnapshot>(std::move(m_snapshot));
    }

private:
    void require_live() const
    {
        if (!m_snapshot) throw std::logic_error("SectionSnapshotBuilder already frozen");
    }

    std::unique_ptr<SectionSnapshot> m_snapshot;
};

using SectionSnapshotPtr = std::shared_ptr<const SectionSnapshot>;

static_assert(!std::is_default_constructible_v<SectionSnapshot>,
              "snapshots must be produced through SectionSnapshotBuilder");
static_assert(std::is_nothrow_move_constructible_v<SectionSnapshotPtr>,
              "task transfer must not throw while holding a mailbox lock");

} // namespace tt::voxel_game::streaming
