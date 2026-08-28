#include "voxel_game/persistence/WorldSnapshotCodec.h"

#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

namespace
{

using tt::voxel_game::BlockCoord;
using tt::voxel_game::BlockState;
using tt::voxel_game::DecodeResult;
using tt::voxel_game::Section;
using tt::voxel_game::SectionCoord;
using tt::voxel_game::SnapshotLimits;
using tt::voxel_game::SnapshotStatus;
using tt::voxel_game::World;

constexpr std::size_t HeaderSize = 28;
constexpr std::size_t RecordSize = 12 + Section::BlockCount * 2;

void require(bool condition, const char* message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

void put_u16(std::vector<std::uint8_t>& bytes, std::size_t offset, std::uint16_t value)
{
    bytes[offset] = static_cast<std::uint8_t>(value);
    bytes[offset + 1] = static_cast<std::uint8_t>(value >> 8U);
}

void put_u32(std::vector<std::uint8_t>& bytes, std::size_t offset, std::uint32_t value)
{
    for (unsigned shift = 0; shift < 32; shift += 8)
    {
        bytes[offset + shift / 8U] = static_cast<std::uint8_t>(value >> shift);
    }
}

void put_u64(std::vector<std::uint8_t>& bytes, std::size_t offset, std::uint64_t value)
{
    for (unsigned shift = 0; shift < 64; shift += 8)
    {
        bytes[offset + shift / 8U] = static_cast<std::uint8_t>(value >> shift);
    }
}

std::uint64_t fnv1a64(const std::vector<std::uint8_t>& bytes)
{
    std::uint64_t hash = 14695981039346656037ULL;
    for (const std::uint8_t byte : bytes)
    {
        hash ^= byte;
        hash *= 1099511628211ULL;
    }
    return hash;
}

void refresh_checksum(std::vector<std::uint8_t>& bytes)
{
    const std::uint32_t checksum = tt::voxel_game::crc32_ieee(
        bytes.data() + HeaderSize, bytes.size() - HeaderSize);
    put_u32(bytes, 24, checksum);
}

std::vector<std::uint8_t> make_snapshot(std::vector<std::uint8_t> payload,
                                        std::uint32_t section_count)
{
    std::vector<std::uint8_t> bytes{
        'T', 'T', 'V', 'S', 1, 0, 16, 0, 2, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    put_u32(bytes, 12, section_count);
    put_u64(bytes, 16, payload.size());
    bytes.insert(bytes.end(), payload.begin(), payload.end());
    refresh_checksum(bytes);
    return bytes;
}

void require_failure(const std::vector<std::uint8_t>& bytes, SnapshotStatus expected,
                     SnapshotLimits limits = {})
{
    const DecodeResult result = tt::voxel_game::decode_world_snapshot(bytes, limits);
    require(result.status == expected, "unexpected rejection status");
    require(result.world.section_count() == 0, "rejected snapshot returned a partial world");
}

std::vector<std::uint8_t> one_section_snapshot(SectionCoord coordinate = {0, 0, 0})
{
    World world;
    world.set({coordinate.x * Section::Size, coordinate.y * Section::Size,
               coordinate.z * Section::Size}, BlockState::Stone);
    const auto encoded = tt::voxel_game::encode_world_snapshot(world);
    require(encoded.status == SnapshotStatus::Ok, "single section encoding failed");
    return encoded.bytes;
}

void test_empty_golden_and_crc()
{
    const World empty;
    const auto encoded = tt::voxel_game::encode_world_snapshot(empty);
    require(encoded.status == SnapshotStatus::Ok, "empty encoding failed");
    const std::vector<std::uint8_t> golden{
        'T', 'T', 'V', 'S', 1, 0, 16, 0, 2, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    require(encoded.bytes == golden, "empty snapshot golden bytes changed");
    require(fnv1a64(encoded.bytes) == 17730866564269589421ULL,
            "empty snapshot golden hash changed");
    const std::uint8_t crc_input[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
    require(tt::voxel_game::crc32_ieee(crc_input, sizeof(crc_input)) == 0xCBF43926U,
            "IEEE CRC-32 golden mismatch");
    const auto decoded = tt::voxel_game::decode_world_snapshot(encoded.bytes);
    require(decoded.status == SnapshotStatus::Ok && decoded.world.section_count() == 0,
            "empty round trip failed");
}

void test_round_trips_and_determinism()
{
    World world;
    world.set({0, 0, 0}, BlockState::Grass);
    world.set({15, 15, 15}, BlockState::Clay);
    world.set({16, 0, 0}, BlockState::Dirt);
    world.set({-1, -16, -17}, BlockState::Stone);
    const auto encoded = tt::voxel_game::encode_world_snapshot(world);
    require(encoded.status == SnapshotStatus::Ok, "complex encoding failed");
    const auto decoded = tt::voxel_game::decode_world_snapshot(encoded.bytes);
    require(decoded.status == SnapshotStatus::Ok, "complex decoding failed");
    require(decoded.world.get({0, 0, 0}) == BlockState::Grass &&
                decoded.world.get({15, 15, 15}) == BlockState::Clay &&
                decoded.world.get({16, 0, 0}) == BlockState::Dirt &&
                decoded.world.get({-1, -16, -17}) == BlockState::Stone,
            "round trip lost boundary or negative block data");

    World first;
    World second;
    const BlockCoord coordinates[] = {{-1, 0, 0}, {16, 0, 0}, {0, 0, 0}, {31, 15, 1}};
    const BlockState states[] = {BlockState::Stone, BlockState::Dirt, BlockState::Grass, BlockState::Clay};
    for (std::size_t index = 0; index < 4; ++index)
    {
        first.set(coordinates[index], states[index]);
        second.set(coordinates[3 - index], states[3 - index]);
    }
    const auto a = tt::voxel_game::encode_world_snapshot(first);
    const auto b = tt::voxel_game::encode_world_snapshot(second);
    require(a.status == SnapshotStatus::Ok && b.status == SnapshotStatus::Ok && a.bytes == b.bytes,
            "insertion order changed canonical bytes");
    for (int repeat = 0; repeat < 4; ++repeat)
    {
        require(tt::voxel_game::encode_world_snapshot(first).bytes == a.bytes,
                "repeated encoding changed canonical bytes");
    }
}

void test_air_omission_and_mesh_rebuild()
{
    World deleted;
    deleted.set({0, 0, 0}, BlockState::Stone);
    deleted.set({0, 0, 0}, BlockState::Air);
    require(deleted.section_count() == 1, "test did not retain empty loaded section");
    const auto empty = tt::voxel_game::encode_world_snapshot(deleted);
    require(empty.status == SnapshotStatus::Ok && empty.bytes.size() == HeaderSize,
            "empty loaded section was serialized");
    const auto decoded_empty = tt::voxel_game::decode_world_snapshot(empty.bytes);
    require(decoded_empty.status == SnapshotStatus::Ok && decoded_empty.world.section_count() == 0,
            "empty section was restored from snapshot");

    auto decoded = tt::voxel_game::decode_world_snapshot(one_section_snapshot());
    const SectionCoord origin{};
    require(decoded.status == SnapshotStatus::Ok && decoded.world.section_dirty(origin) &&
                decoded.world.mesh_revision(origin) == 0,
            "decoder fabricated mesh state");
    const auto rebuilt = decoded.world.rebuild_dirty_meshes();
    require(rebuilt.size() == 1 && rebuilt[0] == origin && !decoded.world.section_dirty(origin) &&
                decoded.world.mesh_revision(origin) == 1,
            "decoded world could not rebuild dirty mesh normally");
}

void test_header_and_input_rejections()
{
    const auto valid = one_section_snapshot();
    require_failure({}, SnapshotStatus::Truncated);
    require(tt::voxel_game::decode_world_snapshot(nullptr, 1).status == SnapshotStatus::InvalidArgument,
            "null nonzero input contract changed");
    require(tt::voxel_game::decode_world_snapshot(nullptr, 0).status == SnapshotStatus::Truncated,
            "null zero input contract changed");
    for (std::size_t size = 0; size < valid.size(); ++size)
    {
        require_failure(std::vector<std::uint8_t>(valid.begin(), valid.begin() + size),
                        size < HeaderSize ? SnapshotStatus::Truncated : SnapshotStatus::Truncated);
    }
    auto altered = valid;
    altered[0] = 'X';
    require_failure(altered, SnapshotStatus::BadMagic);
    altered = valid;
    put_u16(altered, 4, 2);
    require_failure(altered, SnapshotStatus::UnsupportedVersion);
    for (const std::size_t field : {std::size_t{6}, std::size_t{8}, std::size_t{10}})
    {
        altered = valid;
        altered[field] ^= 1U;
        require_failure(altered, SnapshotStatus::InvalidSectionMetadata);
    }
    altered = valid;
    altered.push_back(0);
    require_failure(altered, SnapshotStatus::TrailingData);
    altered = valid;
    altered[HeaderSize + 12] ^= 1U;
    require_failure(altered, SnapshotStatus::ChecksumMismatch);
    altered = valid;
    put_u64(altered, 16, 0);
    require_failure(altered, SnapshotStatus::InvalidSectionMetadata);
    altered = valid;
    put_u64(altered, 16, (std::numeric_limits<std::uint64_t>::max)());
    require_failure(altered, SnapshotStatus::InvalidSectionMetadata);
}

void test_payload_rejections_and_limits()
{
    const auto valid = one_section_snapshot();
    auto altered = valid;
    put_u16(altered, HeaderSize + 12, 999);
    refresh_checksum(altered);
    require_failure(altered, SnapshotStatus::InvalidBlockState);

    std::vector<std::uint8_t> duplicated(valid.begin() + HeaderSize, valid.end());
    duplicated.insert(duplicated.end(), valid.begin() + HeaderSize, valid.end());
    require_failure(make_snapshot(std::move(duplicated), 2), SnapshotStatus::DuplicateSection);

    altered = valid;
    put_u32(altered, HeaderSize, static_cast<std::uint32_t>((std::numeric_limits<int>::max)()));
    refresh_checksum(altered);
    require_failure(altered, SnapshotStatus::CoordinateOverflow);

    World world;
    world.set({0, 0, 0}, BlockState::Stone);
    require(tt::voxel_game::encode_world_snapshot(world, {0, 64U * 1024U * 1024U}).status ==
                SnapshotStatus::SizeLimit,
            "encode section count limit not enforced");
    require(tt::voxel_game::encode_world_snapshot(World{}, {1, HeaderSize - 1}).status ==
                SnapshotStatus::SizeLimit,
            "encode byte limit not enforced");
    require_failure(valid, SnapshotStatus::SizeLimit, {1, HeaderSize});
    altered = valid;
    put_u32(altered, 12, 2);
    put_u64(altered, 16, static_cast<std::uint64_t>(RecordSize * 2));
    require_failure(altered, SnapshotStatus::SizeLimit, {1, 64U * 1024U * 1024U});
}

} // namespace

int main()
{
    try
    {
        test_empty_golden_and_crc();
        test_round_trips_and_determinism();
        test_air_omission_and_mesh_rebuild();
        test_header_and_input_rejections();
        test_payload_rejections_and_limits();
        std::cout << "world_snapshot_codec_tests: PASS" << std::endl;
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "world_snapshot_codec_tests: FAIL: " << error.what() << std::endl;
        return 1;
    }
}
