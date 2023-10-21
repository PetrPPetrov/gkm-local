// Copyright 2023 Petr Petrov. All rights reserved.
// License: https://github.com/PetrPPetrov/gkm-local/blob/main/LICENSE

#pragma once

#include <cstdint>
#include <atomic>
#include <memory>
#include <deque>
#include "block.h"
#include "spin_lock.h"

constexpr BlockIndex WORLD_BLOCK_HEIGHT = 16;
constexpr BlockIndex WORLD_BLOCK_SIZE_X = 64;
constexpr BlockIndex WORLD_BLOCK_SIZE_Y = 64;
constexpr BlockIndex VIEW_DISTANCE = 8;

class WorldColumn {
    static_assert(std::atomic<bool>::is_always_lock_free);

    SpinLocked<TopLevelBlock::Ptr> blocks[WORLD_BLOCK_HEIGHT];

public:
    typedef std::shared_ptr<WorldColumn> Ptr;

    TopLevelBlock::Ptr getBlock(BlockIndex z) const {
        TopLevelBlock::Ptr result = nullptr;
        if (z >= 0 && z < WORLD_BLOCK_HEIGHT) {
            result = blocks[z].read();
        }
        return result;
    }
    void setBlock(BlockIndex z, const TopLevelBlock::Ptr& block) {
        if (z >= 0 && z < WORLD_BLOCK_HEIGHT) {
            blocks[z].write(block);
        }
    }
};

class WorldLineY {
    static_assert(std::atomic<bool>::is_always_lock_free);

    BlockIndex base_y = -WORLD_BLOCK_SIZE_Y / 2;
    mutable std::atomic<bool> columns_locked;
    std::deque<WorldColumn::Ptr> columns;

public:
    typedef std::shared_ptr<WorldLineY> Ptr;

    WorldLineY();

    WorldColumn::Ptr getColumnByAbsoluteIndex(BlockIndex y) const {
        WorldColumn::Ptr result = nullptr;
        SpinLock columns_lock(columns_locked);
        const BlockIndex index_y = y - base_y;
        if (index_y >= 0 && index_y < columns.size()) {
            result = columns[index_y];
        }
        return result;
    }
    void setColumnByAbsoluteIndex(BlockIndex y, const WorldColumn::Ptr& column) {
        SpinLock columns_lock(columns_locked);
        const BlockIndex index_y = y - base_y;
        if (index_y >= 0 && index_y < columns.size()) {
            columns[index_y] = column;
        }
    }
    WorldColumn::Ptr getColumn(BlockIndex index_y) const {
        WorldColumn::Ptr result = nullptr;
        SpinLock columns_lock(columns_locked);
        if (index_y >= 0 && index_y < columns.size()) {
            result = columns[index_y];
        }
        return result;
    }
    void setColumn(BlockIndex index_y, const WorldColumn::Ptr& column) {
        SpinLock columns_lock(columns_locked);
        if (index_y >= 0 && index_y < columns.size()) {
            columns[index_y] = column;
        }
    }
};

class World {
    static_assert(std::atomic<bool>::is_always_lock_free);

    BlockIndex base_x = -WORLD_BLOCK_SIZE_X / 2;
    mutable std::atomic<bool> lines_locked;
    std::deque<WorldLineY::Ptr> lines;

public:
    typedef std::shared_ptr<World> Ptr;

    World();

    WorldLineY::Ptr getLineByAbsoluteIndex(BlockIndex x) const {
        WorldLineY::Ptr result = nullptr;
        SpinLock lines_lock(lines_locked);
        const BlockIndex index_x = x - base_x;
        if (index_x >= 0 && index_x < lines.size()) {
            result = lines[index_x];
        }
        return result;
    }
    void setLineByAbsoluteIndex(BlockIndex x, const WorldLineY::Ptr& line) {
        SpinLock lines_lock(lines_locked);
        const BlockIndex index_x = x - base_x;
        if (index_x >= 0 && index_x < lines.size()) {
            lines[index_x] = line;
        }
    }
    WorldLineY::Ptr getLine(BlockIndex index_x) const {
        WorldLineY::Ptr result = nullptr;
        SpinLock lines_lock(lines_locked);
        if (index_x >= 0 && index_x < lines.size()) {
            result = lines[index_x];
        }
        return result;
    }
    void setLine(BlockIndex index_x, const WorldLineY::Ptr& line) {
        SpinLock lines_lock(lines_locked);
        if (index_x >= 0 && index_x < lines.size()) {
            lines[index_x] = line;
        }
    }
};

void intializeWorld();

extern World::Ptr g_world;
