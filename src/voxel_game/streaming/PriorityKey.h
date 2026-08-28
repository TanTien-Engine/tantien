#pragma once

#include "voxel_game/streaming/StreamingCoord.h"

#include <cstdint>

namespace tt::voxel_game::streaming
{

// Deterministic scheduling key. Order: in-frustum sections first, then
// smaller squared section distance, then lexicographic coordinate.
struct PriorityKey
{
    bool in_frustum = false;
    std::uint64_t distance_squared = 0;
    SectionCoord coord{};

    friend bool operator==(const PriorityKey& a, const PriorityKey& b)
    {
        return a.in_frustum == b.in_frustum &&
               a.distance_squared == b.distance_squared &&
               a.coord == b.coord;
    }
};

// True when `a` must be scheduled strictly before `b`.
inline bool priority_before(const PriorityKey& a, const PriorityKey& b)
{
    if (a.in_frustum != b.in_frustum) return a.in_frustum;
    if (a.distance_squared != b.distance_squared) return a.distance_squared < b.distance_squared;
    return a.coord < b.coord;
}

inline PriorityKey make_priority_key(SectionCoord coord, bool in_frustum,
                                     SectionCoord camera_section)
{
    PriorityKey key;
    key.in_frustum = in_frustum;
    key.distance_squared = section_distance_squared(coord, camera_section);
    key.coord = coord;
    return key;
}

// SequencedKey adds the monotonically increasing submission counter: equal
// keys resolve FIFO by submission order, so the total order is strict and no
// container-internal iteration order can influence scheduling results.
struct SequencedKey
{
    PriorityKey key{};
    std::uint64_t sequence = 0;
};

inline bool sequenced_before(const SequencedKey& a, const SequencedKey& b)
{
    if (!(a.key == b.key)) return priority_before(a.key, b.key);
    return a.sequence < b.sequence;
}

} // namespace tt::voxel_game::streaming
