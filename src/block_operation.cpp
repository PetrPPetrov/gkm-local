// Copyright 2023 Petr Petrov. All rights reserved.
// License: https://github.com/PetrPPetrov/gkm-local/blob/main/LICENSE

#include <cstdint>
#include <unordered_map>
#include <thread>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include "main.h"
#include "fnv_hash.h"
#include "block.h"
#include "world.h"
#include "mutex_free_compound.h"
#include "mutex_free_fifo.h"
#include "tessellation.h"
#include "block_operation.h"

const static boost::posix_time::milliseconds BLOCK_OPERATION_WAKE_UP_TIME_INTERVAL(250);

static std::unique_ptr<std::thread> g_block_operation_thread = nullptr;
static MutexFreeFifo<BlockOperation> g_block_operation_queue;
static std::unique_ptr<boost::asio::io_service> g_io_service;
static std::unique_ptr<boost::asio::deadline_timer> g_timer;

template <std::uint8_t Level>
using BlockCache = std::unordered_map<FnvHash::Hash, typename Block<Level>::WeakPtr>;

template <std::uint8_t Level>
BlockCache<Level>& getCache() {
    static BlockCache<Level> block_cache;
    return block_cache;
}

template <std::uint8_t Level>
typename BlockCache<Level>::iterator& getCacheCleaningIterator() {
    static BlockCache<Level>::iterator cache_cleaning_iterator;
    return cache_cleaning_iterator;
}

template <std::uint8_t Level>
struct CacheCleaningIteratorInitializer {
    static void execute() {
        getCacheCleaningIterator<Level>() = getCache<Level>().begin();
    }
};

static void initializeCacheCleaningIterators() {
    executeForAllLevels<CacheCleaningIteratorInitializer>();
}

template <std::uint8_t Level>
void cleanUpCache() {
    if (getCacheCleaningIterator<Level>() != getCache<Level>().end()) {
        if (!getCacheCleaningIterator<Level>()->second.lock()) {
            getCache<Level>().erase(getCacheCleaningIterator<Level>());
            getCacheCleaningIterator<Level>() = getCache<Level>().begin();
        } else {
            ++getCacheCleaningIterator<Level>();
        }
    } else {
        getCacheCleaningIterator<Level>() = getCache<Level>().begin();
    }
}

template <std::uint8_t Level>
struct CleanUpCache {
    static void execute() {
        cleanUpCache<Level>();
    }
};

static void cleanUpCaches() {
    executeForAllLevels<CleanUpCache>();
}

template <std::uint8_t Level>
static inline typename Block<Level>::Ptr getCached(const typename Block<Level>::Ptr& block) {
    auto& cache = getCache<Level>();
    FnvHash::Hash hash = block->calculateHash();
    auto fit = cache.find(hash);
    if (fit != cache.end()) {
        auto existing = fit->second.lock();
        if (existing) {
            return existing;
        } else {
            fit->second = block;
            TessellationRequest<Level> request;
            request.block = block;
            postBlockTessellationRequest(request);
            return block;
        }
    } else {
        cache.emplace(hash, block);
        TessellationRequest<Level> request;
        request.block = block;
        postBlockTessellationRequest(request);
        return block;
    }
}

template <std::uint8_t Level>
struct BlockOperationProcessor;

template <>
struct BlockOperationProcessor<0> {
    template <class Operation>
    static Block<0>::Ptr process(
        const Block<0>::Ptr& block,
        bool use_level, std::uint8_t level,
        BlockIndexType x, BlockIndexType y, BlockIndexType z,
        Operation&& operation) {
        Block<0>::Ptr copy_block = nullptr;
        if (use_level && level == 0) {
            copy_block = std::make_shared<Block<0>>();
            copy_block->entire = true;
            operation(copy_block->material);
        } else {
            copy_block = std::make_shared<Block<0>>(*block);
            if (copy_block->entire) {
                copy_block->entire = false;
                BlockMaterialType overall_material = copy_block->material.load();
                for (std::int32_t i = 0; i < Block<0>::MATERIAL_COUNT; ++i) {
                    copy_block->materials[i] = overall_material;
                }
            }
            operation(copy_block->materials[z * NESTED_BLOCKS * NESTED_BLOCKS + y * NESTED_BLOCKS + x]);
            bool all_the_same = true;
            BlockMaterialType first_material = copy_block->materials[0];
            for (std::int32_t i = 1; i < Block<0>::MATERIAL_COUNT; ++i) {
                if (copy_block->materials[i] != first_material) {
                    all_the_same = false;
                    break;
                }
            }
            if (all_the_same) {
                copy_block->entire = true;
                copy_block->material = first_material;
            }
        }
        auto result_block = getCached<0>(copy_block);
        return result_block;
    }
};

template <std::uint8_t Level>
struct BlockOperationProcessor {
    template <class Operation>
    static typename Block<Level>::Ptr process(
        const typename Block<Level>::Ptr& block,
        bool use_level, std::uint8_t level,
        BlockIndexType x, BlockIndexType y, BlockIndexType z,
        Operation&& operation) {
        Block<Level>::Ptr copy_block = nullptr;
        if (use_level && level == Level) {
            copy_block = std::make_shared<Block<Level>>();
            copy_block->entire = true;
            operation(copy_block->material);
        } else {
            copy_block = std::make_shared<Block<Level>>(*block);
            if (copy_block->entire) {
                copy_block->entire = false;
                auto sub_level_block = std::make_shared<Block<Level - 1>>(copy_block->material);
                auto cached_sub_level_block = getCached<Level - 1>(sub_level_block);
                for (std::int32_t i = 0; i < Block<Level>::CHILDREN_COUNT; ++i) {
                    copy_block->children[i].write(cached_sub_level_block);
                }
            }
            GlobalCoordinateType sub_block_x = x / Block<Level - 1>::SIZE;
            GlobalCoordinateType sub_block_y = y / Block<Level - 1>::SIZE;
            GlobalCoordinateType sub_block_z = z / Block<Level - 1>::SIZE;
            GlobalCoordinateType sub_block_local_x = x % Block<Level - 1>::SIZE;
            GlobalCoordinateType sub_block_local_y = y % Block<Level - 1>::SIZE;
            GlobalCoordinateType sub_block_local_z = z % Block<Level - 1>::SIZE;
            std::int32_t child_index = sub_block_z * NESTED_BLOCKS * NESTED_BLOCKS + sub_block_y * NESTED_BLOCKS + sub_block_x;
            auto existing_child = copy_block->children[child_index].read();
            copy_block->children[child_index].write(BlockOperationProcessor<Level - 1>::process(existing_child, use_level, level, sub_block_local_x, sub_block_local_y, sub_block_local_z, operation));
            bool all_sub_blocks_same = true;
            auto first_sub_block = copy_block->children[0].read();
            for (std::int32_t i = 1; i < Block<Level>::CHILDREN_COUNT; ++i) {
                if (copy_block->children[i].read().get() != first_sub_block.get()) {
                    all_sub_blocks_same = false;
                    break;
                }
            }
            if (all_sub_blocks_same && first_sub_block->entire) {
                copy_block->entire = true;
                copy_block->material = first_sub_block->material.load();
                for (std::int32_t i = 0; i < Block<Level>::CHILDREN_COUNT; ++i) {
                    copy_block->children[i].write(nullptr);
                }
            }
        }
        auto result_block = getCached<Level>(copy_block);
        return result_block;
    }
};

static inline void putBlock(
    const WorldColumn::Ptr& column, BlockIndexType block_z_index,
    bool use_level, std::uint8_t level,
    BlockIndexType x, BlockIndexType y, BlockIndexType z,
    BlockMaterialType new_material) {
    auto top_block = column->getBlock(block_z_index);
    BlockMaterialType existing_material = top_block->getMaterial(x, y, z);
    if (existing_material == 0) {
        auto result_top_block = BlockOperationProcessor<TOP_LEVEL>::process(top_block, use_level, level, x, y, z, [new_material](std::atomic<BlockMaterialType>& material) {
            material = new_material;
        });
        column->setBlock(block_z_index, result_top_block);
    }
}

static inline void removeBlock(
    const WorldColumn::Ptr& column, BlockIndexType block_z_index,
    bool use_level, std::uint8_t level,
    BlockIndexType x, BlockIndexType y, BlockIndexType z) {
    auto top_block = column->getBlock(block_z_index);
    BlockMaterialType existing_material = top_block->getMaterial(x, y, z);
    if (existing_material != 0) {
        auto result_top_block = BlockOperationProcessor<TOP_LEVEL>::process(top_block, use_level, level, x, y, z, [](std::atomic<BlockMaterialType>& material) {
            material = 0;
        });
        column->setBlock(block_z_index, result_top_block);
    }
}

static inline void processBlockOperation(const BlockOperation& block_operation) {
    BlockIndexType local_x;
    BlockIndexType local_y;
    BlockIndexType local_z;
    BlockIndexType block_x_index = globalCoordinateToTopLevelBlockIndex(block_operation.x, local_x);
    BlockIndexType block_y_index = globalCoordinateToTopLevelBlockIndex(block_operation.y, local_y);
    BlockIndexType block_z_index = globalCoordinateToTopLevelBlockIndex(block_operation.z, local_z);
    BlockMaterialType existing_material = 0;
    auto line = g_world->getLineByAbsoluteIndex(block_x_index);
    if (line) {
        auto column = line->getColumnByAbsoluteIndex(block_y_index);
        if (column) {
            if (block_operation.put_block) {
                putBlock(column, block_z_index, block_operation.use_level, block_operation.level, local_x, local_y, local_z, block_operation.material);
            } else {
                removeBlock(column, block_z_index, block_operation.use_level, block_operation.level, local_x, local_y, local_z);
            }
        }
    }
    g_block_operation_queue.pop();
}

static void processBlockOperations(const boost::system::error_code& error) {
    SetThreadPriority(GetCurrentThread(), THREAD_MODE_BACKGROUND_BEGIN);

    BlockOperation block_operation;
    while (g_block_operation_queue.peek(block_operation) && g_is_running) {
        processBlockOperation(block_operation);
        initializeCacheCleaningIterators();
    }

    for (unsigned i = 0; i < WORLD_BLOCK_SIZE_X && g_is_running; ++i) {
        cleanUpCaches();
    }

    if (g_is_running) {
        g_timer->expires_at(g_timer->expires_at() + BLOCK_OPERATION_WAKE_UP_TIME_INTERVAL);
        g_timer->async_wait(&processBlockOperations);
    } else {
        // Clean up tessellation request queue
        BlockOperation block_operation;
        while (g_block_operation_queue.peek(block_operation)) {
            g_block_operation_queue.pop();
        }
    }

    SetThreadPriority(GetCurrentThread(), THREAD_MODE_BACKGROUND_END);
}

static void blockOperationThread() {
    initializeCacheCleaningIterators();
    g_io_service = std::make_unique<boost::asio::io_service>();
    g_timer = std::make_unique<boost::asio::deadline_timer>(*g_io_service, BLOCK_OPERATION_WAKE_UP_TIME_INTERVAL);
    g_timer->async_wait(&processBlockOperations);
    g_io_service->run();
}

void startBlockOperationThread() {
    g_block_operation_thread = std::make_unique<std::thread>(&blockOperationThread);
}

void finishBlockOperationThread() {
    g_block_operation_thread->join();
}

void postBlockOperation(const BlockOperation& block_operation) {
    g_block_operation_queue.push(block_operation);
}

template <std::uint8_t Level>
typename Block<Level>::Ptr getBlockCached(const typename Block<Level>::Ptr& block) {
    return getCached<Level>(block);
}

template <std::uint8_t Level>
struct GetBlockCachedInstantiation;

template <>
struct GetBlockCachedInstantiation<0> {
    static void instantiate() {
        getBlockCached<0>(nullptr);
    }
};

template <std::uint8_t Level>
struct GetBlockCachedInstantiation {
    static void instantiate() {
        getBlockCached<Level>(nullptr);
        GetBlockCachedInstantiation<Level - 1>::instantiate();
    }
};

template struct GetBlockCachedInstantiation<TOP_LEVEL>;
