#pragma once

#include "voxel_game/voxel/World.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace tt::voxel_game
{

// A World snapshot is a CPU-only buffer, not a file or region format.  All
// multi-byte fields are little-endian.  Its exact layout is:
//   0  magic[4] = "TTVS"
//   4  u16 format version (1)
//   6  u16 Section::Size (16)
//   8  u16 sizeof(BlockState) (2)
//  10  u16 reserved (zero)
//  12  u32 non-empty section count
//  16  u64 payload byte count
//  24  u32 IEEE CRC-32 of exactly the payload bytes (offset 28 onward)
// Payload records are sorted SectionCoord x/y/z and are [i32 x, i32 y,
// i32 z, 4096 * u16 BlockState], with the Section::index_of x-fastest order.
// Compression, if added, belongs outside this versioned canonical format.
enum class SnapshotStatus
{
    Ok,
    InvalidArgument,
    SizeLimit,
    BadMagic,
    UnsupportedVersion,
    Truncated,
    TrailingData,
    ChecksumMismatch,
    InvalidSectionMetadata,
    DuplicateSection,
    InvalidBlockState,
    CoordinateOverflow,
};

struct SnapshotLimits
{
    std::uint32_t max_sections = 4096;
    std::size_t max_bytes = 64U * 1024U * 1024U;
};

struct EncodeResult
{
    SnapshotStatus status = SnapshotStatus::InvalidArgument;
    std::vector<std::uint8_t> bytes;
};

struct DecodeResult
{
    SnapshotStatus status = SnapshotStatus::InvalidArgument;
    World world;
};

// Standard IEEE CRC-32 (poly 0xEDB88320, initial 0xFFFFFFFF, final xor
// 0xFFFFFFFF). A null pointer with zero length is the empty byte sequence.
std::uint32_t crc32_ieee(const std::uint8_t* data, std::size_t size) noexcept;

EncodeResult encode_world_snapshot(const World& world,
                                   SnapshotLimits limits = {});

// Null + nonzero is InvalidArgument. Null + zero (and a non-null zero-sized
// buffer) is Truncated, because a snapshot header is required.
DecodeResult decode_world_snapshot(const std::uint8_t* data,
                                   std::size_t size,
                                   SnapshotLimits limits = {});

inline DecodeResult decode_world_snapshot(const std::vector<std::uint8_t>& bytes,
                                          SnapshotLimits limits = {})
{
    return decode_world_snapshot(bytes.data(), bytes.size(), limits);
}

} // namespace tt::voxel_game
