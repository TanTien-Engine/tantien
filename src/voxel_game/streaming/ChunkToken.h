#pragma once

#include "voxel_game/streaming/StreamingCoord.h"

#include <cstdint>
#include <limits>

namespace tt::voxel_game::streaming
{

class TokenRevision
{
public:
    constexpr TokenRevision() = default;
    constexpr explicit TokenRevision(std::uint64_t value) : m_value(value) {}
    constexpr std::uint64_t value() const noexcept { return m_value; }
    bool try_bump() noexcept
    {
        if (m_value == std::numeric_limits<std::uint64_t>::max()) return false;
        ++m_value;
        return true;
    }
    friend constexpr bool operator==(TokenRevision a, TokenRevision b) noexcept
    {
        return a.m_value == b.m_value;
    }
    friend constexpr bool operator!=(TokenRevision a, TokenRevision b) noexcept
    {
        return !(a == b);
    }
private:
    std::uint64_t m_value = 0;
};

class WorldEpoch
{
public:
    constexpr WorldEpoch() = default;
    constexpr explicit WorldEpoch(std::uint64_t value) : m_value(value) {}
    constexpr std::uint64_t value() const noexcept { return m_value; }
    friend constexpr bool operator==(WorldEpoch a, WorldEpoch b) noexcept
    {
        return a.m_value == b.m_value;
    }
    friend constexpr bool operator!=(WorldEpoch a, WorldEpoch b) noexcept
    {
        return !(a == b);
    }
private:
    std::uint64_t m_value = 0;
};

// Immutable transport value. Mutable authoritative state is kept separately
// in ChunkTokenAuthority; normal callers cannot roll a token backward.
class ChunkToken
{
public:
    constexpr ChunkToken() = default;
    constexpr ChunkToken(SectionCoord coord, WorldEpoch epoch,
                         TokenRevision generation, TokenRevision block,
                         TokenRevision neighborhood) noexcept
        : m_coord(coord), m_world_epoch(epoch), m_load_generation(generation),
          m_block_revision(block), m_neighborhood_revision(neighborhood) {}

    constexpr const SectionCoord& coord() const noexcept { return m_coord; }
    constexpr WorldEpoch world_epoch() const noexcept { return m_world_epoch; }
    constexpr TokenRevision load_generation() const noexcept { return m_load_generation; }
    constexpr TokenRevision block_revision() const noexcept { return m_block_revision; }
    constexpr TokenRevision neighborhood_revision() const noexcept { return m_neighborhood_revision; }

    friend bool operator==(const ChunkToken& a, const ChunkToken& b) noexcept
    {
        return a.m_coord == b.m_coord && a.m_world_epoch == b.m_world_epoch &&
               a.m_load_generation == b.m_load_generation &&
               a.m_block_revision == b.m_block_revision &&
               a.m_neighborhood_revision == b.m_neighborhood_revision;
    }
    friend bool operator!=(const ChunkToken& a, const ChunkToken& b) noexcept
    {
        return !(a == b);
    }
private:
    SectionCoord m_coord{};
    WorldEpoch m_world_epoch{};
    TokenRevision m_load_generation{};
    TokenRevision m_block_revision{};
    TokenRevision m_neighborhood_revision{};
};

class ChunkTokenAuthority
{
public:
    explicit ChunkTokenAuthority(SectionCoord coord, WorldEpoch epoch = WorldEpoch{1})
        : m_coord(coord), m_epoch(epoch) {}

    ChunkToken snapshot() const noexcept
    {
        return ChunkToken(m_coord, m_epoch, m_generation, m_block, m_neighborhood);
    }
    bool exhausted() const noexcept { return m_exhausted; }
    bool try_bump_load_generation() noexcept { return bump(m_generation); }
    bool try_bump_block_revision() noexcept { return bump(m_block); }
    bool try_bump_neighborhood_revision() noexcept { return bump(m_neighborhood); }
private:
    bool bump(TokenRevision& revision) noexcept
    {
        if (m_exhausted || !revision.try_bump()) {
            m_exhausted = true;
            return false;
        }
        return true;
    }
    SectionCoord m_coord{};
    WorldEpoch m_epoch{};
    TokenRevision m_generation{};
    TokenRevision m_block{};
    TokenRevision m_neighborhood{};
    bool m_exhausted = false;
};

enum class TokenVerdict : std::uint8_t
{
    Current, StaleCoord, StaleWorldEpoch, StaleGeneration,
    StaleBlockRevision, StaleNeighborhoodRevision,
};

inline TokenVerdict validate_token(const ChunkToken& produced,
                                   const ChunkToken& authoritative) noexcept
{
    if (produced.coord() != authoritative.coord()) return TokenVerdict::StaleCoord;
    if (produced.world_epoch() != authoritative.world_epoch()) return TokenVerdict::StaleWorldEpoch;
    if (produced.load_generation() != authoritative.load_generation()) return TokenVerdict::StaleGeneration;
    if (produced.block_revision() != authoritative.block_revision()) return TokenVerdict::StaleBlockRevision;
    if (produced.neighborhood_revision() != authoritative.neighborhood_revision()) {
        return TokenVerdict::StaleNeighborhoodRevision;
    }
    return TokenVerdict::Current;
}

inline bool token_is_current(const ChunkToken& produced, const ChunkToken& authoritative) noexcept
{
    return validate_token(produced, authoritative) == TokenVerdict::Current;
}

} // namespace tt::voxel_game::streaming
