// Copyright 2023 Petr Petrov. All rights reserved.
// License: https://github.com/PetrPPetrov/gkm-local/blob/main/LICENSE

#pragma once

#include <cstdint>
#include <atomic>

// We use integer coordinates, 1 unit = 1 centimeter.
typedef std::int32_t BlockIndexType;

// Block material type. 0 means hole.
typedef std::uint8_t BlockMaterialType;

static_assert(std::atomic<BlockMaterialType>::is_always_lock_free);
