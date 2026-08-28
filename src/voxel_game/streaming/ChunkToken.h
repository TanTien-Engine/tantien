#pragma once

#include "voxel_game/streaming/StreamingCoord.h"

#include <cstdint>
#include <limits>

namespace tt::voxel_game::streaming
{

// A revision deliberately has no increment operator. Callers must use
// try_bump(), which refuses overflow instead of silently creating an ABA
// token match after wraparound.
class TokenRevision
{
public:
    constexpr TokenRevision() = default;
    constexpr TokenRevision(std::uint64_t value) : m_value(value) {}

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

// Identifies the exact world state a streaming job was computed from. All
// four fields must match the authoritative state again when the result comes
// back; any single mismatch means the result is stale and must be dropped.
struct ChunkToken
{
    SectionCoord coord{};
    TokenRevision load_generation{};       // bumps on section unload/reload
    TokenRevision block_revision{};        // bumps on block edits inside the section
    TokenRevision neighborhood_revision{}; // bumps on face-neighbor section changes

    friend bool operator==(const ChunkToken& a, const ChunkToken& b)
    {
        return a.coord == b.coord &&
               a.load_generation == b.load_generation &&
               a.block_revision == b.block_revision &&
               a.neighborhood_revision == b.neighborhood_revision;
    }

    friend bool operator!=(const ChunkToken& a, const ChunkToken& b)
    {
        return !(a == b);
    }
};

enum class TokenVerdict : std::uint8_t
{
    Current,
    StaleCoord,
    StaleGeneration,
    StaleBlockRevision,
    StaleNeighborhoodRevision,
};

// Pure validation. `produced` is the token stamped on a worker result;
// `authoritative` is the token the game currently holds for that section.
inline TokenVerdict validate_token(const ChunkToken& produced, const ChunkToken& authoritative)
{
    if (produced.coord != authoritative.coord) return TokenVerdict::StaleCoord;
    if (produced.load_generation != authoritative.load_generation) return TokenVerdict::StaleGeneration;
    if (produced.block_revision != authoritative.block_revision) return TokenVerdict::StaleBlockRevision;
    if (produced.neighborhood_revision != authoritative.neighborhood_revision) return TokenVerdict::StaleNeighborhoodRevision;
    return TokenVerdict::Current;
}

inline bool token_is_current(const ChunkToken& produced, const ChunkToken& authoritative)
{
    return validate_token(produced, authoritative) == TokenVerdict::Current;
}

} // namespace tt::voxel_game::streaming
