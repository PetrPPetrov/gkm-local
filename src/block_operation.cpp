// Copyright 2023 Petr Petrov. All rights reserved.
// License: https://github.com/PetrPPetrov/gkm-local/blob/main/LICENSE

#include <cstdint>
#include <unordered_map>
#include <thread>
#include "main.h"
#include "fnv_hash.h"
#include "block.h"
#include "world.h"
#include "request_queue.h"
#include "tessellation.h"
#include "block_operation.h"

static std::unique_ptr<std::thread> g_block_operation_thread = nullptr;
static RequestQueue<BlockOperation> g_block_operation_queue;

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
    static Block<0>::Ptr process(
        const Block<0>::Ptr& block,
        const BlockOperation& operation) {
        Block<0>::Ptr copy_block = nullptr;
        if (operation.use_level && operation.level == 0) {
            copy_block = std::make_shared<Block<0>>();
            copy_block->entire = true;
            copy_block->material = operation.material;
        } else {
            copy_block = std::make_shared<Block<0>>(*block);
            if (copy_block->entire) {
                copy_block->entire = false;
                BlockMaterialType overall_material = copy_block->material.load();
                for (BlockIndexType i = 0; i < Block<0>::MATERIAL_COUNT; ++i) {
                    copy_block->materials[i] = overall_material;
                }
            }
            auto block_index = operation.z * NESTED_BLOCKS * NESTED_BLOCKS + operation.y * NESTED_BLOCKS + operation.x;
            copy_block->materials[block_index] = operation.material;
            bool all_the_same = true;
            BlockMaterialType first_material = copy_block->materials[0];
            for (BlockIndexType i = 1; i < Block<0>::MATERIAL_COUNT; ++i) {
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
        return getCached<0>(copy_block);
    }
};

template <std::uint8_t Level>
struct BlockOperationProcessor {
    static typename Block<Level>::Ptr process(
        const typename Block<Level>::Ptr& block,
        const BlockOperation& operation) {
        Block<Level>::Ptr copy_block = nullptr;
        if (operation.use_level && operation.level == Level) {
            copy_block = std::make_shared<Block<Level>>();
            copy_block->entire = true;
            copy_block->material = operation.material;
        } else {
            copy_block = std::make_shared<Block<Level>>(*block);
            if (copy_block->entire) {
                copy_block->entire = false;
                auto sub_level_block = std::make_shared<Block<Level - 1>>(copy_block->material);
                auto cached_sub_level_block = getCached<Level - 1>(sub_level_block);
                for (BlockIndexType i = 0; i < Block<Level>::CHILDREN_COUNT; ++i) {
                    copy_block->children[i].write(cached_sub_level_block);
                }
            }
            BlockOperation sub_operation;
            sub_operation.use_level = operation.use_level;
            sub_operation.level = operation.level;
            sub_operation.material = operation.material;
            sub_operation.x = operation.x % Block<Level - 1>::SIZE;
            sub_operation.y = operation.y % Block<Level - 1>::SIZE;
            sub_operation.z = operation.z % Block<Level - 1>::SIZE;
            BlockIndexType sub_block_x = operation.x / Block<Level - 1>::SIZE;
            BlockIndexType sub_block_y = operation.y / Block<Level - 1>::SIZE;
            BlockIndexType sub_block_z = operation.z / Block<Level - 1>::SIZE;
            auto child_index = sub_block_z * NESTED_BLOCKS * NESTED_BLOCKS + sub_block_y * NESTED_BLOCKS + sub_block_x;
            auto child = copy_block->children[child_index].read();
            copy_block->children[child_index].write(BlockOperationProcessor<Level - 1>::process(child, sub_operation));
            bool all_sub_blocks_same = true;
            auto first_sub_block = copy_block->children[0].read();
            if (first_sub_block->entire) {
                for (BlockIndexType i = 1; i < Block<Level>::CHILDREN_COUNT; ++i) {
                    if (copy_block->children[i].read().get() != first_sub_block.get()) {
                        all_sub_blocks_same = false;
                        break;
                    }
                }
                if (all_sub_blocks_same) {
                    copy_block->entire = true;
                    copy_block->material = first_sub_block->material.load();
                    for (BlockIndexType i = 0; i < Block<Level>::CHILDREN_COUNT; ++i) {
                        copy_block->children[i].write(nullptr);
                    }
                }
            }
        }
        return getCached<Level>(copy_block);
    }
};

static inline void processBlockOperation(const BlockOperation& operation) {
    BlockOperation sub_operation;
    sub_operation.use_level = operation.use_level;
    sub_operation.level = operation.level;
    sub_operation.material = operation.material;
    BlockIndexType block_x_index = globalCoordinateToTopLevelBlockIndex(operation.x, sub_operation.x);
    BlockIndexType block_y_index = globalCoordinateToTopLevelBlockIndex(operation.y, sub_operation.y);
    BlockIndexType block_z_index = globalCoordinateToTopLevelBlockIndex(operation.z, sub_operation.z);

    auto line = g_world->getLineByAbsoluteIndex(block_x_index);
    if (line) {
        auto column = line->getColumnByAbsoluteIndex(block_y_index);
        if (column) {
            auto top_block = column->getBlock(block_z_index);
            BlockMaterialType existing_material = top_block->getMaterial(sub_operation.x, sub_operation.y, sub_operation.z);
            if (existing_material == operation.material) {
                // Do nothing.
                return;
            }
            if ((existing_material != 0) && (operation.material != 0)) {
                // Do nothing
                return;
            }
            auto result_top_block = BlockOperationProcessor<TOP_LEVEL>::process(top_block, sub_operation);
            column->setBlock(block_z_index, result_top_block);
        }
    }
}

static void blockOperationThread() {
    SetThreadPriority(GetCurrentThread(), THREAD_MODE_BACKGROUND_BEGIN);

    while (g_is_running) {
        BlockOperation block_operation;
        g_block_operation_queue.waitForNewRequests(block_operation);
        do {
            if (!g_is_running || block_operation.finish) {
                return;
            }
            processBlockOperation(block_operation);
        } while (g_block_operation_queue.pop(block_operation));

        initializeCacheCleaningIterators();
        for (BlockIndexType i = 0; i < WORLD_BLOCK_SIZE_X; ++i) {
            if (!g_is_running) {
                return;
            }
            cleanUpCaches();
        }
    }

    SetThreadPriority(GetCurrentThread(), THREAD_MODE_BACKGROUND_END);
}

void startBlockOperationThread() {
    g_block_operation_thread = std::make_unique<std::thread>(&blockOperationThread);
}

void finishBlockOperationThread() {
    BlockOperation wakeup_and_finish_operation;
    wakeup_and_finish_operation.finish = true;
    postBlockOperation(wakeup_and_finish_operation);
    g_block_operation_thread->join();
    g_block_operation_thread.reset();
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
