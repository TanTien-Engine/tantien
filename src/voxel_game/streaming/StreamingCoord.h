#pragma once

// Pure-CPU streaming primitives for voxel_game chunk work. Nothing in the
// streaming headers may reference engine runtime objects (no scene handles,
// no render/GPU/VM types). That is a header-level convention enforced by
// review, not something the type system proves.

#include <cstddef>
#include <cstdint>

namespace tt::voxel_game::streaming
{

// Section-space coordinate, field-compatible with voxel_game::SectionCoord
// in voxel/World.h. Deliberately an independent type so streaming headers
// never include World.h; the bridge converts between the two trivially.
struct SectionCoord
{
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::int32_t z = 0;

    friend bool operator==(const SectionCoord& a, const SectionCoord& b)
    {
        return a.x == b.x && a.y == b.y && a.z == b.z;
    }

    friend bool operator!=(const SectionCoord& a, const SectionCoord& b)
    {
        return !(a == b);
    }

    // Lexicographic total order; the final deterministic tie-break of the
    // scheduling priority key.
    friend bool operator<(const SectionCoord& a, const SectionCoord& b)
    {
        if (a.x != b.x) return a.x < b.x;
        if (a.y != b.y) return a.y < b.y;
        return a.z < b.z;
    }
};

// Standard 64-bit FNV-1a over bytes (offset basis 0xcbf29ce484222325,
// prime 0x100000001b3). Golden vectors: fnv1a64("") == 14695981039346656037,
// fnv1a64("a") == 0xaf63dc4c8601ec8c.
inline std::uint64_t fnv1a64(const std::uint8_t* data, std::size_t size) noexcept
{
    std::uint64_t hash = 14695981039346656037ull;
    for (std::size_t i = 0; i < size; ++i) {
        hash ^= static_cast<std::uint64_t>(data[i]);
        hash *= 1099511628211ull;
    }
    return hash;
}

// Hash for SectionCoord: FNV-1a over the three coordinates in little-endian
// byte order (endianness-independent by construction). Golden pin for
// regression: hash({0,0,0}) == 0x5467b0da1d106495.
struct SectionCoordHash
{
    std::size_t operator()(const SectionCoord& coordinate) const noexcept
    {
        std::uint64_t hash = 14695981039346656037ull;
        const std::uint32_t parts[3] = {
            static_cast<std::uint32_t>(coordinate.x),
            static_cast<std::uint32_t>(coordinate.y),
            static_cast<std::uint32_t>(coordinate.z),
        };
        for (std::uint32_t part : parts) {
            for (int byte = 0; byte < 4; ++byte) {
                hash ^= static_cast<std::uint64_t>((part >> (8 * byte)) & 0xffu);
                hash *= 1099511628211ull;
            }
        }
        return static_cast<std::size_t>(hash);
    }
};

// Per-axis |a - b| is at most 2^32 - 1, so each squared axis delta fits in
// 64 bits exactly ((2^32-1)^2 < 2^64). The three-way sum can exceed 2^64, so
// it saturates instead of wrapping: coordinates at the INT32 extremes are
// valid inputs.
inline std::uint64_t saturating_add(std::uint64_t a, std::uint64_t b, std::uint64_t c) noexcept
{
    const std::uint64_t ab = a > UINT64_MAX - b ? UINT64_MAX : a + b;
    return ab > UINT64_MAX - c ? UINT64_MAX : ab + c;
}

inline std::uint64_t section_distance_squared(const SectionCoord& a,
                                              const SectionCoord& b) noexcept
{
    const auto axis = [](std::int32_t pa, std::int32_t pb) {
        const std::int64_t delta =
            static_cast<std::int64_t>(pa) - static_cast<std::int64_t>(pb);
        const std::uint64_t magnitude =
            delta < 0 ? static_cast<std::uint64_t>(-delta) : static_cast<std::uint64_t>(delta);
        return magnitude * magnitude; // exact: |delta| <= 2^32 - 1
    };
    return saturating_add(axis(a.x, b.x), axis(a.y, b.y), axis(a.z, b.z));
}

} // namespace tt::voxel_game::streaming
