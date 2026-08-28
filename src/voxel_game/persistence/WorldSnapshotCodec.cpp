#include "voxel_game/persistence/WorldSnapshotCodec.h"

#include "voxel_game/voxel/BlockRegistry.h"

#include <array>
#include <limits>

namespace tt::voxel_game
{
namespace
{

constexpr std::size_t HeaderSize = 28;
constexpr std::size_t RecordHeaderSize = 12;
constexpr std::size_t BlockBytes = sizeof(std::uint16_t);
constexpr std::size_t RecordSize = RecordHeaderSize + Section::BlockCount * BlockBytes;
constexpr std::uint16_t FormatVersion = 1;

bool checked_add(std::size_t left, std::size_t right, std::size_t* result)
{
    if (left > (std::numeric_limits<std::size_t>::max)() - right)
    {
        return false;
    }
    *result = left + right;
    return true;
}

bool checked_multiply(std::size_t left, std::size_t right, std::size_t* result)
{
    if (left != 0 && right > (std::numeric_limits<std::size_t>::max)() / left)
    {
        return false;
    }
    *result = left * right;
    return true;
}

void append_u16(std::vector<std::uint8_t>& bytes, std::uint16_t value)
{
    bytes.push_back(static_cast<std::uint8_t>(value));
    bytes.push_back(static_cast<std::uint8_t>(value >> 8U));
}

void append_u32(std::vector<std::uint8_t>& bytes, std::uint32_t value)
{
    for (unsigned shift = 0; shift < 32; shift += 8)
    {
        bytes.push_back(static_cast<std::uint8_t>(value >> shift));
    }
}

void append_u64(std::vector<std::uint8_t>& bytes, std::uint64_t value)
{
    for (unsigned shift = 0; shift < 64; shift += 8)
    {
        bytes.push_back(static_cast<std::uint8_t>(value >> shift));
    }
}

std::uint16_t read_u16(const std::uint8_t* data)
{
    return static_cast<std::uint16_t>(data[0]) |
           (static_cast<std::uint16_t>(data[1]) << 8U);
}

std::uint32_t read_u32(const std::uint8_t* data)
{
    std::uint32_t value = 0;
    for (unsigned shift = 0; shift < 32; shift += 8)
    {
        value |= static_cast<std::uint32_t>(data[shift / 8U]) << shift;
    }
    return value;
}

std::uint64_t read_u64(const std::uint8_t* data)
{
    std::uint64_t value = 0;
    for (unsigned shift = 0; shift < 64; shift += 8)
    {
        value |= static_cast<std::uint64_t>(data[shift / 8U]) << shift;
    }
    return value;
}

int read_i32(const std::uint8_t* data)
{
    const std::uint32_t raw = read_u32(data);
    if (raw <= static_cast<std::uint32_t>((std::numeric_limits<int>::max)()))
    {
        return static_cast<int>(raw);
    }
    return static_cast<int>(static_cast<std::int64_t>(raw) - (INT64_C(1) << 32));
}

bool coordinate_can_address_section(SectionCoord coordinate)
{
    const auto valid_axis = [](int value)
    {
        const std::int64_t origin = static_cast<std::int64_t>(value) * Section::Size;
        return origin >= (std::numeric_limits<int>::min)() &&
               origin + Section::Size - 1 <= (std::numeric_limits<int>::max)();
    };
    return valid_axis(coordinate.x) && valid_axis(coordinate.y) && valid_axis(coordinate.z);
}

struct DecodedSection
{
    SectionCoord coordinate;
    std::array<BlockState, Section::BlockCount> blocks{};
};

DecodeResult failure(SnapshotStatus status)
{
    return {status, {}};
}

} // namespace

std::uint32_t crc32_ieee(const std::uint8_t* data, std::size_t size) noexcept
{
    if (data == nullptr && size != 0)
    {
        return 0;
    }
    std::uint32_t crc = 0xFFFFFFFFU;
    for (std::size_t index = 0; index < size; ++index)
    {
        crc ^= data[index];
        for (unsigned bit = 0; bit < 8; ++bit)
        {
            crc = (crc >> 1U) ^ ((crc & 1U) != 0 ? 0xEDB88320U : 0U);
        }
    }
    return crc ^ 0xFFFFFFFFU;
}

EncodeResult encode_world_snapshot(const World& world, SnapshotLimits limits)
{
    std::vector<SectionCoord> coordinates;
    for (const SectionCoord coordinate : world.section_coordinates())
    {
        const Section* section = world.find_section(coordinate);
        bool non_empty = false;
        for (std::size_t index = 0; index < Section::BlockCount; ++index)
        {
            const int x = static_cast<int>(index % Section::Size);
            const int z = static_cast<int>((index / Section::Size) % Section::Size);
            const int y = static_cast<int>(index / (Section::Size * Section::Size));
            if (section->get({x, y, z}) != BlockState::Air)
            {
                non_empty = true;
                break;
            }
        }
        if (non_empty)
        {
            if (!coordinate_can_address_section(coordinate))
            {
                return {SnapshotStatus::CoordinateOverflow, {}};
            }
            coordinates.push_back(coordinate);
        }
    }

    if (coordinates.size() > limits.max_sections)
    {
        return {SnapshotStatus::SizeLimit, {}};
    }
    std::size_t payload_size = 0;
    std::size_t total_size = 0;
    if (!checked_multiply(coordinates.size(), RecordSize, &payload_size) ||
        !checked_add(HeaderSize, payload_size, &total_size) ||
        total_size > limits.max_bytes ||
        payload_size > (std::numeric_limits<std::uint64_t>::max)())
    {
        return {SnapshotStatus::SizeLimit, {}};
    }

    std::vector<std::uint8_t> bytes;
    bytes.reserve(total_size);
    bytes.insert(bytes.end(), {'T', 'T', 'V', 'S'});
    append_u16(bytes, FormatVersion);
    append_u16(bytes, static_cast<std::uint16_t>(Section::Size));
    append_u16(bytes, static_cast<std::uint16_t>(sizeof(BlockState)));
    append_u16(bytes, 0);
    append_u32(bytes, static_cast<std::uint32_t>(coordinates.size()));
    append_u64(bytes, static_cast<std::uint64_t>(payload_size));
    append_u32(bytes, 0);

    for (const SectionCoord coordinate : coordinates)
    {
        const Section* section = world.find_section(coordinate);
        append_u32(bytes, static_cast<std::uint32_t>(coordinate.x));
        append_u32(bytes, static_cast<std::uint32_t>(coordinate.y));
        append_u32(bytes, static_cast<std::uint32_t>(coordinate.z));
        for (std::size_t index = 0; index < Section::BlockCount; ++index)
        {
            const int x = static_cast<int>(index % Section::Size);
            const int z = static_cast<int>((index / Section::Size) % Section::Size);
            const int y = static_cast<int>(index / (Section::Size * Section::Size));
            append_u16(bytes, static_cast<std::uint16_t>(section->get({x, y, z})));
        }
    }
    const std::uint32_t checksum = crc32_ieee(bytes.data() + HeaderSize, payload_size);
    bytes[24] = static_cast<std::uint8_t>(checksum);
    bytes[25] = static_cast<std::uint8_t>(checksum >> 8U);
    bytes[26] = static_cast<std::uint8_t>(checksum >> 16U);
    bytes[27] = static_cast<std::uint8_t>(checksum >> 24U);
    return {SnapshotStatus::Ok, std::move(bytes)};
}

DecodeResult decode_world_snapshot(const std::uint8_t* data, std::size_t size,
                                   SnapshotLimits limits)
{
    if (data == nullptr && size != 0)
    {
        return failure(SnapshotStatus::InvalidArgument);
    }
    if (size > limits.max_bytes)
    {
        return failure(SnapshotStatus::SizeLimit);
    }
    if (size < HeaderSize)
    {
        return failure(SnapshotStatus::Truncated);
    }
    if (data[0] != 'T' || data[1] != 'T' || data[2] != 'V' || data[3] != 'S')
    {
        return failure(SnapshotStatus::BadMagic);
    }
    if (read_u16(data + 4) != FormatVersion)
    {
        return failure(SnapshotStatus::UnsupportedVersion);
    }
    if (read_u16(data + 6) != Section::Size ||
        read_u16(data + 8) != sizeof(BlockState) || read_u16(data + 10) != 0)
    {
        return failure(SnapshotStatus::InvalidSectionMetadata);
    }

    const std::uint32_t count = read_u32(data + 12);
    const std::uint64_t payload_u64 = read_u64(data + 16);
    const std::uint32_t expected_checksum = read_u32(data + 24);
    if (count > limits.max_sections ||
        payload_u64 > (std::numeric_limits<std::size_t>::max)())
    {
        return failure(SnapshotStatus::SizeLimit);
    }
    const std::size_t payload_size = static_cast<std::size_t>(payload_u64);
    std::size_t expected_payload = 0;
    std::size_t total_size = 0;
    if (!checked_multiply(count, RecordSize, &expected_payload) ||
        !checked_add(HeaderSize, payload_size, &total_size) ||
        payload_size != expected_payload || total_size > limits.max_bytes)
    {
        return failure(SnapshotStatus::InvalidSectionMetadata);
    }
    if (size < total_size)
    {
        return failure(SnapshotStatus::Truncated);
    }
    if (size > total_size)
    {
        return failure(SnapshotStatus::TrailingData);
    }
    if (crc32_ieee(data + HeaderSize, payload_size) != expected_checksum)
    {
        return failure(SnapshotStatus::ChecksumMismatch);
    }

    std::vector<DecodedSection> decoded;
    decoded.reserve(count);
    const std::uint8_t* cursor = data + HeaderSize;
    for (std::uint32_t section_index = 0; section_index < count; ++section_index)
    {
        const SectionCoord coordinate{read_i32(cursor), read_i32(cursor + 4), read_i32(cursor + 8)};
        if (!coordinate_can_address_section(coordinate))
        {
            return failure(SnapshotStatus::CoordinateOverflow);
        }
        if (!decoded.empty())
        {
            if (coordinate == decoded.back().coordinate)
            {
                return failure(SnapshotStatus::DuplicateSection);
            }
            if (!(decoded.back().coordinate < coordinate))
            {
                return failure(SnapshotStatus::InvalidSectionMetadata);
            }
        }
        DecodedSection item;
        item.coordinate = coordinate;
        bool non_empty = false;
        cursor += RecordHeaderSize;
        for (std::size_t block_index = 0; block_index < Section::BlockCount; ++block_index)
        {
            const BlockState state = static_cast<BlockState>(read_u16(cursor));
            if (!is_valid_block_state(state))
            {
                return failure(SnapshotStatus::InvalidBlockState);
            }
            item.blocks[block_index] = state;
            non_empty = non_empty || state != BlockState::Air;
            cursor += BlockBytes;
        }
        if (!non_empty)
        {
            return failure(SnapshotStatus::InvalidSectionMetadata);
        }
        decoded.push_back(std::move(item));
    }

    World world;
    for (const DecodedSection& item : decoded)
    {
        const std::int64_t origin_x = static_cast<std::int64_t>(item.coordinate.x) * Section::Size;
        const std::int64_t origin_y = static_cast<std::int64_t>(item.coordinate.y) * Section::Size;
        const std::int64_t origin_z = static_cast<std::int64_t>(item.coordinate.z) * Section::Size;
        for (std::size_t index = 0; index < Section::BlockCount; ++index)
        {
            const BlockState state = item.blocks[index];
            if (state == BlockState::Air)
            {
                continue;
            }
            const int x = static_cast<int>(index % Section::Size);
            const int z = static_cast<int>((index / Section::Size) % Section::Size);
            const int y = static_cast<int>(index / (Section::Size * Section::Size));
            world.set({static_cast<int>(origin_x + x), static_cast<int>(origin_y + y),
                       static_cast<int>(origin_z + z)}, state);
        }
    }
    return {SnapshotStatus::Ok, std::move(world)};
}

} // namespace tt::voxel_game
