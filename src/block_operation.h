// Copyright 2023 Petr Petrov. All rights reserved.
// License: https://github.com/PetrPPetrov/gkm-local/blob/main/LICENSE

#pragma once

#include "game_logic.h"
#include "block.h"

struct BlockOperation {
    bool finish = false;
    bool use_level = false;
    std::uint8_t level = 0;
    BlockMaterial material;
    BlockIndex x, y, z;
};

void startBlockOperationThread();
void finishBlockOperationThread();
void postBlockOperation(const BlockOperation& block_operation);

template <std::uint8_t Level>
typename Block<Level>::Ptr getBlockCached(const typename Block<Level>::Ptr& block); // TODO: Not thread safe right now
