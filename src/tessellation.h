// Copyright 2023 Petr Petrov. All rights reserved.
// License: https://github.com/PetrPPetrov/gkm-local/blob/main/LICENSE

#pragma once

#include "block.h"

template <std::uint8_t Level>
struct TessellationRequest {
    bool finish = false;
    typename Block<Level>::Ptr block;
};

void startTessellationThreads();
void finishTessellationThreads();

template <std::uint8_t Level>
void postBlockTessellationRequest(const TessellationRequest<Level>& request);
