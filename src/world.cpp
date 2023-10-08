// Copyright 2023 Petr Petrov. All rights reserved.
// License: https://github.com/PetrPPetrov/gkm-local/blob/main/LICENSE

#include "block_operation.h"
#include "game_logic.h"
#include "tessellation.h"
#include "world.h"

World::Ptr g_world = nullptr;

WorldLineY::WorldLineY() {
    for (BlockIndexType x = 0; x < WORLD_BLOCK_SIZE_Y; ++x) {
        columns.push_back(std::make_shared<WorldColumn>());
    }
}

World::World() {
    for (BlockIndexType x = 0; x < WORLD_BLOCK_SIZE_X; ++x) {
        lines.push_back(std::make_shared<WorldLineY>());
    }
}

static World::Ptr createFlatWorld() {
    auto empty_block = getBlockCached<TOP_LEVEL>(std::make_shared<TopLevelBlock>(0));
    auto empty_block0 = getBlockCached<0>(std::make_shared<Block<0>>(0));
    auto empty_block1 = getBlockCached<1>(std::make_shared<Block<1>>(0));
    auto empty_block2 = getBlockCached<2>(std::make_shared<Block<2>>(0));
    auto ground_block = getBlockCached<TOP_LEVEL>(std::make_shared<TopLevelBlock>(1));
    World::Ptr new_world = std::make_shared<World>();
    for (BlockIndexType x = 0; x < WORLD_BLOCK_SIZE_X; ++x) {
        auto line = new_world->getLine(x);
        for (BlockIndexType y = 0; y < WORLD_BLOCK_SIZE_Y; ++y) {
            auto column = line->getColumn(y);
            column->setBlock(0, ground_block);
            for (BlockIndexType z = 1; z < WORLD_BLOCK_HEIGHT; ++z) {
                column->setBlock(z, empty_block);
            }
        }
    }

    BlockOperation brick;
    brick.use_level = false;
    brick.put_block = true;
    brick.material = 2;
    brick.x = 8;
    brick.y = 40;
    brick.z = TopLevelBlock::SIZE + Block<1>::SIZE * 2 + 16 + 7;
    postBlockOperation(brick);

    BlockOperation brick0_0;
    brick0_0.use_level = true;
    brick0_0.level = 0;
    brick0_0.put_block = true;
    brick0_0.material = 2;
    brick0_0.x = 0;
    brick0_0.y = 40;
    brick0_0.z = TopLevelBlock::SIZE + Block<1>::SIZE * 2 + 16;
    postBlockOperation(brick0_0);

    BlockOperation brick0_1;
    brick0_1.use_level = true;
    brick0_1.level = 0;
    brick0_1.put_block = true;
    brick0_1.material = 2;
    brick0_1.x = 16;
    brick0_1.y = 40;
    brick0_1.z = TopLevelBlock::SIZE + Block<1>::SIZE * 2 + 16;
    postBlockOperation(brick0_1);

    BlockOperation brick0_2;
    brick0_2.use_level = true;
    brick0_2.level = 0;
    brick0_2.put_block = true;
    brick0_2.material = 2;
    brick0_2.x = 16;
    brick0_2.y = 40;
    brick0_2.z = TopLevelBlock::SIZE + Block<1>::SIZE * 2;
    postBlockOperation(brick0_2);

    BlockOperation brick0_3;
    brick0_3.use_level = true;
    brick0_3.level = 0;
    brick0_3.put_block = true;
    brick0_3.material = 2;
    brick0_3.x = 0;
    brick0_3.y = 40;
    brick0_3.z = TopLevelBlock::SIZE + Block<1>::SIZE * 2;
    postBlockOperation(brick0_3);

    BlockOperation brick1;
    brick1.use_level = true;
    brick1.level = 1;
    brick1.put_block = true;
    brick1.material = 2;
    brick1.x = 0;
    brick1.y = Block<1>::SIZE;
    brick1.z = TopLevelBlock::SIZE;
    postBlockOperation(brick1);

    BlockOperation brick1_2;
    brick1_2.use_level = true;
    brick1_2.level = 1;
    brick1_2.put_block = true;
    brick1_2.material = 2;
    brick1_2.x = 0;
    brick1_2.y = Block<1>::SIZE;
    brick1_2.z = TopLevelBlock::SIZE + Block<1>::SIZE * 2;
    postBlockOperation(brick1_2);

    BlockOperation brick2;
    brick2.use_level = true;
    brick2.level = 2;
    brick2.put_block = true;
    brick2.material = 2;
    brick2.x = 0;
    brick2.y = Block<2>::SIZE;
    brick2.z = TopLevelBlock::SIZE;
    postBlockOperation(brick2);

    BlockOperation brick3;
    brick3.use_level = true;
    brick3.level = 3;
    brick3.put_block = true;
    brick3.material = 2;
    brick3.x = 0;
    brick3.y = Block<3>::SIZE;
    brick3.z = TopLevelBlock::SIZE;
    postBlockOperation(brick3);

    return new_world;
}

void intializeWorld() {
    g_world = createFlatWorld();
}
