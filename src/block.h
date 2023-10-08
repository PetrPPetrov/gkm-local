// Copyright 2023 Petr Petrov. All rights reserved.
// License: https://github.com/PetrPPetrov/gkm-local/blob/main/LICENSE

#pragma once

#include <cstdint>
#include <atomic>
#include <memory>
#include "draw_info.h"
#include "fnv_hash.h"
#include "game_logic.h"
#include "spin_lock.h"
#include "constants.sh"

typedef GlobalCoordinateType BlockIndexType;

constexpr std::int32_t NESTED_BLOCKS = 8;

// Level 0 = 8 cm
// Level 1 = 64 cm
// Level 2 = 5 m 12 cm
// Level 3 = 40 m 96 cm

template <std::uint8_t Level>
struct Block;

template <>
struct Block<0> {
    static_assert(std::atomic<bool>::is_always_lock_free);
    static_assert(std::atomic<BlockMaterialType>::is_always_lock_free);

    typedef std::shared_ptr<Block<0>> Ptr;
    typedef std::weak_ptr<Block<0>> WeakPtr;
    constexpr static std::int32_t MATERIAL_COUNT = NESTED_BLOCKS * NESTED_BLOCKS * NESTED_BLOCKS;
    constexpr static GlobalCoordinateType SIZE = NESTED_BLOCKS;

    std::atomic<bool> entire = true;
    std::atomic<BlockMaterialType> material = 0;
    std::atomic<BlockMaterialType> materials[MATERIAL_COUNT] = { 0 };
    SpinLocked<BlockDrawInfo::Ptr> draw_info;
    std::list<DrawInstanceInfo> draw_instance_info;

    Block(BlockMaterialType material_ = 0) {
        material = material_;
    }

    Block(const Block<0>& other) {
        entire = other.entire.load();
        if (entire) {
            material = other.material.load();
        } else {
            for (std::int32_t i = 0; i < MATERIAL_COUNT; ++i) {
                materials[i] = other.materials[i].load();
            }
        }
    }

    BlockMaterialType getMaterial(GlobalCoordinateType x, GlobalCoordinateType y, GlobalCoordinateType z) const {
        if (entire) {
            return material;
        } else {
            return materials[z * NESTED_BLOCKS * NESTED_BLOCKS + y * NESTED_BLOCKS + x];
        }
    }

    FnvHash::Hash calculateHash() const {
        FnvHash hash;
        bool entire_value = entire.load();
        hash.update(entire_value);
        if (entire_value) {
            BlockMaterialType material_value = material.load();
            hash.update(material_value);
        } else {
            for (unsigned i = 0; i < MATERIAL_COUNT; ++i) {
                hash.update(materials[i].load());
            }
        }
        return hash.getHash();
    }
};

template <std::uint8_t Level>
struct Block
{
    static_assert(std::atomic<bool>::is_always_lock_free);
    static_assert(std::atomic<BlockMaterialType>::is_always_lock_free);

    typedef std::shared_ptr<Block<Level>> Ptr;
    typedef std::weak_ptr<Block<Level>> WeakPtr;
    constexpr static std::int32_t CHILDREN_COUNT = NESTED_BLOCKS * NESTED_BLOCKS * NESTED_BLOCKS;
    constexpr static GlobalCoordinateType SIZE = NESTED_BLOCKS * Block<Level - 1>::SIZE;

    std::atomic<bool> entire = true;
    std::atomic<BlockMaterialType> material = 0;
    SpinLocked<typename Block<Level-1>::Ptr> children[CHILDREN_COUNT] = { nullptr };
    SpinLocked<BlockDrawInfo::Ptr> draw_info;
    std::list<DrawInstanceInfo> draw_instance_info;

    Block(BlockMaterialType material_ = 0) {
        material = material_;
    }

    Block(const Block<Level>& other) {
        entire = other.entire.load();
        if (entire) {
            material = other.material.load();
        } else {
            for (std::int32_t i = 0; i < Block<Level>::CHILDREN_COUNT; ++i) {
                children[i].write(other.children[i].read());
            }
        }
    }

    BlockMaterialType getMaterial(GlobalCoordinateType x, GlobalCoordinateType y, GlobalCoordinateType z) const {
        if (entire) {
            return material;
        }
        BlockIndexType sub_block_x = x / Block<Level - 1>::SIZE;
        BlockIndexType sub_block_y = y / Block<Level - 1>::SIZE;
        BlockIndexType sub_block_z = z / Block<Level - 1>::SIZE;
        auto child = children[sub_block_z * NESTED_BLOCKS * NESTED_BLOCKS + sub_block_y * NESTED_BLOCKS + sub_block_x].read();
        assert(child.get());
        BlockIndexType inside_sub_block_x = x % Block<Level - 1>::SIZE;
        BlockIndexType inside_sub_block_y = y % Block<Level - 1>::SIZE;
        BlockIndexType inside_sub_block_z = z % Block<Level - 1>::SIZE;
        return child->getMaterial(inside_sub_block_x, inside_sub_block_y, inside_sub_block_z);
    }

    FnvHash::Hash calculateHash() const {
        FnvHash hash;
        bool entire_value = entire.load();
        hash.update(entire_value);
        if (entire_value) {
            BlockMaterialType material_value = material.load();
            hash.update(material_value);
        } else {
            for (unsigned i = 0; i < CHILDREN_COUNT; ++i) {
                hash.update(children[i].read().get());
            }
        }
        return hash.getHash();
    }
};

constexpr std::uint8_t STANDARD_LEVEL = 1;
typedef Block<STANDARD_LEVEL> StandardBlock;
constexpr std::int16_t TEX_COORD_RATIO = TEXTURE_SIZE / StandardBlock::SIZE;

constexpr std::uint8_t TOP_LEVEL = 3;
typedef Block<TOP_LEVEL> TopLevelBlock;

template <template<std::uint8_t> typename Operator, std::uint8_t Level>
struct ExecutorForAllLevels;

template <template<std::uint8_t> typename Operator>
struct ExecutorForAllLevels<Operator, 0> {
    static void execute() {
        Operator<0>::execute();
    }
};

template <template<std::uint8_t> typename Operator, std::uint8_t Level>
struct ExecutorForAllLevels {
    static void execute() {
        Operator<Level>::execute();
        ExecutorForAllLevels<Operator, Level - 1>::execute();
    }
};

template <template<std::uint8_t> typename Operator>
void executeForAllLevels() {
    ExecutorForAllLevels<Operator, TOP_LEVEL>::execute();
}

inline BlockIndexType globalCoordinateToTopLevelBlockIndex(GlobalCoordinateType value, GlobalCoordinateType& local) {
    BlockIndexType result;
    if (value < 0) {
        result = value / TopLevelBlock::SIZE - 1;
    } else {
        result = value / TopLevelBlock::SIZE;
    }
    local = value - result * TopLevelBlock::SIZE;
    return result;
}
