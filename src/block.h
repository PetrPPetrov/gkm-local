// Copyright 2023 Petr Petrov. All rights reserved.
// License: https://github.com/PetrPPetrov/gkm-local/blob/main/LICENSE

#pragma once

#include <cstdint>
#include <atomic>
#include <memory>
#include "gkm_local.h"
#include "draw_info.h"
#include "fnv_hash.h"
#include "game_logic.h"
#include "spin_lock.h"
#include "constants.sh"

static_assert(std::atomic<bool>::is_always_lock_free);

// Base class for representing blocks in this game.
struct BlockBase {
    // Indicates that the entire block is filled by one material (or entire empty).
    std::atomic<bool> entire = true;
    // Specifies the material for this block. Usefull only if entire is true.
    std::atomic<BlockMaterial> material = 0;

    // Indicates that tessellation request was sent for this block.
    std::atomic<bool> tessellation_request = false;
    // Contains pre-tessellated vertex buffers for rendering by BGFX.
    SpinLocked<BlockDrawInfo::Ptr> draw_info;
    // Collects information for instance drawing. It is used only by single rendering thread.
    // So, no multi-threaded synchronization is required.
    std::list<DrawInstanceInfo> draw_instance_info;

    BlockBase(BlockMaterial material_ = 0) {
        material = material_;
    }

    BlockBase(const BlockBase& other) {
        entire = other.entire.load();
        if (entire) {
            material = other.material.load();
        }
    }
};

constexpr BlockIndex NESTED_BLOCKS = 8;

// Level 0 = 8 cm
// Level 1 = 64 cm
// Level 2 = 5 m 12 cm
// Level 3 = 40 m 96 cm

template <std::uint8_t Level>
struct Block;

template <>
struct Block<0> : public BlockBase {
    typedef std::shared_ptr<Block<0>> Ptr;
    typedef std::weak_ptr<Block<0>> WeakPtr;
    constexpr static BlockIndex MATERIAL_COUNT = NESTED_BLOCKS * NESTED_BLOCKS * NESTED_BLOCKS;
    constexpr static BlockIndex SIZE = NESTED_BLOCKS;

    std::atomic<BlockMaterial> materials[MATERIAL_COUNT] = { 0 };

    Block(BlockMaterial material_ = 0) : BlockBase(material_) {
    }

    Block(const Block<0>& other) : BlockBase(other) {
        if (!entire) {
            for (BlockIndex i = 0; i < MATERIAL_COUNT; ++i) {
                materials[i] = other.materials[i].load();
            }
        }
    }

    BlockMaterial getMaterial(BlockIndex x, BlockIndex y, BlockIndex z) const {
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
            BlockMaterial material_value = material.load();
            hash.update(material_value);
        } else {
            for (BlockIndex i = 0; i < MATERIAL_COUNT; ++i) {
                hash.update(materials[i].load());
            }
        }
        return hash.getHash();
    }
};

template <std::uint8_t Level>
struct Block : public BlockBase
{
    typedef std::shared_ptr<Block<Level>> Ptr;
    typedef std::weak_ptr<Block<Level>> WeakPtr;
    constexpr static BlockIndex CHILDREN_COUNT = NESTED_BLOCKS * NESTED_BLOCKS * NESTED_BLOCKS;
    constexpr static BlockIndex SIZE = NESTED_BLOCKS * Block<Level - 1>::SIZE;

    SpinLocked<typename Block<Level-1>::Ptr> children[CHILDREN_COUNT] = { nullptr };

    Block(BlockMaterial material_ = 0) : BlockBase(material_) {
    }

    Block(const Block<Level>& other) : BlockBase(other) {
        if (!entire) {
            for (BlockIndex i = 0; i < Block<Level>::CHILDREN_COUNT; ++i) {
                children[i].write(other.children[i].read());
            }
        }
    }

    BlockMaterial getMaterial(BlockIndex x, BlockIndex y, BlockIndex z) const {
        if (entire) {
            return material;
        }
        BlockIndex sub_block_x = x / Block<Level - 1>::SIZE;
        BlockIndex sub_block_y = y / Block<Level - 1>::SIZE;
        BlockIndex sub_block_z = z / Block<Level - 1>::SIZE;
        auto child = children[sub_block_z * NESTED_BLOCKS * NESTED_BLOCKS + sub_block_y * NESTED_BLOCKS + sub_block_x].read();
        assert(child.get());
        BlockIndex inside_sub_block_x = x % Block<Level - 1>::SIZE;
        BlockIndex inside_sub_block_y = y % Block<Level - 1>::SIZE;
        BlockIndex inside_sub_block_z = z % Block<Level - 1>::SIZE;
        return child->getMaterial(inside_sub_block_x, inside_sub_block_y, inside_sub_block_z);
    }

    FnvHash::Hash calculateHash() const {
        FnvHash hash;
        bool entire_value = entire.load();
        hash.update(entire_value);
        if (entire_value) {
            BlockMaterial material_value = material.load();
            hash.update(material_value);
        } else {
            for (BlockIndex i = 0; i < CHILDREN_COUNT; ++i) {
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

template <std::uint8_t Level>
inline BlockIndex coordToBlockIndex(BlockIndex value, BlockIndex& local) {
    BlockIndex result;
    if (value < 0) {
        result = (value + 1) / Block<Level>::SIZE - 1;
    }
    else {
        result = value / Block<Level>::SIZE;
    }
    local = value - result * Block<Level>::SIZE;
    return result;
}
